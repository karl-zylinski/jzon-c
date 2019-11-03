#include "jzon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pretty_print(int depth, JzonValue* value)
{
    char prefix[100];
    char new_prefix[100];   
    unsigned i = 0;

    memset(prefix, ' ', depth * 2);
    prefix[depth * 2] = '\0';
    memset(new_prefix, ' ', (depth + 1) * 2);
    new_prefix[(depth + 1) * 2] = '\0';

    if (value->is_table)
    {
        printf("\n");
        printf(prefix);
        printf("{");
        printf("\n");
        for (i = 0; i < value->size; ++i)
        {
            printf(new_prefix);
            printf(value->table_val[i].key);
            pretty_print(depth + 1, &value->table_val[i].val);
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
            pretty_print(depth + 1, value->array_val + i);
            printf("\n");
        }
        
        printf(prefix);
        printf("]");
    }
    else if (value->is_bool)
    {
        printf(prefix);

        if (value->bool_val)
            printf("true");
        else
            printf("false");
    }
    else if (value->is_float)
    {
        printf(prefix);
        printf("%f", value->float_val);
    }
    else if (value->is_int)
    {
        printf(prefix);
        printf("%i", value->int_val);
    }
    else if (value->is_null)
    {
        printf(prefix);
        printf("null");
    }
    else if (value->is_string)
    {
        printf(prefix);
        printf(value->string_val);
    }
}

static uint32_t num_allocs = 0;

void* test_alloc(size_t s)
{
    void* p = malloc(s);
    if (p)
        ++num_allocs;
    return p;
}

void test_dealloc(void* p)
{
    if (p)
        --num_allocs;
    free(p);
}

void* test_realloc(void* p, size_t s)
{
    void* np = realloc(p, s);
    if (np && !p)
        ++num_allocs;
    return np;
}

int main()
{
    // Open file
    FILE* fp = fopen("test.jzon", "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = (char*)malloc(filesize);
    assert(data);
    fread(data, 1, filesize, fp);
    data[filesize] = 0;
    fclose(fp);

    // Parse
    JzonAllocator allocator = { test_alloc, test_dealloc, test_realloc };
    JzonParseResult result = jzon_parse_custom_allocator(data, &allocator);
    free(data);

    // Run tests
    assert(result.ok);
    JzonValue* nested_table_val = jzon_get(&result.output, "nested_table");
    assert(nested_table_val->is_table);
    JzonValue* things_arr = jzon_get(nested_table_val, "things");
    assert(things_arr->is_array);
    assert(strcmp(jzon_get(things_arr->array_val + 0, "val")->string_val, "test0_value") == 0);
    assert(strcmp(jzon_get(things_arr->array_val + 1, "val")->string_val, "test1_value") == 0);
    JzonValue* mystery_words = jzon_get(&result.output, "mysterious_words_by_id");
    assert(mystery_words->is_table);
    assert(strcmp(jzon_get(mystery_words, "7b09c1a7-01bf-45c0-be19-753e1faecdde")->string_val, "hello") == 0);

    // Print parsed content as a tree
    pretty_print(0, &result.output);

    jzon_free_custom_allocator(&result.output, &allocator);
    assert(num_allocs == 0);
    return 0;
}
