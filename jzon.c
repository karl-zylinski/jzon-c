#include "jzon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// String helpers

typedef struct String
{
	int size;
	int capacity;
	char* str;
} String;

void str_grow(String* str)
{
	int new_capacity = str->capacity == 0 ? 2 : str->capacity * 2;
	char* new_str = (char*)malloc(new_capacity);
	
	if (str->str == NULL)
		new_str[0] = 0;
	else
		strcpy(new_str, str->str);

	str->str = new_str;
	str->capacity = new_capacity;
}

void str_add(String* str, char c)
{
	if (str->size + 1 >= str->capacity)
		str_grow(str);

	str->str[str->size] = c;
	str->str[str->size + 1] = '\0';
	++str->size;
}

bool str_equals(String* str, char* other)
{
	return strcmp(str->str, other) == 0;
}


// Array helpers

typedef struct Array
{
	int size;
	int capacity;
	void** arr;
} Array;

void arr_grow(Array* arr)
{
	int new_capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
	void** new_arr = (void**)malloc(new_capacity * sizeof(void*));
	memcpy(new_arr, arr->arr, arr->size * sizeof(void*));
	arr->arr = new_arr;
	arr->capacity = new_capacity;
}

void arr_add(Array* arr, void* e)
{
	if (arr->size == arr->capacity)
		arr_grow(arr);

	arr->arr[arr->size] = e;
	++arr->size;
}


// Jzon implmenetation

void next(const char** input)
{
	++*input;
}

char current(const char** input)
{
	return **input;
}

void skip_whitespace(const char** input)
{
	while (current(input))
	{
		while (current(input) && (current(input) <= ' ' || current(input) == '\n' || current(input) == '\r'))
			next(input);
		
		// Skip comment.
		if (current(input) == '#')
		{
			while (current(input) && current(input) != '\n')
				next(input);
		}
		else
			break;
	}
};

char* parse_pure_string(const char** input)
{
	String str = {0};

	if (current(input) != '"')
		return NULL;

	next(input);

	while (current(input))
	{
		char ch = current(input);

		if (ch == '"')
		{
			next(input);
			return str.str;
		}

		str_add(&str, ch);
		next(input);
	}

	free(str.str);
	return NULL;
}

char* parse_keyname(const char** input)
{
	String name = {0};

	if (current(input) == '"')
		return parse_pure_string(input);

	while (current(input))
	{
		char ch = current(input);

		if (ch == ':')
			return name.str;
		else if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9')
			str_add(&name, ch);
		else
			return NULL;

		next(input);
	}

	return NULL;
}


char* parse_multiline_string(const char** input)
{
	return NULL;
}

int parse_value(const char** input, JzonValue* output);

int parse_string(const char** input, JzonValue* output)
{
	char* str = parse_pure_string(input);

	if (str == NULL)
		return -1;

	output->is_string = true;
	output->string_value = str;
	return 0;
}

int parse_array(const char** input, JzonValue* output)
{
	Array array_values = {0};
	
	if (current(input) != '[')
		return -1;
	
	output->is_array = true;
	next(input);

	// Empty array.
	if (current(input) == ']')
	{
		output->size = 0; 
		return 0;
	}

	while (current(input))
	{
		JzonValue* value = NULL;
		int error = 0;
		skip_whitespace(input);
		value = (JzonValue*)malloc(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		error = parse_value(input, value);

		if (error != 0)
			return error;

		arr_add(&array_values, value);
		skip_whitespace(input);

		if (current(input) == ',')
			next(input);
		
		skip_whitespace(input);

		if (current(input) == ']')
		{
			next(input);
			break;
		}
	}
	
	output->size = array_values.size; 
	output->array_values = (JzonValue**)array_values.arr;	
	return 0;
}

int parse_object(const char** input, JzonValue* output)
{
	Array object_values = {0};
	JzonKeyValuePair* pair = NULL;

	if (current(input) != '{')
		return -1;

	output->is_object = true;
	next(input);
	
	// Empty object.
	if (current(input) == '}')
	{
		output->size = 0; 
		return 0;
	}

	while (current(input))
	{
		char* key = NULL;
		JzonValue* value = NULL;
		int error = 0;
		pair = (JzonKeyValuePair*)malloc(sizeof(JzonKeyValuePair));
		skip_whitespace(input);
		key = parse_keyname(input);
		
		if (key == NULL)
			return -1;

		skip_whitespace(input);

		if (current(input) != ':')
			return -1;

		next(input);
		value = (JzonValue*)malloc(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		error = parse_value(input, value);

		if (error != 0)
			return error;

		pair->key = key;
		pair->value = value;
		arr_add(&object_values, pair);

		if (current(input) == ',')
			next(input);
		
		skip_whitespace(input);

		if (current(input) == '}')
		{
			next(input);
			break;
		}
	}
	
	output->size = object_values.size; 
	output->object_values = (JzonKeyValuePair**)object_values.arr;	
	return 0;
}

int parse_number(const char** input, JzonValue* output)
{
	String num = {0};
	bool is_float = false;

	if (current(input) == '-')
	{
		str_add(&num, current(input));
		next(input);
	}

	while (current(input) >= '0' && current(input) <= '9')
	{
		str_add(&num, current(input));
		next(input);
	}

	if (current(input) == '.')
	{
		is_float = true;
		str_add(&num, current(input));
		next(input);

		while (current(input) >= '0' && current(input) <= '9')
		{
			str_add(&num, current(input));
			next(input);
		}
	}

	if (current(input) == 'e' || current(input) == 'E')
	{
		str_add(&num, current(input));
		next(input);

		if (current(input) == '-' || current(input) == '+')
		{
			str_add(&num, current(input));
			next(input);
		}

		while (current(input) >= '0' && current(input) <= '9')
		{
			str_add(&num, current(input));
			next(input);
		}
	}

	if (is_float)
	{
		output->is_float = true;
		output->float_value = (float)atof(num.str);
	}
	else
	{
		output->is_int = true;
		output->int_value = atoi(num.str);
	}

	free(num.str);
	return 0;
}

int parse_word_or_string(const char** input, JzonValue* output)
{
	String str = {0};

	while (current(input))
	{
		char ch = current(input);
		
		if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '\'')
			str_add(&str, ch);
		else
		{
			if (str.size == 3 && str_equals(&str, "'''"))
			{
				output->string_value = parse_multiline_string(input);
				output->is_string = true;
			}
			else if (str.size == 4 && str_equals(&str, "true"))
			{
				output->is_bool = true;
				output->bool_value = true;
			}
			else if (str.size == 5 && str_equals(&str, "false"))
			{
				output->is_bool = true;
				output->bool_value = false;
			}
			else if (str.size == 4 && str_equals(&str, "null"))
			{
				output->is_null = true;
			}
			else
			{
				free(str.str);
				return -1;
			}

			break;
		}

		next(input);
	}	
	
	next(input);
	free(str.str);
	return 0;
}

int parse_value(const char** input, JzonValue* output)
{
	char ch;
	skip_whitespace(input);
	ch = current(input);

	switch (ch)
	{
		case '{': return parse_object(input, output);
		case '[': return parse_array(input, output);
		case '"': return parse_string(input, output);
		case '-': return parse_number(input, output);
		default: return ch >= '0' && ch <= '9' ? parse_number(input, output) : parse_word_or_string(input, output);
	}

	return -1;
}

JzonParseResult jzon_parse(const char* input)
{
	JzonValue output = {0};
	int error = parse_value(&input, &output);
	JzonParseResult result = {0};
	result.output = output;
	result.success = error == 0;
	return result;
}
