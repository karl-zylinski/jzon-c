#include "jzon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	char* file;
	int error = 0;
	JzonParseResult result;
	load_file("test.jzon", &file);
	result = jzon_parse(file);
	assert(result.success == true);
	pretty_print(0, result.output);
	jzon_free(result.output);
	getchar();
	return 0;
}
