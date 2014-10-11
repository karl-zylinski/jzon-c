#ifndef _JZON_H_
#define _JZON_H_

struct JzonKeyValuePair;
typedef struct JzonKeyValuePair JzonKeyValuePair;

typedef struct JzonValue
{
	bool is_string;
	bool is_int;
	bool is_float;
	bool is_object;
	bool is_array;
	bool is_bool;
	bool is_null;
	unsigned size;

	union
	{
		char* string_value;
		int int_value;
		bool bool_value;
		float float_value;
		struct JzonKeyValuePair** object_values;
		struct JzonValue** array_values;
	};
} JzonValue;

struct JzonKeyValuePair {
	char* key;
	JzonValue* value;
};

typedef struct JzonParseResult {
	bool success;
	JzonValue* output;
} JzonParseResult;

typedef void* (*jzon_allocate)(size_t);
typedef void (*jzon_deallocate)(void*);

typedef struct JzonAllocator {
	jzon_allocate allocate;
	jzon_deallocate deallocate;
} JzonAllocator;

JzonParseResult jzon_parse(const char* input, JzonAllocator* allocator);
// Uses standard malloc allocation.
JzonParseResult jzon_parse(const char* input);
void jzon_free(JzonValue* value, JzonAllocator* allocator);
// Uses standard free deallocation.
void jzon_free(JzonValue* value);
JzonValue* jzon_get(JzonValue* value, const char* key);

#endif
