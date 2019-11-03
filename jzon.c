#include "jzon.h"
#include <stdlib.h>
#include <string.h>


// String helpers

static char* copy_str(const char* str, unsigned len, JzonAllocator* allocator)
{
    unsigned size = len + 1;
    char* new_str = (char*)allocator->allocate(size);
    memcpy(new_str, str, len);
    new_str[len] = '\0';
    return new_str;
}

static bool is_str(const char* input, char* str)
{
    for (uint32_t i = 0; i < strlen(str); ++i)
    {
        if (input[i] == 0)
            return false;

        if (input[i] != str[i])
            return false;
    }

    return true;
}

static bool str_equals(char* str, char* other)
{
    return strcmp(str, other) == 0;
}

static char* concat_str(JzonAllocator* allocator, const char* str1, unsigned str1_len, const char* str2, unsigned str2_len)
{
    unsigned size = str1_len + str2_len;
    char* new_str = (char*)allocator->allocate(size + 1);
    memcpy(new_str, str1, str1_len);
    memcpy(new_str + str1_len, str2, str2_len);
    new_str[size] = '\0';
    return new_str;
}


// Hash function used for hashing object keys.
// From http://murmurhash.googlepages.com/

static uint64_t hash_str(const char* str)
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
        uint64_t k = *data++;

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


// Jzon implementation

static void next(const char** input)
{
    ++*input;
}

static char current(const char** input)
{
    return **input;
}

static bool is_multiline_string_quotes(const char* str)
{
    return *str == '"' && *(str + 1) == '"' && *(str + 1) == '"';
}

static uint32_t find_pair_insertion_index(JzonKeyValuePair* objects, uint32_t size, uint64_t key_hash)
{
    if (size == 0)
        return 0;

    for (uint32_t i = 0; i < size; ++i)
    {
        if (objects[i].key_hash > key_hash)
            return i;
    }

    return size;
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_whitespace(const char** input)
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

static char* parse_multiline_string(const char** input, JzonAllocator* allocator)
{
    if (!is_multiline_string_quotes(*input))
        return NULL;
    
    *input += 3;
    char* start = (char*)*input;
    char* result = "";

    while (current(input))
    {
        if (current(input) == '\n' || current(input) == '\r')
        {
            unsigned result_len = (unsigned)strlen(result);
            unsigned line_len = (unsigned)(*input - start);

            if (result_len > 0) {
                char* new_result = concat_str(allocator, result, result_len, "\n", 1);
                allocator->deallocate(result);
                result = new_result;
                ++result_len;
            }

            skip_whitespace(input);

            if (line_len != 0)
            {
                char* new_result = concat_str(allocator, result, result_len, start, line_len);

                if (result_len > 0)
                    allocator->deallocate(result);

                result = new_result;
            }

            start = (char*)*input;
        }

        if (is_multiline_string_quotes(*input))
        {
            unsigned result_len = (unsigned)strlen(result);
            char* new_result = concat_str(allocator, result, result_len, start, (unsigned)(*input - start));
            allocator->deallocate(result);
            result = new_result;
            *input += 3;
            return result;
        }

        next(input);
    }

    allocator->deallocate(result);
    return NULL;
}

static char* parse_string_internal(const char** input, JzonAllocator* allocator)
{
    if (current(input) != '"')
        return NULL;

    if (is_multiline_string_quotes(*input))
        return parse_multiline_string(input, allocator);

    next(input);
    char* start = (char*)*input;

    while (current(input))
    {
        if (current(input) == '"')
        {
            char* end = (char*)*input;
            next(input);
            return copy_str(start, (unsigned)(end - start), allocator);
            break;
        }

        next(input);
    }

    return NULL;
}

static char* parse_keyname(const char** input, JzonAllocator* allocator)
{
    if (current(input) == '"')
        return parse_string_internal(input, allocator);

    char* start = (char*)*input;

    while (current(input))
    {
        const char* cur_wo_whitespace = *input;
        if (is_whitespace(current(input)))
            skip_whitespace(input);

        if (current(input) == ':')
            return copy_str(start, (unsigned)(cur_wo_whitespace - start), allocator);

        next(input);
    }

    return NULL;
}

static bool parse_value(const char** input, JzonValue* output, JzonAllocator* allocator);

static bool parse_string(const char** input, JzonValue* output, JzonAllocator* allocator)
{
    char* str = parse_string_internal(input, allocator);

    if (str == NULL)
        return false;

    output->is_string = true;
    output->string_val = str;
    return true;
}

static bool parse_array(const char** input, JzonValue* output, JzonAllocator* allocator)
{   
    if (current(input) != '[')
        return false;
    
    output->is_array = true;
    next(input);

    // Empty array.
    if (current(input) == ']')
    {
        output->size = 0; 
        return true;
    }

    JzonValue* array = allocator->allocate(sizeof(JzonValue) * 1);
    uint32_t array_cap = 1;
    uint32_t array_num = 0;

    while (current(input))
    {
        skip_whitespace(input);

        if (array_num == array_cap)
        {
            array_cap = array_cap * 2;
            array = allocator->reallocate(array, sizeof(JzonValue) * array_cap);
        }

        JzonValue* val = array + (array_num++);
        memset(val, 0, sizeof(JzonValue));
        if (parse_value(input, val, allocator) == false)
            return false;

        skip_whitespace(input);

        if (current(input) == ']')
        {
            next(input);
            break;
        }
    }
    
    if (array_cap != array_num)
        array = allocator->reallocate(array, sizeof(JzonValue) * array_num);

    output->size = array_num; 
    output->array_val = array;   
    return true;
}

static bool parse_table(const char** input, JzonValue* output, bool root_object, JzonAllocator* allocator)
{
    if (current(input) == '{')
        next(input);
    else if (!root_object)
        return false;

    output->is_table = true;
    skip_whitespace(input);

    // Empty object.
    if (current(input) == '}')
    {
        output->size = 0;
        return true;
    }

    const char* counter_input = *input;
    uint32_t table_num_max = 0; 
    while (*counter_input)
    {
        if(*counter_input == ':') // there may be ":" in keys or values, so we might get some unused slots
            ++table_num_max;

        if (*counter_input == '}')
            break;

        ++counter_input;
    }

    JzonKeyValuePair* table = allocator->allocate(sizeof(JzonKeyValuePair) * table_num_max);
    uint32_t table_num = 0;
    while (current(input))
    {
        skip_whitespace(input);
        char* key = parse_keyname(input, allocator);
        skip_whitespace(input);

        if (key == NULL || current(input) != ':')
            return false;

        next(input);

        JzonValue value = {0};

        if (!parse_value(input, &value, allocator))
            return false;

        JzonKeyValuePair pair = {0};
        pair.key = key;
        pair.key_hash = hash_str(key);
        pair.val = value;

        // we want it sorted for binary search
        uint32_t insertion_idx = find_pair_insertion_index(table, table_num, pair.key_hash);
        memmove(table + insertion_idx + 1, table + insertion_idx, (table_num - insertion_idx) * sizeof(JzonKeyValuePair));
        table[insertion_idx] = pair;
        ++table_num;

        skip_whitespace(input);

        if (current(input) == '}')
        {
            next(input);
            break;
        }
    }

    if (table_num_max > table_num)
        table = allocator->reallocate(table, table_num * sizeof(JzonKeyValuePair));

    output->size = table_num;
    output->table_val = table;
    return true;
}

static bool parse_number(const char** input, JzonValue* output)
{
    bool is_float = false;
    char* start = (char*)*input;

    if (current(input) == '-')
        next(input);

    while (current(input) >= '0' && current(input) <= '9')
        next(input);

    if (current(input) == '.')
    {
        is_float = true;
        next(input);

        while (current(input) >= '0' && current(input) <= '9')
            next(input);
    }

    if (current(input) == 'e' || current(input) == 'E')
    {
        is_float = true;
        next(input);

        if (current(input) == '-' || current(input) == '+')
            next(input);

        while (current(input) >= '0' && current(input) <= '9')
            next(input);
    }

    if (is_float)
    {
        output->is_float = true;
        output->float_val = (float)strtod(start, NULL);
    }
    else
    {
        output->is_int = true;
        output->int_val = (int)strtol(start, NULL, 10);
    }

    return true;
}

static bool parse_true(const char** input, JzonValue* output)
{
    if (is_str(*input, "true"))
    {
        output->is_bool = true;
        output->bool_val = true;
        *input += 4;
        return true;
    }

    return false;
}

static bool parse_false(const char** input, JzonValue* output)
{
    if (is_str(*input, "false"))
    {
        output->is_bool = true;
        output->bool_val = false;
        *input += 5;
        return true;
    }

    return false;
}

static bool parse_null(const char** input, JzonValue* output)
{
    if (is_str(*input, "null"))
    {
        output->is_null = true;
        *input += 4;
        return true;
    }

    return false;
}

static bool parse_value(const char** input, JzonValue* output, JzonAllocator* allocator)
{
    skip_whitespace(input);
    char ch = current(input);

    switch (ch)
    {
        case '{': return parse_table(input, output, false, allocator);
        case '[': return parse_array(input, output, allocator);
        case '"': return parse_string(input, output, allocator);
        case '-': return parse_number(input, output);
        case 'f': return parse_false(input, output);
        case 't': return parse_true(input, output);
        case 'n': return parse_null(input, output);
        default: return ch >= '0' && ch <= '9' ? parse_number(input, output) : false;
    }
}


// Public interface

JzonParseResult jzon_parse_custom_allocator(const char* input, JzonAllocator* allocator)
{
    JzonValue output = {0};
    bool ok = parse_table(&input, &output, true, allocator);
    return (JzonParseResult) {
        .ok = ok,
        .output = output
    };
}

JzonParseResult jzon_parse(const char* input)
{
    JzonAllocator allocator = { malloc, free, realloc };
    return jzon_parse_custom_allocator(input, &allocator);
}

void jzon_free_custom_allocator(JzonValue* value, JzonAllocator* allocator)
{
    if (value->is_table)
    {
        for (uint32_t i = 0; i < value->size; ++i)
        {
            allocator->deallocate(value->table_val[i].key);
            jzon_free_custom_allocator(&value->table_val[i].val, allocator);
        }

        allocator->deallocate(value->table_val);
    }
    else if (value->is_array)
    {
        for (uint32_t i = 0; i < value->size; ++i)
            jzon_free_custom_allocator(value->array_val + i, allocator);

        allocator->deallocate(value->array_val);
    }
    else if (value->is_string)
    {
        allocator->deallocate(value->string_val);
    }
}

void jzon_free(JzonValue* value)
{
    JzonAllocator allocator = { malloc, free, realloc };
    jzon_free_custom_allocator(value, &allocator);
}

JzonValue* jzon_get(JzonValue* object, const char* key)
{
    if (!object->is_table)
        return NULL;

    if (object->size == 0)
        return NULL;
    
    uint64_t key_hash = hash_str(key);

    unsigned first = 0;
    unsigned last = object->size - 1;
    unsigned middle = (first + last) / 2;

    while (first <= last)
    {
        if (object->table_val[middle].key_hash < key_hash)
            first = middle + 1;
        else if (object->table_val[middle].key_hash == key_hash)
            return &object->table_val[middle].val;
        else
            last = middle - 1;

        middle = (first + last) / 2;
    }

    return NULL;
}
