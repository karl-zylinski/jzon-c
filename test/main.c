#include "../jzon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LoadedFile
{
	int size;
	char* data;
} LoadedFile;

LoadedFile load_file(const char *filename)
{ 
	FILE* fp;
	size_t filesize;
	unsigned char* data;
	fp = fopen(filename, "rb");

	assert(fp);

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = (unsigned char*)malloc(filesize);
	assert(data);

	fread(data, 1, filesize, fp);
	data[filesize] = 0;
	fclose(fp);

	LoadedFile lf;
	lf.data = (char*)data;
	lf.size = filesize;
	return lf;
}

void pretty_print(int depth, JzonValue* value)
{
	char prefix[100];
	char new_prefix[100];	
	unsigned i = 0;

	memset(prefix, ' ', depth * 2);
	prefix[depth * 2] = '\0';
	memset(new_prefix, ' ', (depth + 1) * 2);
	new_prefix[(depth + 1) * 2] = '\0';

	if (value->is_object)
	{
		printf("\n");
		printf(prefix);
		printf("{");
		printf("\n");
		for (i = 0; i < value->size; ++i)
		{
			printf(new_prefix);
			printf(value->object_values[i]->key);
			pretty_print(depth + 1, value->object_values[i]->value);
			printf("\n");
		}
		
		printf(prefix);
		printf("}");
	}
	else if (value->is_array)
	{
		printf("\n");
		printf(prefix);
		printf("[");
		printf("\n");
		for (i = 0; i < value->size; ++i)
		{
			printf(new_prefix);
			pretty_print(depth + 1, value->array_values[i]);
			printf("\n");
		}
		
		printf(prefix);
		printf("]");
	}
	else if (value->is_bool)
	{
		printf(prefix);

		if (value->bool_value)
			printf("true");
		else
			printf("false");
	}
	else if (value->is_float)
	{
		printf(prefix);
		printf("%f", value->float_value);
	}
	else if (value->is_int)
	{
		printf(prefix);
		printf("%i", value->int_value);
	}
	else if (value->is_null)
	{
		printf(prefix);
		printf("null");
	}
	else if (value->is_string)
	{
		printf(prefix);
		printf(value->string_value);
	}
}

int main()
{
	LoadedFile file = load_file("test.jzon");
	JzonParseResult result = jzon_parse(file.data);
	assert(result.success == true);
	pretty_print(0, result.output);
	JzonValue* trailing_value = jzon_get(result.output, "mysterious_words_by_id");
	assert(trailing_value != NULL);
	(void)trailing_value;
	jzon_free(result.output);
	getchar();
	return 0;
}
