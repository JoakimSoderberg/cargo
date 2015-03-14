
API reference
=============

Default defines
---------------

# `CARGO_CONFIG`

# `CARGO_NAME_COUNT`

# `CARGO_DEFAULT_PREFIX`

# `CARGO_DEFAULT_MAX_OPTS`

# `CARGO_DEFAULT_MAX_WIDTH`

# `CARGO_AUTO_MAX_WIDTH`

# `CARGO_MAX_MAX_WIDTH`

Cargo version
-------------

# `CARGO_MAJOR_VERSION`

The major version of cargo. Example: **1**.2.3

# `CARGO_MINOR_VERSION`

The minor version of cargo. Example: 1.**2**.3

# `CARGO_PATCH_VERSION`

The patch version of cargo. Example: 1.2.**3**

# `CARGO_RELEASE`

If this is a release build, this will be set to `0`. Otherwise it is a
development build.

# `CARGO_VERSION`

A hexidecimal representation of the cargo version, as a 4-byte integer in the
form `(MAJOR << 24) | (MINOR << 16) | (PATCH << 8)`.

The last byte signifies the release status. `0` means release, and non-zero is a
developer release after a given release.

So fo example cargo version `"1.2.3"` will have a version value of
`0x01020300`. And  `"1.2.3-alpha"` could be `0x01020304`.

This is useful for doing compile time checks that you have a new
enough version for instance.

```c
#include <cargo.h>

#if !defined(CARGO_VERSION) || (CARGO_VERSION < 0x01020300)
#error "cargo version too old, get 1.2.3 or later"
#endif
```

And check for specific capabilities based on version:

```c
...

#if (CARGO_VERSION >= 0x01020300)
	cargo_new_fangled_feature(cargo);
#endif
...
```

# `CARGO_VERSION_STR`

The cargo version as a string. Example: `"0.2.0"`.

# `const char *cargo_get_version()`

Function that gets the cargo version as a string. Example: `"0.2.0"`.


