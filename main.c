#include "hjson.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int load_file(const char *filename, char **result) 
{ 
	int size = 0;
	FILE *f = fopen(filename, "rb");

	if (f == NULL) 
	{ 
		*result = NULL;
		return -1; // -1 means file opening fail 
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size+1);

	if (size != fread(*result, sizeof(char), size, f)) 
	{ 
		free(*result);
		return -2; // -2 means file reading fail 
	} 

	fclose(f);
	(*result)[size] = 0;
	return size;
}

int main()
{
	char* file;
	int error = 0;
	HJsonValue output = {0};
	load_file("test.hjson", &file);
	error = hjson_parse(file, &output);
	assert(error == 0);
	printf("%i", output.is_object);
	getchar();
	return 0;
}
