#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dmg/dmg.h>

int buildInOut(const char* source, const char* dest, AbstractFile** in, AbstractFile** out) {
	*in = createAbstractFileFromFile(fopen(source, "rb"));
	if(!(*in)) {
		printf("cannot open source ISO: %s\n", source);
		return 0;
	}

	*out = createAbstractFileFromFile(fopen(dest, "wb"));
	if(!(*out)) {
		(*in)->close(*in);
		printf("cannot open destination DMG: %s\n", dest);
		return 0;
	}

	return 1;
}

int main(int argc, char* argv[]) {
	AbstractFile* in;
	AbstractFile* out;
	
	if(argc < 3) {
		printf("usage: ./dmg .iso .dmg\n");
		return 0;
	}

	if(!buildInOut(argv[1], argv[2], &in, &out)) {
		return 0;
	}

	convertToDMG(in, out);
	return 0;
}
