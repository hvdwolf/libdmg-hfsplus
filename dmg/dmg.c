#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dmg.h>

int main(int argc, char *argv[]) {
  AbstractFile *iso = NULL;
  AbstractFile *dmg = NULL;

  if (argc < 3) {
    fprintf(stderr, "usage: ./dmg path/to/.iso path/to/.dmg\n");
    exit(EXIT_FAILURE);
  }

  iso = createAbstractFileFromFile(fopen(argv[1], "rb"));
  if (!iso) {
    fprintf(stderr, "Cannot open source ISO: %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  dmg = createAbstractFileFromFile(fopen(argv[2], "wb"));
  if (!dmg) {
    (iso)->close(iso);
    fprintf(stderr, "cannot open destination DMG: %s\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  return convertToDMG(iso, dmg);
}
