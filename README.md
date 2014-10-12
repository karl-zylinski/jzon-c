jzon-c
======

jzon is a format for describing data. It is based on Hjson (http://laktak.github.io/hjson) and SJSON (http://bitsquid.blogspot.se/2009/10/simplified-json-notation.html)

This is a C99 / C++ implementation. Only parser for the time being.

## How does it differ from JSON?

- Quotes around keys and values are optional.
- Multi-line string support.
- Commas are optional.
- Explicit root node is optional.

## How does jzon differ from and relate to Hjson?

- Explicit root node is optional.
- Triple double quote is used for multi-line strings instead of triple single quote.
- Non-quoted object keys may contain any character except for colon.

## How does jzon differ from and relate to Hjson?

- String value quotes are optional

## Installation and usage

The easiest way is to just throw jzon.h and jzon.c into your project. Include jzon.h and then call jzon_parse(your_string_with_jzon) to have it parse your jzon.

## Dependencies

The code has no dependencies. It compiles as C++ and C99 (with the common anonymous union extension).

## Custom allocators

You can use custom allocators by, instead of using jzon_parse (which defaults to malloc / free), using jzon_parse_custom_allocator. As second argument this function takes a JzonAllocator struct which should contain a function pointer to an allocate and a deallocate function.
