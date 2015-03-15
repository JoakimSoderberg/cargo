Table of contents
=================

[TOC]

API reference
=============

## Default defines ##

### `CARGO_CONFIG` ###

If this is set, `cargo.h` will `#include cargo_config.h` where you can
override any of the default macros below. So that you can leave
`cargo.h` untouched.

### `CARGO_NAME_COUNT` ###

The max number of names allowed for an option. Defines how many
names and aliases any given option is allowed to have.

### `CARGO_DEFAULT_PREFIX` ###

This defines the default prefix characters for options. Note that this can also
be changed using [`cargo_set_prefix`](api.md#cargo_set_prefix).

### `CARGO_DEFAULT_MAX_OPTS` ###

The default start value for the max number of options. If more
options are added, the list of options will be reallocated.

This can also be changed using the function
[`cargo_set_option_count_hint`](api.md#cargo_set_option_count_hint)

### `CARGO_DEFAULT_MAX_WIDTH` ###

The default max width for the usage output if we cannot get the console width
from the operating system. By default the max width is set to
[`CARGO_AUTO_MAX_WIDTH`](api.md#CARGO_AUTO_MAX_WIDTH).

This is used by [`cargo_set_max_width`](api.md#cargo_set_max_width).

### `CARGO_AUTO_MAX_WIDTH` ###

Sets the max width for the usage output to the console width.

### `CARGO_MAX_MAX_WIDTH`

The absolute max console width allowed, any value set via
This is used by [`cargo_set_max_width`](api.md#cargo_set_max_width), will be
capped to this.

Cargo version
-------------

### `CARGO_MAJOR_VERSION` ###

The major version of cargo. Example: **1**.2.3

### `CARGO_MINOR_VERSION` ###

The minor version of cargo. Example: 1.**2**.3

### `CARGO_PATCH_VERSION` ###

The patch version of cargo. Example: 1.2.**3**

### `CARGO_RELEASE` ###

If this is a release build, this will be set to `0`. Otherwise it is a
development build.

### `CARGO_VERSION` ###

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

### `CARGO_VERSION_STR` ###

The cargo version as a string. Example: `"0.2.0"`.

### cargo_get_version ###

```c
const char *cargo_get_version()
```
 
Function that gets the cargo version as a string. Example: `"0.2.0"`.

## Function pointers ##

### cargo_custom_cb_t ###

```c
typedef int (*cargo_custom_cb_t)(cargo_t ctx, void *user, const char *optname,
                                 int argc, char **argv);
```

This is the callback function for doing custom parsing as specified when using
[`cargo_add_option`](api.md#cargo_add_option) and giving the `c` type specifier
in the **format** string.

You can read more about adding custom parser callbacks in the
[add options guide](adding.md#custom-parsing).

## Functions ##

Here you find the core API for cargo.

### cargo_init ###

### cargo_destroy ###

### cargo_set_flags ###

### cargo_get_flags ###

### cargo_add_optionv ###

### cargo_add_option ###

### cargo_add_alias ###

### cargo_add_group ###

### cargo_group_add_option ###

### cargo_group_set_flags ###

### cargo_add_mutex_group ###

### cargo_mutex_group_add_option ###

### cargo_set_metavar ###

### cargo_set_internal_usage_flags ###

### cargo_parse ###

### cargo_set_option_count_hint ###

### cargo_set_prefix ###

### cargo_set_max_width ###
### cargo_set_description ###
### cargo_set_epilog ###
### cargo_fprint_usage ###

### cargo_print_usage ###

### cargo_get_usage ###

### cargo_get_error ###

### cargo_get_unknown ###

### cargo_get_args ###

## Utility functions ##

These are not a part of the core, but are nice to have.

### cargo_get_fprint_args ###

### cargo_get_fprintl_args ###

### cargo_get_vfprint_args ###

### cargo_fprint_args ###

### cargo_fprintl_args ###

### cargo_split_commandline ###

### cargo_free_commandline ###
