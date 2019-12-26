#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <dmg.h>
#include <inttypes.h>

#define BUFFERS_NEEDED 0x208 // 520
#define ENTIRE_DEVICE_DESCRIPTOR 0xFFFFFFFE
#define SECTORS_AT_A_TIME 0x200 // 512
#define SECTOR_SIZE 512

BLKXTable *insertBLKX(AbstractFile *out, AbstractFile *in,
                      ChecksumFunc uncompressedChk, void *uncompressedChkToken,
                      ChecksumFunc compressedChk, void *compressedChkToken) {
  BLKXTable *blkx;

  uint32_t roomForRuns = 2;
  uint32_t curRun = 0;
  uint64_t curSector = 0;

  unsigned char *inBuffer;
  unsigned char *outBuffer;
  size_t bufferSize;
  size_t have;
  int ret;

  uint32_t numSectors = in->getLength(in) / SECTOR_SIZE;

  z_stream strm;

  blkx = (BLKXTable *)malloc(sizeof(BLKXTable) + (2 * sizeof(BLKXRun)));
  memset(blkx, 0, sizeof(BLKXTable) + (roomForRuns * sizeof(BLKXRun)));

  blkx->fUDIFBlocksSignature = UDIF_BLOCK_SIGNATURE;
  blkx->infoVersion = 1;
  blkx->firstSectorNumber = 0;
  blkx->sectorCount = numSectors;
  blkx->dataStart = 0;
  blkx->decompressBufferRequested = BUFFERS_NEEDED;
  blkx->blocksDescriptor = ENTIRE_DEVICE_DESCRIPTOR;
  blkx->reserved1 = 0;
  blkx->reserved2 = 0;
  blkx->reserved3 = 0;
  blkx->reserved4 = 0;
  blkx->reserved5 = 0;
  blkx->reserved6 = 0;
  memset(&(blkx->checksum), 0, sizeof(blkx->checksum));
  blkx->checksum.type = CHECKSUM_CRC32;
  blkx->checksum.size = KOLY_CHECKSUM_SIZE;
  blkx->blocksRunCount = 0;

  bufferSize = SECTOR_SIZE * blkx->decompressBufferRequested;

  ASSERT(inBuffer = (unsigned char *)malloc(bufferSize), "malloc");
  ASSERT(outBuffer = (unsigned char *)malloc(bufferSize), "malloc");

  while (numSectors > 0) {
    if (curRun >= roomForRuns) {
      roomForRuns <<= 1;
      blkx = (BLKXTable *)realloc(blkx, sizeof(BLKXTable) +
                                            (roomForRuns * sizeof(BLKXRun)));
    }

    blkx->runs[curRun].type = BLOCK_ZLIB;
    blkx->runs[curRun].reserved = 0;
    blkx->runs[curRun].sectorStart = curSector;
    blkx->runs[curRun].sectorCount =
        (numSectors > SECTORS_AT_A_TIME) ? SECTORS_AT_A_TIME : numSectors;

    memset(&strm, 0, sizeof(strm));
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // printf("run %d: sectors=%" PRId64 ", left=%d\n", curRun,
    // blkx->runs[curRun].sectorCount, numSectors);

    ASSERT(deflateInit(&strm, Z_DEFAULT_COMPRESSION) == Z_OK, "deflateInit");

    ASSERT((strm.avail_in = in->read(
                in, inBuffer, blkx->runs[curRun].sectorCount * SECTOR_SIZE)) ==
               (blkx->runs[curRun].sectorCount * SECTOR_SIZE),
           "mRead");
    strm.next_in = inBuffer;

    if (uncompressedChk)
      (*uncompressedChk)(uncompressedChkToken, inBuffer,
                         blkx->runs[curRun].sectorCount * SECTOR_SIZE);

    blkx->runs[curRun].compOffset = out->tell(out) - blkx->dataStart;
    blkx->runs[curRun].compLength = 0;

    strm.avail_out = bufferSize;
    strm.next_out = outBuffer;

    ASSERT((ret = deflate(&strm, Z_FINISH)) != Z_STREAM_ERROR,
           "deflate/Z_STREAM_ERROR");
    if (ret != Z_STREAM_END) {
      ASSERT(0, "deflate");
    }
    have = bufferSize - strm.avail_out;

    if ((have / SECTOR_SIZE) > blkx->runs[curRun].sectorCount) {
      blkx->runs[curRun].type = BLOCK_RAW;
      ASSERT(out->write(out, inBuffer,
                        blkx->runs[curRun].sectorCount * SECTOR_SIZE) ==
                 (blkx->runs[curRun].sectorCount * SECTOR_SIZE),
             "fwrite");
      blkx->runs[curRun].compLength +=
          blkx->runs[curRun].sectorCount * SECTOR_SIZE;

      if (compressedChk)
        (*compressedChk)(compressedChkToken, inBuffer,
                         blkx->runs[curRun].sectorCount * SECTOR_SIZE);

    } else {
      ASSERT(out->write(out, outBuffer, have) == have, "fwrite");

      if (compressedChk)
        (*compressedChk)(compressedChkToken, outBuffer, have);

      blkx->runs[curRun].compLength += have;
    }

    deflateEnd(&strm);

    curSector += blkx->runs[curRun].sectorCount;
    numSectors -= blkx->runs[curRun].sectorCount;
    curRun++;
  }

  if (curRun >= roomForRuns) {
    roomForRuns <<= 1;
    blkx = (BLKXTable *)realloc(blkx, sizeof(BLKXTable) +
                                          (roomForRuns * sizeof(BLKXRun)));
  }

  blkx->runs[curRun].type = BLOCK_TERMINATOR;
  blkx->runs[curRun].reserved = 0;
  blkx->runs[curRun].sectorStart = curSector;
  blkx->runs[curRun].sectorCount = 0;
  blkx->runs[curRun].compOffset = out->tell(out) - blkx->dataStart;
  blkx->runs[curRun].compLength = 0;
  blkx->blocksRunCount = curRun + 1;

  free(inBuffer);
  free(outBuffer);

  return blkx;
}
