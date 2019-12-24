#ifndef DMGLIB_H
#define DMGLIB_H

#include <dmg/dmg.h>
#include "abstractfile.h"

#ifdef __cplusplus
extern "C" {
#endif
	int convertToDMG(AbstractFile* abstractIn, AbstractFile* abstractOut);
#ifdef __cplusplus
}
#endif

#endif
