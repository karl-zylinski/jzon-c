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
	
	if (str->str != NULL)
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


// Hash function used for hashing object keys.
// From http://murmurhash.googlepages.com/

uint64_t hash_str(const char* str)
{
	size_t len = strlen(str);
	uint64_t seed = 0;

	const uint64_t m = 0xc6a4a7935bd1e995ULL;
	const uint32_t r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)str;
	const uint64_t * end = data + (len / 8);

	while (data != end)
	{
#ifdef PLATFORM_BIG_ENDIAN
		uint64_t k = *data++;
		char *p = (char *)&k;
		char c;
		c = p[0]; p[0] = p[7]; p[7] = c;
		c = p[1]; p[1] = p[6]; p[6] = c;
		c = p[2]; p[2] = p[5]; p[5] = c;
		c = p[3]; p[3] = p[4]; p[4] = c;
#else
		uint64_t k = *data++;
#endif

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch (len & 7)
	{
	case 7: h ^= ((uint64_t)data2[6]) << 48;
	case 6: h ^= ((uint64_t)data2[5]) << 40;
	case 5: h ^= ((uint64_t)data2[4]) << 32;
	case 4: h ^= ((uint64_t)data2[3]) << 24;
	case 3: h ^= ((uint64_t)data2[2]) << 16;
	case 2: h ^= ((uint64_t)data2[1]) << 8;
	case 1: h ^= ((uint64_t)data2[0]);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
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
	if (!is_multiline_string_quotes(*input))
		return NULL;
	
	*input += 3;
	String str = { 0 };

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

char* parse_string_internal(const char** input, JzonAllocator* allocator)
{
	if (current(input) != '"')
		return NULL;

	if (is_multiline_string_quotes(*input))
		return parse_multiline_string(input, allocator);

	next(input);
	String str = { 0 };

	while (current(input))
	{
		if (current(input) == '"')
		{
			next(input);
			return str.str;
		}

		str_add(&str, current(input), allocator);
		next(input);
	}

	allocator->deallocate(str.str);
	return NULL;
}

char* parse_keyname(const char** input, JzonAllocator* allocator)
{
	if (current(input) == '"')
		return parse_string_internal(input, allocator);

	String name = { 0 };

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
	char* str = parse_string_internal(input, allocator);

	if (str == NULL)
		return -1;

	output->is_string = true;
	output->string_value = str;
	return 0;
}

int parse_array(const char** input, JzonValue* output, JzonAllocator* allocator)
{	
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

	Array array_values = { 0 };

	while (current(input))
	{
		skip_whitespace(input);
		JzonValue* value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		int error = parse_value(input, value, allocator);

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

	Array object_values = { 0 };

	while (current(input))
	{
		JzonKeyValuePair* pair = (JzonKeyValuePair*)allocator->allocate(sizeof(JzonKeyValuePair));
		skip_whitespace(input);
		char* key = parse_keyname(input, allocator);
		skip_whitespace(input);
		
		if (key == NULL || current(input) != ':')
			return -1;

		next(input);
		JzonValue* value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		int error = parse_value(input, value, allocator);

		if (error != 0)
			return error;

		pair->key = key;
		pair->key_hash = hash_str(key);
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
		is_float = true;
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
		if (current(input) == '\r' || current(input) == '\n')
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
			str_add(&str, current(input), allocator);

		next(input);
	}

	allocator->deallocate(str.str);
	return -1;
}

int parse_value(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	skip_whitespace(input);
	char ch = current(input);

	switch (ch)
	{
		case '{': return parse_object(input, output, false, allocator);
		case '[': return parse_array(input, output, allocator);
		case '"': return parse_string(input, output, allocator);
		case '-': return parse_number(input, output, allocator);
		default: return ch >= '0' && ch <= '9' ? parse_number(input, output, allocator) : parse_word_or_string(input, output, allocator);
	}
}


// Public interface

JzonParseResult jzon_parse_custom_allocator(const char* input, JzonAllocator* allocator)
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
	JzonAllocator allocator = { malloc, free };
	return jzon_parse_custom_allocator(input, &allocator);
}

void jzon_free_custom_allocator(JzonValue* value, JzonAllocator* allocator)
{
	if (value->is_object)
	{
		for (unsigned i = 0; i < value->size; ++i)
		{
			allocator->deallocate(value->object_values[i]->key);
			jzon_free_custom_allocator(value->object_values[i]->value, allocator);
		}

		allocator->deallocate(value->object_values);
	}
	else if (value->is_array)
	{
		for (unsigned i = 0; i < value->size; ++i)
			jzon_free_custom_allocator(value->array_values[i], allocator);

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
	JzonAllocator allocator = { malloc, free };
	jzon_free_custom_allocator(value, &allocator);
}

JzonValue* jzon_get(JzonValue* object, const char* key)
{
	if (!object->is_object)
		return NULL;
	
	uint64_t key_hash = hash_str(key);

	for (unsigned i = 0; i < object->size; ++i)
	{
		JzonKeyValuePair* pair = object->object_values[i];

		if (pair->key_hash == key_hash)
			return pair->value;
	}

	return NULL;
}
