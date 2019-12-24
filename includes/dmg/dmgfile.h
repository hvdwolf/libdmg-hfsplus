/*
 *  dmgfile.h
 *  libdmg-hfsplus
 *
 */

#include <dmg/dmg.h>

typedef struct DMG {
	AbstractFile* dmg;
	ResourceKey* resources;
	uint32_t numBLKX;
	BLKXTable** blkx;
	void* runData;
	uint64_t runStart;
	uint64_t runEnd;
	uint64_t offset;
} DMG;
