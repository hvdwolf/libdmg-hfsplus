#ifndef ABSTRACTFILE_H
#define ABSTRACTFILE_H

#include <common.h>
#include <stdint.h>

typedef struct AbstractFile AbstractFile;
typedef struct AbstractFile2 AbstractFile2;

typedef size_t (*WriteFunc)(AbstractFile *file, const void *data, size_t len);
typedef size_t (*ReadFunc)(AbstractFile *file, void *data, size_t len);
typedef int (*SeekFunc)(AbstractFile *file, off_t offset);
typedef off_t (*TellFunc)(AbstractFile *file);
typedef void (*CloseFunc)(AbstractFile *file);
typedef off_t (*GetLengthFunc)(AbstractFile *file);
typedef void (*SetKeyFunc)(AbstractFile2 *file, const unsigned int *key,
                           const unsigned int *iv);

typedef enum AbstractFileType {
  AbstractFileTypeFile,
  AbstractFileType8900,
  AbstractFileTypeImg2,
  AbstractFileTypeImg3,
  AbstractFileTypeLZSS,
  AbstractFileTypeIBootIM,
  AbstractFileTypeMem,
  AbstractFileTypeMemFile,
  AbstractFileTypeDummy
} AbstractFileType;

struct AbstractFile {
  void *data;
  WriteFunc write;
  ReadFunc read;
  SeekFunc seek;
  TellFunc tell;
  GetLengthFunc getLength;
  CloseFunc close;
  AbstractFileType type;
};

struct AbstractFile2 {
  AbstractFile super;
  SetKeyFunc setKey;
};

#ifdef __cplusplus
extern "C" {
#endif
AbstractFile *createAbstractFileFromFile(FILE *file);
void abstractFilePrint(AbstractFile *file, const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif
