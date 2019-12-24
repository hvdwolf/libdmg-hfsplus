/*
 *  dmgfile.c
 *  libdmg-hfsplus
 */

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <dmg/dmg.h>
#include <dmg/adc.h>
#include <dmg/dmgfile.h>

static void cacheRun(DMG* dmg, BLKXTable* blkx, int run) {
	size_t bufferSize;
	z_stream strm;
	void* inBuffer;
	int ret;
	size_t have;

	if(dmg->runData) {
		free(dmg->runData);
	}

	bufferSize = SECTOR_SIZE * blkx->runs[run].sectorCount;

	dmg->runData = (void*) malloc(bufferSize);
	inBuffer = (void*) malloc(bufferSize);
	memset(dmg->runData, 0, bufferSize);

	ASSERT(dmg->dmg->seek(dmg->dmg, blkx->dataStart + blkx->runs[run].compOffset) == 0, "fseeko");

    switch(blkx->runs[run].type) {
                 case BLOCK_ADC:
                 {
			 size_t bufferRead = 0;
			 do {
				 strm.avail_in = dmg->dmg->read(dmg->dmg, inBuffer, blkx->runs[run].compLength);
				 strm.avail_out = adc_decompress(strm.avail_in, inBuffer, bufferSize, dmg->runData, &have);
				 bufferRead+=strm.avail_out;
			 } while (bufferRead < blkx->runs[run].compLength);
			 break;
                }

		case BLOCK_ZLIB:
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;

			ASSERT(inflateInit(&strm) == Z_OK, "inflateInit");

			ASSERT((strm.avail_in = dmg->dmg->read(dmg->dmg, inBuffer, blkx->runs[run].compLength)) == blkx->runs[run].compLength, "fread");
			strm.next_in = (unsigned char*) inBuffer;

			do {
				strm.avail_out = bufferSize;
				strm.next_out = (unsigned char*) dmg->runData;
				ASSERT((ret = inflate(&strm, Z_NO_FLUSH)) != Z_STREAM_ERROR, "inflate/Z_STREAM_ERROR");
				if(ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) {
					ASSERT(FALSE, "inflate");
				}
				have = bufferSize - strm.avail_out;
			} while (strm.avail_out == 0);

			ASSERT(inflateEnd(&strm) == Z_OK, "inflateEnd");
			break;
		case BLOCK_RAW:
			ASSERT((have = dmg->dmg->read(dmg->dmg, dmg->runData, blkx->runs[run].compLength)) == blkx->runs[run].compLength, "fread");
			break;
		case BLOCK_IGNORE:
			break;
		case BLOCK_COMMENT:
			break;
		case BLOCK_TERMINATOR:
			break;
		default:
			break;
    }

	dmg->runStart = (blkx->runs[run].sectorStart + blkx->firstSectorNumber) * SECTOR_SIZE;
	dmg->runEnd = dmg->runStart + (blkx->runs[run].sectorCount * SECTOR_SIZE);
}

static void cacheOffset(DMG* dmg, off_t location) {
	int i;
	int j;
	uint64_t sector;

	sector = (uint64_t)(location / SECTOR_SIZE);

	for(i = 0; i < dmg->numBLKX; i++) {
		if(sector >= dmg->blkx[i]->firstSectorNumber && sector < (dmg->blkx[i]->firstSectorNumber + dmg->blkx[i]->sectorCount)) {
			for(j = 0; j < dmg->blkx[i]->blocksRunCount; j++) {
				if(sector >= (dmg->blkx[i]->firstSectorNumber + dmg->blkx[i]->runs[j].sectorStart) &&
					sector < (dmg->blkx[i]->firstSectorNumber + dmg->blkx[i]->runs[j].sectorStart + dmg->blkx[i]->runs[j].sectorCount)) {
					cacheRun(dmg, dmg->blkx[i], j);
				}
			}
		}
	}
}

static int dmgFileRead(io_func* io, off_t location, size_t size, void *buffer) {
	DMG* dmg;
	size_t toRead;

	dmg = (DMG*) io->data;

	location += dmg->offset;

	if(size == 0) {
		return TRUE;
	}

	if(location < dmg->runStart || location >= dmg->runEnd) {
		cacheOffset(dmg, location);
	}

	if((location + size) > dmg->runEnd) {
		toRead = dmg->runEnd - location;
	} else {
		toRead = size;
	}

	memcpy(buffer, (void*)((uint8_t*)dmg->runData + (uint32_t)(location - dmg->runStart)), toRead);
	size -= toRead;
	location += toRead;
	buffer = (void*)((uint8_t*)buffer + toRead);

	if(size > 0) {
		return dmgFileRead(io, location - dmg->offset, size, buffer);
	} else {
		return TRUE;
	}
}

static int dmgFileWrite(io_func* io, off_t location, size_t size, void *buffer) {
	fprintf(stderr, "Error: writing to DMGs is not supported (impossible to achieve with compressed images and retain asr multicast ordering).\n");
	return FALSE;
}


static void closeDmgFile(io_func* io) {
	DMG* dmg;

	dmg = (DMG*) io->data;

	if(dmg->runData) {
		free(dmg->runData);
	}

	free(dmg->blkx);
	releaseResources(dmg->resources);
	dmg->dmg->close(dmg->dmg);
	free(dmg);
	free(io);
}
