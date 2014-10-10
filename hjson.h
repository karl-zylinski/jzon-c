#ifndef _HJSON_H_
#define _HJSON_H_

typedef int bool;
#define true 1
#define false 0

struct HJsonKeyValuePair;
typedef struct HJsonKeyValuePair HJsonKeyValuePair;

typedef struct HJsonValue
{
	bool is_string;
	bool is_int;
	bool is_float;
	bool is_object;
	bool is_array;
	bool is_bool;
	bool is_null;
	int size;

	union
	{
		char* string_value;
		int int_value;
		bool bool_value;
		float float_value;
		struct HJsonKeyValuePair** object_values;
		struct HJsonValue** array_values;
	};
} HJsonValue;

struct HJsonKeyValuePair {
	char* key;
	HJsonValue* value;
};

int hjson_parse(const char* input, HJsonValue* output);

#endif
