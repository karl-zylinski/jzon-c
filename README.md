jzon-c
======

jzon is a format for describing data that is easier and less error-prone to edit than standard JSON.

It is based on Hjson (http://laktak.github.io/hjson) and SJSON (http://bitsquid.blogspot.se/2009/10/simplified-json-notation.html), which are both in turn based on JSON.

This is a C99 / C++ implementation. Only parser for the time being.

## jzon can look like this

```
key: "value"
something: quotes are not needed
"but you can still use them if want to": <-- he's drunk, don't listen to him

# These are really the best, I promise.
the_best_numbers: [ 2, 7, 12, 18 ]

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

- Quotes around keys and values are optional.
- Multi-line string support.
- Commas are optional.
- Explicit root node is optional.

## How does jzon differ from and relate to Hjson?

- Explicit root node is optional.
- Triple double quote is used for multi-line strings instead of triple single quote.
- Non-quoted object keys may contain any character except for colon.

Parts of the code is based on the JavaScript Hjson implementation. The multi-line string indentation rules aren't as sophisticated as in Hjson (yet).

## How does jzon differ from SJSON?

- String value quotes are optional.
- Stuck to the default JSON delimiter for key-value-pairs (:, SJSON uses =).

## Installation and usage

The easiest way is to just throw jzon.h and jzon.c into your project. Include jzon.h and then call jzon_parse(your_string_with_jzon) to have it parse your jzon.

## Dependencies

The code has no dependencies. It compiles as C++ and C99 (with the common anonymous union extension).

## Custom allocators

You can use custom allocators by, instead of using jzon_parse (which defaults to malloc / free), using jzon_parse_custom_allocator. As second argument this function takes a JzonAllocator struct which should contain a function pointer to an allocate and a deallocate function.

## Projects using jzon

- https://github.com/KarlZylinski/Raket — This game uses an engine which defines materials, fonts, entities etc using jzon.
