jzon-c
======

Like Hjson (http://laktak.github.io/hjson), but with optional root node and consistent use of double quotes (no single quotes for multiline strings). Only parser for the time being. Support for custom allocators is on the way.

Parts of the code is based on the JavaScript Hjson implementation.

This Hjson:

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

    haiku: '''
    JSON I love you.
    But you strangle my expression.
    This is so much better.
    '''

    favNumbers: [ 1, 2, 3, 6, 42 ]
}
```

Is equivalent to this Jzon:

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

haiku: """
JSON I love you.
But you strangle my expression.
This is so much better.
"""

favNumbers: [ 1, 2, 3, 6, 42 ]
```
