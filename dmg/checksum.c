#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dmg.h>

void BlockCRC(void *token, const unsigned char *data, size_t len) {
  ChecksumToken *ckSumToken;
  ckSumToken = (ChecksumToken *)token;
  MKBlockChecksum(&(ckSumToken->block), data, len);
  CRC32Checksum(&(ckSumToken->crc), data, len);
}

uint32_t calculateMasterChecksum(ResourceKey *resources) {
  ResourceKey *blkxKeys;
  ResourceData *data;
  BLKXTable *blkx;
  unsigned char *buffer;
  int blkxNum = 0;
  uint32_t result = 0;

  blkxKeys = getResourceByKey(resources, "blkx");

  data = blkxKeys->data;
  while (data != NULL) {
    blkx = (BLKXTable *)data->data;
    if (blkx->checksum.type == CHECKSUM_CRC32) {
      blkxNum++;
    }
    data = data->next;
  }

  buffer = (unsigned char *)malloc(4 * blkxNum);
  data = blkxKeys->data;
  blkxNum = 0;
  while (data != NULL) {
    blkx = (BLKXTable *)data->data;
    if (blkx->checksum.type == CHECKSUM_CRC32) {
      buffer[(blkxNum * 4) + 0] = (blkx->checksum.data[0] >> 24) & 0xff;
      buffer[(blkxNum * 4) + 1] = (blkx->checksum.data[0] >> 16) & 0xff;
      buffer[(blkxNum * 4) + 2] = (blkx->checksum.data[0] >> 8) & 0xff;
      buffer[(blkxNum * 4) + 3] = (blkx->checksum.data[0] >> 0) & 0xff;
      blkxNum++;
    }
    data = data->next;
  }

  CRC32Checksum(&result, (const unsigned char *)buffer, 4 * blkxNum);
  free(buffer);
  return result;
}

void CRCProxy(void *token, const unsigned char *data, size_t len) {
  ChecksumToken *ckSumToken;
  ckSumToken = (ChecksumToken *)token;
  CRC32Checksum(&(ckSumToken->crc), data, len);
}

/*
 *
 * MediaKit block checksumming reverse-engineered from Leopard MediaKit
 * framework
 *
 */

uint32_t MKBlockChecksum(uint32_t *ckSum, const unsigned char *data,
                         size_t len) {
  uint32_t *curDWordPtr;
  uint32_t curDWord;
  uint32_t myCkSum;

  myCkSum = *ckSum;

  if (data) {
    curDWordPtr = (uint32_t *)data;
    while (curDWordPtr < (uint32_t *)(&data[len & 0xFFFFFFFC])) {
      curDWord = *curDWordPtr;
      FLIPENDIAN(curDWord);
      myCkSum = curDWord + ((myCkSum >> 31) | (myCkSum << 1));
      curDWordPtr++;
    }
  }

  *ckSum = myCkSum;
  return myCkSum;
}
