#ifndef _JZON_H_
#define _JZON_H_

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
	uint64_t key_hash;
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

// Parse using default malloc allocator.
JzonParseResult jzon_parse(const char* input);

// Parse using custom allocator. Make sure to call jzon_free_custom_allocator using the same allocator.
JzonParseResult jzon_parse_custom_allocator(const char* input, JzonAllocator* allocator);

// Free parse result data structure using default free deallocator.
void jzon_free(JzonValue* value);

// Free parse result data structure which was parsed using custom allocator. Make sure to pass the same allocator you did to jzon_parse_custom_allocator.
void jzon_free_custom_allocator(JzonValue* value, JzonAllocator* allocator);

// Get value from object using key. Returns NULL if object is not an actual jzon object or there exists no value with the specified key.
JzonValue* jzon_get(JzonValue* object, const char* key);

#ifdef __cplusplus
}
#endif

#endif
