#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abstractfile.h"
#include "common.h"

size_t freadWrapper(AbstractFile *file, void *data, size_t len) {
  return fread(data, 1, len, (FILE *)(file->data));
}

size_t fwriteWrapper(AbstractFile *file, const void *data, size_t len) {
  return fwrite(data, 1, len, (FILE *)(file->data));
}

int fseekWrapper(AbstractFile *file, off_t offset) {
  return fseeko((FILE *)(file->data), offset, SEEK_SET);
}

off_t ftellWrapper(AbstractFile *file) { return ftello((FILE *)(file->data)); }

void fcloseWrapper(AbstractFile *file) {
  fclose((FILE *)(file->data));
  free(file);
}

off_t fileGetLength(AbstractFile *file) {
  off_t length;
  off_t pos;

  pos = ftello((FILE *)(file->data));

  fseeko((FILE *)(file->data), 0, SEEK_END);
  length = ftello((FILE *)(file->data));

  fseeko((FILE *)(file->data), pos, SEEK_SET);

  return length;
}

AbstractFile *createAbstractFileFromFile(FILE *file) {
  AbstractFile *toReturn;

  if (file == NULL) {
    return NULL;
  }

  toReturn = (AbstractFile *)malloc(sizeof(AbstractFile));
  toReturn->data = file;
  toReturn->read = freadWrapper;
  toReturn->write = fwriteWrapper;
  toReturn->seek = fseekWrapper;
  toReturn->tell = ftellWrapper;
  toReturn->getLength = fileGetLength;
  toReturn->close = fcloseWrapper;
  toReturn->type = AbstractFileTypeFile;
  return toReturn;
}

void abstractFilePrint(AbstractFile *file, const char *format, ...) {
  va_list args;
  char buffer[1024];
  size_t length;

  buffer[0] = '\0';
  va_start(args, format);
  length = vsprintf(buffer, format, args);
  va_end(args);
  ASSERT(file->write(file, buffer, length) == length, "fwrite");
}
