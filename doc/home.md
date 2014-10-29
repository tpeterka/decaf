# Decaf Documentation

Documentation will use the [Markdown](http://daringfireball.net/projects/markdown/) syntax. The core of decaf is written in C++. Individual lower-level libraries are in either C or C++. Optionally, we may provide a C-style library of API wrapper functions for users who do not wish to use the C++ interface (TBD). For C++ users, all that is needed to build with decaf is to simply include its top-level header file:

```
#include <decaf/decaf.h>
```
No library is needed; all functions are contained in header files only.

# TOC

## [Class Organization](classes.md)

## [Transport Layers](transport.md)

## [Communicators](comm.md)

## [Selection and aggregation](select.md)

## [Data model](data.md)

## [Error Handling](errors.md)

## [Examples](examples.md)
