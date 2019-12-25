#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#define fseeko fseeko64
#define ftello ftello64
#define off_t off64_t
#endif

#define FLIPENDIAN(x) flipEndian((unsigned char *)(&(x)), sizeof(x))

#define TIME_OFFSET_FROM_UNIX 2082844800L
#define APPLE_TO_UNIX_TIME(x) ((x)-TIME_OFFSET_FROM_UNIX)

#define ASSERT(x, m)                                                           \
  if (!(x)) {                                                                  \
    fflush(stdout);                                                            \
    fprintf(stderr, "error: %s\n", m);                                         \
    perror("error");                                                           \
    fflush(stderr);                                                            \
    exit(1);                                                                   \
  }

static inline int endianness() {
  short int word = 0x0001;
  char *byte = (char *)&word;
  return byte[0] ? 1 : 0;
}

static inline void flipEndian(unsigned char *x, int length) {
  int i;
  unsigned char tmp;

  if (endianness() == 0) {
    return; // little endian
  } else {
    for (i = 0; i < (length / 2); i++) {
      tmp = x[i];
      x[i] = x[length - i - 1];
      x[length - i - 1] = tmp;
    }
  }
}

struct io_func_struct;

typedef int (*readFunc)(struct io_func_struct *io, off_t location, size_t size,
                        void *buffer);
typedef int (*writeFunc)(struct io_func_struct *io, off_t location, size_t size,
                         void *buffer);
typedef void (*closeFunc)(struct io_func_struct *io);

typedef struct io_func_struct {
  void *data;
  readFunc read;
  writeFunc write;
  closeFunc close;
} io_func;

#endif
