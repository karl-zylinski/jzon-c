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

void str_grow(String* str, JzonAllocator* allocator)
{
	int new_capacity = str->capacity == 0 ? 2 : str->capacity * 2;
	char* new_str = (char*)allocator->allocate(new_capacity);
	
	if (str->str == NULL)
		new_str[0] = 0;
	else
		strcpy(new_str, str->str);

	allocator->deallocate(str->str);
	str->str = new_str;
	str->capacity = new_capacity;
}

void str_add(String* str, char c, JzonAllocator* allocator)
{
	if (str->size + 1 >= str->capacity)
		str_grow(str, allocator);

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

void arr_grow(Array* arr, JzonAllocator* allocator)
{
	int new_capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
	void** new_arr = (void**)allocator->allocate(new_capacity * sizeof(void*));
	memcpy(new_arr, arr->arr, arr->size * sizeof(void*));
	allocator->deallocate(arr->arr);
	arr->arr = new_arr;
	arr->capacity = new_capacity;
}

void arr_add(Array* arr, void* e, JzonAllocator* allocator)
{
	if (arr->size == arr->capacity)
		arr_grow(arr, allocator);

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
		while (current(input) && (current(input) <= ' ' || current(input) == ','))
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

char* parse_multiline_string(const char** input, JzonAllocator* allocator)
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
				str_add(&str, '\n', allocator);
		}

		if (is_multiline_string_quotes(*input))
		{
			*input += 3;
			return str.str;
		}

		str_add(&str, current(input), allocator);
		next(input);
	}

	allocator->deallocate(str.str);
	return NULL;
}

char* parse_pure_string(const char** input, JzonAllocator* allocator)
{
	String str = {0};

	if (current(input) != '"')
		return NULL;

	if (is_multiline_string_quotes(*input))
		return parse_multiline_string(input, allocator);

	next(input);

	while (current(input))
	{
		char ch = current(input);
		
		if (ch == '"')
		{
			next(input);
			return str.str;
		}

		str_add(&str, ch, allocator);
		next(input);
	}

	allocator->deallocate(str.str);
	return NULL;
}

char* parse_keyname(const char** input, JzonAllocator* allocator)
{
	String name = {0};

	if (current(input) == '"')
		return parse_pure_string(input, allocator);

	while (current(input))
	{
		char ch = current(input);

		if (ch == ':')
			return name.str;
		else if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9')
			str_add(&name, ch, allocator);
		else
			return NULL;

		next(input);
	}

	return NULL;
}

int parse_value(const char** input, JzonValue* output, JzonAllocator* allocator);

int parse_string(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	char* str = parse_pure_string(input, allocator);

	if (str == NULL)
		return -1;

	output->is_string = true;
	output->string_value = str;
	return 0;
}

int parse_array(const char** input, JzonValue* output, JzonAllocator* allocator)
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
		value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		error = parse_value(input, value, allocator);

		if (error != 0)
			return error;

		arr_add(&array_values, value, allocator);
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

int parse_object(const char** input, JzonValue* output, bool root_object, JzonAllocator* allocator)
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
		pair = (JzonKeyValuePair*)allocator->allocate(sizeof(JzonKeyValuePair));
		skip_whitespace(input);
		key = parse_keyname(input, allocator);
		
		if (key == NULL || current(input) != ':')
			return -1;

		next(input);
		value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		error = parse_value(input, value, allocator);

		if (error != 0)
			return error;

		pair->key = key;
		pair->value = value;
		arr_add(&object_values, pair, allocator);
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

int parse_number(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	String num = {0};
	bool is_float = false;

	if (current(input) == '-')
	{
		str_add(&num, current(input), allocator);
		next(input);
	}

	while (current(input) >= '0' && current(input) <= '9')
	{
		str_add(&num, current(input), allocator);
		next(input);
	}

	if (current(input) == '.')
	{
		is_float = true;
		str_add(&num, current(input), allocator);
		next(input);

		while (current(input) >= '0' && current(input) <= '9')
		{
			str_add(&num, current(input), allocator);
			next(input);
		}
	}

	if (current(input) == 'e' || current(input) == 'E')
	{
		str_add(&num, current(input), allocator);
		next(input);

		if (current(input) == '-' || current(input) == '+')
		{
			str_add(&num, current(input), allocator);
			next(input);
		}

		while (current(input) >= '0' && current(input) <= '9')
		{
			str_add(&num, current(input), allocator);
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

	allocator->deallocate(num.str);
	return 0;
}

int parse_word_or_string(const char** input, JzonValue* output, JzonAllocator* allocator)
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
				allocator->deallocate(str.str);
				return 0;
			}
			else if (str.size == 5 && str_equals(&str, "false"))
			{
				output->is_bool = true;
				output->bool_value = false;
				allocator->deallocate(str.str);
				return 0;
			}
			else if (str.size == 4 && str_equals(&str, "null"))
			{
				output->is_null = true;
				allocator->deallocate(str.str);
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
			str_add(&str, ch, allocator);

		next(input);
	}

	allocator->deallocate(str.str);
	return -1;
}

int parse_value(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	char ch;
	skip_whitespace(input);
	ch = current(input);

	switch (ch)
	{
		case '{': return parse_object(input, output, false, allocator);
		case '[': return parse_array(input, output, allocator);
		case '"': return parse_string(input, output, allocator);
		case '-': return parse_number(input, output, allocator);
		default: return ch >= '0' && ch <= '9' ? parse_number(input, output, allocator) : parse_word_or_string(input, output, allocator);
	}

	return -1;
}

JzonParseResult jzon_parse(const char* input, JzonAllocator* allocator)
{
	JzonValue* output = (JzonValue*)allocator->allocate(sizeof(JzonValue));
	memset(output, 0, sizeof(JzonValue));
	int error = parse_object(&input, output, true, allocator);
	JzonParseResult result = {0};
	result.output = output;
	result.success = error == 0;
	return result;
}

JzonParseResult jzon_parse(const char* input)
{
	JzonAllocator allocator;
	allocator.allocate = malloc;
	allocator.deallocate = free;
	return jzon_parse(input, &allocator);
}

void jzon_free(JzonValue* value, JzonAllocator* allocator)
{
	unsigned i = 0;

	if (value->is_object)
	{
		for (i = 0; i < value->size; ++i)
		{
			allocator->deallocate(value->object_values[i]->key);
			jzon_free(value->object_values[i]->value, allocator);
		}

		allocator->deallocate(value->object_values);
	}
	else if (value->is_array)
	{
		for (i = 0; i < value->size; ++i)
			jzon_free(value->array_values[i], allocator);

		allocator->deallocate(value->array_values);
	}
	else if (value->is_string)
	{
		allocator->deallocate(value->string_value);
	}

	allocator->deallocate(value);
}

void jzon_free(JzonValue* value)
{
	JzonAllocator allocator;
	allocator.allocate = malloc;
	allocator.deallocate = free;
	jzon_free(value, &allocator);
}

JzonValue* jzon_get(JzonValue* value, const char* key)
{
	unsigned i;

	if (!value->is_object)
		return NULL;
		
	for (i = 0; i < value->size; ++i)
	{
		JzonKeyValuePair* pair = value->object_values[i];

		if (strcmp(pair->key, key) == 0)
			return value;
	}

	return NULL;
}
