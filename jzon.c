#include "jzon.h"
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

__forceinline void next(const char** input)
{
	++*input;
}

__forceinline char current(const char** input)
{
	return **input;
}

bool is_multiline_string_quotes(const char* str)
{
	return *str == '"' && *(str + 1) == '"' && *(str + 1) == '"';
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

char* parse_multiline_string(const char** input)
{
	String str = {0};
	bool clean_whitespace = false;

	if (!is_multiline_string_quotes(*input))
		return NULL;
	
	*input += 3;

	while (current(input))
	{
		if (current(input) == '\n' || current(input) == '\r')
		{
			skip_whitespace(input);

			if (str.size > 0)
				str_add(&str, '\n');
		}

		if (is_multiline_string_quotes(*input))
		{
			*input += 3;
			return str.str;
		}

		str_add(&str, current(input));
		next(input);
	}

	free(str.str);
	return NULL;
}

char* parse_pure_string(const char** input)
{
	String str = {0};

	if (current(input) != '"')
		return NULL;

	if (is_multiline_string_quotes(*input))
		return parse_multiline_string(input);

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

int parse_object(const char** input, JzonValue* output, bool root_object)
{
	Array object_values = {0};
	JzonKeyValuePair* pair = NULL;

	if (current(input) == '{')
		next(input);
	else if (!root_object)
		return -1;

	output->is_object = true;
	
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

		if (ch == '\r' || ch == '\n')
		{
			if (str.size == 4 && str_equals(&str, "true"))
			{
				output->is_bool = true;
				output->bool_value = true;
				free(str.str);
				return 0;
			}
			else if (str.size == 5 && str_equals(&str, "false"))
			{
				output->is_bool = true;
				output->bool_value = false;
				free(str.str);
				return 0;
			}
			else if (str.size == 4 && str_equals(&str, "null"))
			{
				output->is_null = true;
				free(str.str);
				return 0;
			}
			else
			{
				output->is_string = true;
				output->string_value = str.str;
				return 0;
			}

			break;
		}		
		else
			str_add(&str, ch);

		next(input);
	}

	free(str.str);
	return -1;
}

int parse_value(const char** input, JzonValue* output)
{
	char ch;
	skip_whitespace(input);
	ch = current(input);

	switch (ch)
	{
		case '{': return parse_object(input, output, false);
		case '[': return parse_array(input, output);
		case '"': return parse_string(input, output);
		case '-': return parse_number(input, output);
		default: return ch >= '0' && ch <= '9' ? parse_number(input, output) : parse_word_or_string(input, output);
	}

	return -1;
}

JzonParseResult jzon_parse(const char* input)
{
	JzonValue* output = (JzonValue*)malloc(sizeof(JzonValue));
	memset(output, 0, sizeof(JzonValue));
	int error = parse_object(&input, output, true);
	JzonParseResult result = {0};
	result.output = output;
	result.success = error == 0;
	return result;
}

void jzon_free(JzonValue* value)
{
	unsigned i = 0;

	if (value->is_object)
	{
		for (i = 0; i < value->size; ++i)
		{
			free(value->object_values[i]->key);
			jzon_free(value->object_values[i]->value);
		}

		free(value->object_values);
	}
	else if (value->is_array)
	{
		for (i = 0; i < value->size; ++i)
			jzon_free(value->array_values[i]);

		free(value->array_values);
	}
	else if (value->is_string)
	{
		free(value->string_value);
	}

	free(value);
}
