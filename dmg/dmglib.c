#include <abstractfile.h>
#include <common.h>
#include <dmg.h>
#include <string.h>

void convertToDMG(AbstractFile *abstractIn, AbstractFile *abstractOut) {

  BLKXTable *blkx;
  ResourceKey *resources = NULL;
  ResourceKey *curResource = NULL;

  ChecksumToken dataForkToken;
  ChecksumToken uncompressedToken;

  NSizResource *nsiz = NULL;
  NSizResource *myNSiz = NULL;
  CSumResource csum;

  off_t plistOffset;
  uint32_t plistSize;
  uint32_t dataForkChecksum;
  uint64_t numSectors = 0;

  UDIFResourceFile koly;

  off_t fileLength;

  memset(&dataForkToken, 0, sizeof(ChecksumToken));
  memset(koly.fUDIFMasterChecksum.data, 0,
         sizeof(koly.fUDIFMasterChecksum.data));
  memset(koly.fUDIFDataForkChecksum.data, 0,
         sizeof(koly.fUDIFDataForkChecksum.data));

  fileLength = abstractIn->getLength(abstractIn);

  memset(&uncompressedToken, 0, sizeof(uncompressedToken));

  abstractIn->seek(abstractIn, 0);
  blkx = insertBLKX(abstractOut, abstractIn, 0, fileLength, &BlockCRC,
                    &uncompressedToken, &CRCProxy, &dataForkToken);
  blkx->checksum.data[0] = uncompressedToken.crc;
  resources =
      insertData(resources, "blkx", 0, "whole disk (unknown partition : 0)",
                 (const char *)blkx,
                 sizeof(BLKXTable) + (blkx->blocksRunCount * sizeof(BLKXRun)),
                 ATTRIBUTE_HDIUTIL);
  free(blkx);

  memset(&csum, 0, sizeof(CSumResource));
  csum.version = 1;
  csum.type = CHECKSUM_MKBLOCK;
  csum.checksum = uncompressedToken.block;
  resources = insertData(resources, "cSum", 0, "", (const char *)(&csum),
                         sizeof(csum), 0);

  if (nsiz == NULL) {
    nsiz = myNSiz = (NSizResource *)malloc(sizeof(NSizResource));
  } else {
    myNSiz->next = (NSizResource *)malloc(sizeof(NSizResource));
    myNSiz = myNSiz->next;
  }

  memset(myNSiz, 0, sizeof(NSizResource));
  myNSiz->isVolume = 0;
  myNSiz->blockChecksum2 = uncompressedToken.block;
  myNSiz->partitionNumber = 0;
  myNSiz->version = 6;
  myNSiz->next = NULL;

  koly.fUDIFImageVariant = kUDIFPartitionImageType;

  dataForkChecksum = dataForkToken.crc;

  printf("Writing XML data...\n");
  fflush(stdout);
  curResource = resources;
  while (curResource->next != NULL) {
    curResource = curResource->next;
  }

  curResource->next = writeNSiz(nsiz);
  curResource = curResource->next;
  releaseNSiz(nsiz);

  curResource->next = makePlst();
  curResource = curResource->next;

  plistOffset = abstractOut->tell(abstractOut);
  writeResources(abstractOut, resources);
  plistSize = abstractOut->tell(abstractOut) - plistOffset;

  printf("Generating UDIF metadata...\n");
  fflush(stdout);

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
  // The output of rand() here is deterministic.
  // No calls to srand() are made, so rand() is
  // automatically seeded with a value of 1.
  // See: https://linux.die.net/man/3/rand
  // TODO: Just hardcode a "random" value. As this
  // needs to remain deterministic.
  koly.fUDIFSegmentID.data1 = rand();
  koly.fUDIFSegmentID.data2 = rand();
  koly.fUDIFSegmentID.data3 = rand();
  koly.fUDIFSegmentID.data4 = rand();
  koly.fUDIFDataForkChecksum.type = CHECKSUM_CRC32;
  koly.fUDIFDataForkChecksum.size = KOLY_CHECKSUM_SIZE;
  koly.fUDIFDataForkChecksum.data[0] = dataForkChecksum;
  koly.fUDIFXMLOffset = plistOffset;
  koly.fUDIFXMLLength = plistSize;
  memset(&(koly.reserved1), 0, KOLY_RESERVED);

  koly.fUDIFMasterChecksum.type = CHECKSUM_CRC32;
  koly.fUDIFMasterChecksum.size = KOLY_CHECKSUM_SIZE;
  koly.fUDIFMasterChecksum.data[0] = calculateMasterChecksum(resources);
  printf("Master checksum: %x\n", koly.fUDIFMasterChecksum.data[0]);
  fflush(stdout);

  koly.fUDIFSectorCount = numSectors;
  koly.reserved2 = 0;
  koly.reserved3 = 0;
  koly.reserved4 = 0;

  printf("Writing out UDIF resource file...\n");
  fflush(stdout);

  writeUDIFResourceFile(abstractOut, &koly);

  printf("Cleaning up...\n");
  fflush(stdout);

  releaseResources(resources);

  abstractIn->close(abstractIn);

  printf("Done\n");
  fflush(stdout);

  abstractOut->close(abstractOut);
}
