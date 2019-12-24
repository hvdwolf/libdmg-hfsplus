#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dmg/dmg.h>

char endianness;

void TestByteOrder()
{
	short int word = 0x0001;
	char *byte = (char *) &word;
	endianness = byte[0] ? IS_LITTLE_ENDIAN : IS_BIG_ENDIAN;
}

int buildInOut(const char* source, const char* dest, AbstractFile** in, AbstractFile** out) {
	*in = createAbstractFileFromFile(fopen(source, "rb"));
	if(!(*in)) {
		printf("cannot open source: %s\n", source);
		return 0;
	}

	*out = createAbstractFileFromFile(fopen(dest, "wb"));
	if(!(*out)) {
		(*in)->close(*in);
		printf("cannot open destination: %s\n", dest);
		return 0;
	}

	return 1;
}

int main(int argc, char* argv[]) {
	AbstractFile* in;
	AbstractFile* out;
	
	TestByteOrder();
	
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
