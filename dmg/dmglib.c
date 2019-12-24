#include <string.h>
#include "common.h"
#include "abstractfile.h"
#include <dmg/dmg.h>

uint32_t calculateMasterChecksum(ResourceKey* resources);

uint32_t calculateMasterChecksum(ResourceKey* resources) {
	ResourceKey* blkxKeys;
	ResourceData* data;
	BLKXTable* blkx;
	unsigned char* buffer;
	int blkxNum;
	uint32_t result;
	
	blkxKeys = getResourceByKey(resources, "blkx");
	
	data = blkxKeys->data;
	blkxNum = 0;
	while(data != NULL) {
		blkx = (BLKXTable*) data->data;
		if(blkx->checksum.type == CHECKSUM_CRC32) {
			blkxNum++;
		}
		data = data->next;
	}
	
	buffer = (unsigned char*) malloc(4 * blkxNum) ;
	data = blkxKeys->data;
	blkxNum = 0;
	while(data != NULL) {
		blkx = (BLKXTable*) data->data;
		if(blkx->checksum.type == CHECKSUM_CRC32) {
			buffer[(blkxNum * 4) + 0] = (blkx->checksum.data[0] >> 24) & 0xff;
			buffer[(blkxNum * 4) + 1] = (blkx->checksum.data[0] >> 16) & 0xff;
			buffer[(blkxNum * 4) + 2] = (blkx->checksum.data[0] >> 8) & 0xff;
			buffer[(blkxNum * 4) + 3] = (blkx->checksum.data[0] >> 0) & 0xff;
			blkxNum++;
		}
		data = data->next;
	}
	
	result = 0;
	CRC32Checksum(&result, (const unsigned char*) buffer, 4 * blkxNum);
	free(buffer);
	return result;  
}

int convertToDMG(AbstractFile* abstractIn, AbstractFile* abstractOut) {
	Partition* partitions;
	DriverDescriptorRecord* DDM;
	int i;
	
	BLKXTable* blkx;
	ResourceKey* resources;
	ResourceKey* curResource;
	
	ChecksumToken dataForkToken;
	ChecksumToken uncompressedToken;
	
	NSizResource* nsiz;
	NSizResource* myNSiz;
	CSumResource csum;
	
	off_t plistOffset;
	uint32_t plistSize;
	uint32_t dataForkChecksum;
	uint64_t numSectors;
	
	UDIFResourceFile koly;
	
	char partitionName[512];
	
	off_t fileLength;
	size_t partitionTableSize;
	
	unsigned int BlockSize;
	
	numSectors = 0;
	
	resources = NULL;
	nsiz = NULL;
	myNSiz = NULL;
	memset(&dataForkToken, 0, sizeof(ChecksumToken));
	memset(koly.fUDIFMasterChecksum.data, 0, sizeof(koly.fUDIFMasterChecksum.data));
	memset(koly.fUDIFDataForkChecksum.data, 0, sizeof(koly.fUDIFDataForkChecksum.data));
	
	partitions = (Partition*) malloc(SECTOR_SIZE);
	
	printf("Processing DDM...\n"); fflush(stdout);
	DDM = (DriverDescriptorRecord*) malloc(SECTOR_SIZE);
	abstractIn->seek(abstractIn, 0);
	ASSERT(abstractIn->read(abstractIn, DDM, SECTOR_SIZE) == SECTOR_SIZE, "fread");
	flipDriverDescriptorRecord(DDM, FALSE);
	
	if(DDM->sbSig == DRIVER_DESCRIPTOR_SIGNATURE) {
		BlockSize = DDM->sbBlkSize;
		writeDriverDescriptorMap(abstractOut, DDM, &CRCProxy, (void*) (&dataForkToken), &resources);
		free(DDM);
		
		printf("Processing partition map...\n"); fflush(stdout);
		
		abstractIn->seek(abstractIn, BlockSize);
		ASSERT(abstractIn->read(abstractIn, partitions, BlockSize) == BlockSize, "fread");
		flipPartitionMultiple(partitions, FALSE, FALSE, BlockSize);
		
		partitionTableSize = BlockSize * partitions->pmMapBlkCnt;
		partitions = (Partition*) realloc(partitions, partitionTableSize);
		
		abstractIn->seek(abstractIn, BlockSize);
		ASSERT(abstractIn->read(abstractIn, partitions, partitionTableSize) == partitionTableSize, "fread");
		flipPartition(partitions, FALSE, BlockSize);
		
		printf("Writing blkx (%d)...\n", partitions->pmMapBlkCnt); fflush(stdout);
		
		for(i = 0; i < partitions->pmMapBlkCnt; i++) {
			if(partitions[i].pmSig != APPLE_PARTITION_MAP_SIGNATURE) {
				break;
			}
			
			printf("Processing blkx %d, total %d...\n", i, partitions->pmMapBlkCnt); fflush(stdout);
			
			sprintf(partitionName, "%s (%s : %d)", partitions[i].pmPartName, partitions[i].pmParType, i + 1);
			
			memset(&uncompressedToken, 0, sizeof(uncompressedToken));
			
			abstractIn->seek(abstractIn, partitions[i].pmPyPartStart * BlockSize);
			blkx = insertBLKX(abstractOut, abstractIn, partitions[i].pmPyPartStart, partitions[i].pmPartBlkCnt, i, CHECKSUM_CRC32,
						&BlockCRC, &uncompressedToken, &CRCProxy, &dataForkToken, NULL);
			
			blkx->checksum.data[0] = uncompressedToken.crc;	
			resources = insertData(resources, "blkx", i, partitionName, (const char*) blkx, sizeof(BLKXTable) + (blkx->blocksRunCount * sizeof(BLKXRun)), ATTRIBUTE_HDIUTIL);
			free(blkx);
			
			memset(&csum, 0, sizeof(CSumResource));
			csum.version = 1;
			csum.type = CHECKSUM_MKBLOCK;
			csum.checksum = uncompressedToken.block;
			resources = insertData(resources, "cSum", i, "", (const char*) (&csum), sizeof(csum), 0);
			
			if(nsiz == NULL) {
				nsiz = myNSiz = (NSizResource*) malloc(sizeof(NSizResource));
			} else {
				myNSiz->next = (NSizResource*) malloc(sizeof(NSizResource));
				myNSiz = myNSiz->next;
			}
			
			memset(myNSiz, 0, sizeof(NSizResource));
			myNSiz->isVolume = FALSE;
			myNSiz->blockChecksum2 = uncompressedToken.block;
			myNSiz->partitionNumber = i;
			myNSiz->version = 6;
			myNSiz->next = NULL;
			
			if((partitions[i].pmPyPartStart + partitions[i].pmPartBlkCnt) > numSectors) {
				numSectors = partitions[i].pmPyPartStart + partitions[i].pmPartBlkCnt;
			}
		}
		
		koly.fUDIFImageVariant = kUDIFDeviceImageType;
	} else {
		printf("No DDM! Just doing one huge blkx then...\n"); fflush(stdout);
		
		fileLength = abstractIn->getLength(abstractIn);
		
		memset(&uncompressedToken, 0, sizeof(uncompressedToken));
		
		abstractIn->seek(abstractIn, 0);
		blkx = insertBLKX(abstractOut, abstractIn, 0, fileLength/SECTOR_SIZE, ENTIRE_DEVICE_DESCRIPTOR, CHECKSUM_CRC32,
					&BlockCRC, &uncompressedToken, &CRCProxy, &dataForkToken, NULL);
		blkx->checksum.data[0] = uncompressedToken.crc;
		resources = insertData(resources, "blkx", 0, "whole disk (unknown partition : 0)", (const char*) blkx, sizeof(BLKXTable) + (blkx->blocksRunCount * sizeof(BLKXRun)), ATTRIBUTE_HDIUTIL);
		free(blkx);
		
		memset(&csum, 0, sizeof(CSumResource));
		csum.version = 1;
		csum.type = CHECKSUM_MKBLOCK;
		csum.checksum = uncompressedToken.block;
		resources = insertData(resources, "cSum", 0, "", (const char*) (&csum), sizeof(csum), 0);
		
		if(nsiz == NULL) {
			nsiz = myNSiz = (NSizResource*) malloc(sizeof(NSizResource));
		} else {
			myNSiz->next = (NSizResource*) malloc(sizeof(NSizResource));
			myNSiz = myNSiz->next;
		}
		
		memset(myNSiz, 0, sizeof(NSizResource));
		myNSiz->isVolume = FALSE;
		myNSiz->blockChecksum2 = uncompressedToken.block;
		myNSiz->partitionNumber = 0;
		myNSiz->version = 6;
		myNSiz->next = NULL;
		
		koly.fUDIFImageVariant = kUDIFPartitionImageType;
	}
	
	dataForkChecksum = dataForkToken.crc;
	
	printf("Writing XML data...\n"); fflush(stdout);
	curResource = resources;
	while(curResource->next != NULL)
		curResource = curResource->next;
    
	curResource->next = writeNSiz(nsiz);
	curResource = curResource->next;
	releaseNSiz(nsiz);
	
	curResource->next = makePlst();
	curResource = curResource->next;
	
	plistOffset = abstractOut->tell(abstractOut);
	writeResources(abstractOut, resources);
	plistSize = abstractOut->tell(abstractOut) - plistOffset;
	
	printf("Generating UDIF metadata...\n"); fflush(stdout);
	
	koly.fUDIFSignature = KOLY_SIGNATURE;
	koly.fUDIFVersion = 4;
	koly.fUDIFHeaderSize = sizeof(koly);
	koly.fUDIFFlags = kUDIFFlagsFlattened;
	koly.fUDIFRunningDataForkOffset = 0;
	koly.fUDIFDataForkOffset = 0;
	koly.fUDIFDataForkLength = plistOffset;
	koly.fUDIFRsrcForkOffset = 0;
	koly.fUDIFRsrcForkLength = 0;
	
	koly.fUDIFSegmentNumber = 1;
	koly.fUDIFSegmentCount = 1;
	koly.fUDIFSegmentID.data1 = rand();
	koly.fUDIFSegmentID.data2 = rand();
	koly.fUDIFSegmentID.data3 = rand();
	koly.fUDIFSegmentID.data4 = rand();
	koly.fUDIFDataForkChecksum.type = CHECKSUM_CRC32;
	koly.fUDIFDataForkChecksum.size = 0x20;
	koly.fUDIFDataForkChecksum.data[0] = dataForkChecksum;
	koly.fUDIFXMLOffset = plistOffset;
	koly.fUDIFXMLLength = plistSize;
	memset(&(koly.reserved1), 0, 0x78);
	
	koly.fUDIFMasterChecksum.type = CHECKSUM_CRC32;
	koly.fUDIFMasterChecksum.size = 0x20;
	koly.fUDIFMasterChecksum.data[0] = calculateMasterChecksum(resources);
	printf("Master checksum: %x\n", koly.fUDIFMasterChecksum.data[0]); fflush(stdout); 
	
	koly.fUDIFSectorCount = numSectors;
	koly.reserved2 = 0;
	koly.reserved3 = 0;
	koly.reserved4 = 0;
	
	printf("Writing out UDIF resource file...\n"); fflush(stdout); 
	
	writeUDIFResourceFile(abstractOut, &koly);
	
	printf("Cleaning up...\n"); fflush(stdout);
	
	releaseResources(resources);
	
	abstractIn->close(abstractIn);
	free(partitions);
	
	printf("Done\n"); fflush(stdout);

	abstractOut->close(abstractOut);
	
	return TRUE;
}
