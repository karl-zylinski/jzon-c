#ifndef _JZON_H_
#define _JZON_H_

typedef int bool;
#define true 1
#define false 0

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
	int size;

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
	JzonValue output;
} JzonParseResult;


JzonParseResult jzon_parse(const char* input);

#endif
