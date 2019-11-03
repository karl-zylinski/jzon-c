jzon-c
======

jzon is a format for describing data that is easier and less error-prone to edit than standard JSON.

It is based on Hjson (http://laktak.github.io/hjson) and SJSON (http://bitsquid.blogspot.se/2009/10/simplified-json-notation.html), which are both in turn based on JSON.

It is written in C11. It is only a parser.

## jzon can look like this

```
key: "value"
quotes are not needed for keys: "very nice"
"but you can still use them if want to": "<-- he's drunk, don't listen to him"

# These are really the best, I promise.
the_best_numbers: [ 2, 7, 12, 18 ]
the_truth: true

instructions: """
    Take one liter of flour.
    Eat an elevator.
    Do a backflip.
    Enjoy.
"""

mysterious_words_by_id: {
    190ca652-c561-4207-9bdf-d92ff5d01efa: "uggla"
    5c05febd-192c-4a0a-872b-0b4def2d67a1: "spade"
    590a13cd-7ef7-41cc-90e0-f2e0ee8c54f3: "termoskvarn"
}
```


## How does jzon differ from JSON?

- Quotes around keys are optional.
- Multi-line string support.
- Commas are optional.
- Explicit root node is optional.

## How does jzon differ from and relate to Hjson?

- Explicit root node is optional.
- Quotes around values are not optional.
- Triple double quote is used for multi-line strings instead of triple single quote.
- Non-quoted object keys may contain any character except for colon.

## How does jzon differ from SJSON?

- Stuck to the default JSON delimiter for key-value-pairs (:, SJSON uses =).

## Installation and usage

The easiest way is to just throw jzon.h and jzon.c into your project. Include jzon.h and then call jzon_parse(your_string_with_jzon) to have it parse your jzon.

## Dependencies

The code has no dependencies. It compiles as C11 and should be usable within a C++ project. It may use some extensions that my give warnings.

## Custom allocators

You can use custom allocators by instead of using jzon_parse (which defaults to malloc / free / realloc), using jzon_parse_custom_allocator. As second argument this function takes a JzonAllocator struct which should contain a function pointer to an allocate and a deallocate function.
