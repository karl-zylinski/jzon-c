jzon-c
======

Like hjson (http://laktak.github.io/hjson), but with optional root node and consistent use of double quotes (no single quotes for multiline strings). Only parser for the time being. Support for custom allocators is on the way.

The code has no dependencies and compiles as C++ and C99 (with the common anonymous union extension).

Parts of the code is based on the JavaScript hjson implementation.

Example on how jzon differs from hjson. This hjson:

```
{
    "rate": 1000 
    key: "value"
    text: look ma, no quotes!

    commas:
    {
        one: 1
        two: 2
    }

    trailing:
    {
        one: 1,
        two: 2,
    }

    haiku:
    '''
    JSON I love you.
    But you strangle my expression.
    This is so much better.
    '''

    favNumbers: [ 1, 2, 3, 6, 42 ]
}
```

Is equivalent to this jzon:

```
"rate": 1000 
key: "value"
text: look ma, no quotes!

commas:
{
    one: 1
    two: 2
}

trailing:
{
    one: 1,
    two: 2,
}

haiku:
"""
JSON I love you.
But you strangle my expression.
This is so much better.
"""

favNumbers: [ 1, 2, 3, 6, 42 ]
```
