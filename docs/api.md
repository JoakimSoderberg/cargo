Table of contents
=================

[TOC]

API reference
=============

## Default defines ##

### `CARGO_CONFIG` ###

If this is set, `cargo.h` will `#include cargo_config.h` where you can override any of the default macros below. So that you can leave `cargo.h` untouched.

### `CARGO_NAME_COUNT` ###

The max number of names allowed for an option. Defines how many names and aliases any given option is allowed to have.

### `CARGO_DEFAULT_PREFIX` ###

This defines the default prefix characters for options. Note that this can also be changed using [`cargo_set_prefix`](api.md#cargo_set_prefix).

### `CARGO_DEFAULT_MAX_OPTS` ###

The default start value for the max number of options. If more options are added, the list of options will be reallocated.

This can also be changed using the function [`cargo_set_option_count_hint`](api.md#cargo_set_option_count_hint)

### `CARGO_DEFAULT_MAX_WIDTH` ###

The default max width for the usage output if we cannot get the console width from the operating system. By default the max width is set to [`CARGO_AUTO_MAX_WIDTH`](api.md#CARGO_AUTO_MAX_WIDTH).

This is used by [`cargo_set_max_width`](api.md#cargo_set_max_width).

### `CARGO_AUTO_MAX_WIDTH` ###

Sets the max width for the usage output to the console width.

### `CARGO_MAX_MAX_WIDTH`

The absolute max console width allowed, any value set via This is used by [`cargo_set_max_width`](api.md#cargo_set_max_width), will be capped to this.

Cargo version
-------------

### `CARGO_MAJOR_VERSION` ###

The major version of cargo. Example: **1**.2.3

### `CARGO_MINOR_VERSION` ###

The minor version of cargo. Example: 1.**2**.3

### `CARGO_PATCH_VERSION` ###

The patch version of cargo. Example: 1.2.**3**

### `CARGO_RELEASE` ###

If this is a release build, this will be set to `0`. Otherwise it is a development build.

### `CARGO_VERSION` ###

A hexidecimal representation of the cargo version, as a 4-byte integer in the form `(MAJOR << 24) | (MINOR << 16) | (PATCH << 8)`.

The last byte signifies the release status. `0` means release, and non-zero is a developer release after a given release.

So fo example cargo version `"1.2.3"` will have a version value of `0x01020300`. And  `"1.2.3-alpha"` could be `0x01020304`.

This is useful for doing compile time checks that you have a new enough version for instance.

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

This is the callback function for doing custom parsing as specified when using [`cargo_add_option`](api.md#cargo_add_option) and giving the `c` type specifier in the **format** string.

You can read more about adding custom parser callbacks in the [add options guide](adding.md#custom-parsing).

## Formatting language ##

This is the language used by the [`cargo_add_option`](api.md#cargo_add_option) function.

### Examples ###

To demonstrate how this language works here is a list of examples and what they mean. After that we can go into the details.

These examples is what you would pass as `fmt` string, and the `...` variable arguments to [`cargo_add_option`](api.md#cargo_add_option):

* `"i", &val`
  Parse a single integer into `int val`.
* `"f", &val`
  Parse a single float into `float val`.
* `"d", &val`
  Parse a single double into `double val`.
* `"s", &str`
  Parse and allocate memory for `char *str`.
* `"s#", &str, 32`
  Parse and allocate memory for `char *str` of max length 32.
* `".s#", &str, 32`
  Parse and copy to `char str[32]` of max length 32.
* `"[i]*", &vals, &count`
  Parse and allocate **zero or more** integers into `int *vals` and store the number parsed into `size_t count`.
* `"[f]+", &vals, &count`
  Parse and allocate **one or more** floats into `float *vals` and store the number parsed into `size_t count`
* `"[s]+", &strs, &count`
  Parse and allocate **one or more** strings into `char **strs` and store the number parsed into `size_t count`
* `"[i]#", &vals, &count, 4`
  Parse and allocate max 4 integers into `int *vals` and store the number parsed into `size_t count`
* `"[s]#", &strs, &count, 5`
  Parse and allocate max 5 strings into `char **strs` and store the number parsed into `size_t count`
* `"[s#]#", &strs, 32, &count, 5`
  Parse and allocate max 5 strings of max 32 length, and store the number parsed into `size_t count`.
* `".[i]#", &vals, &count, 4`
  Parse max 4 integers into `int vals[4]` and store the number parsed into `size_t count`.
* `".[i]+", &vals, &count, 4`
  Parse max 4 integers into `int vals[4]` and store the number parsed into `size_t count`.

### Type ###

The basis of the format is a type specifier:

- `b` boolean `int` (used for flags without arguments).
- `i` integer `int`
- `u` unsigned integer `unsigned int`
- `f` float `float`
- `d` double `double`
- `s` string `char *`
- `c` custom callback (you supply your own parse function).

Only one type specifier is allowed in a format string.

To parse an option that expects a `float` value as argument you call [`cargo_add_option`](api.md#cargo_add_option) in the following way:

Example:
To parse a float:
```c
float val;
cargo_add_option(cargo, 0, "--opt", "description", "f", &val);
```

Parse a string (note that the memory for this will be allocated):
```c
char *str;
..., "s", &str);
```

If an option expects an optional value, you can prepend it with `?`:
```c
float val = 0.3f;
cargo_add_option(cargo, 0, "--opt", "description", "f?", &val);
```

### Arrays

To parse an array/list of values you specify enclose the type into brackets like this: `[` type `]`.

Then to tell cargo how many elements you want to parse, you must append a size specifier:

- `+` 1 or more values.
- `*` 0 or more values.
- `#` or `N` This means we will pass the expected number of values as a variable argument.

So to parse **1 or more** `int`: `"[i]+"`
Or parsing **0 or more** `float`: `"[f]*"`
Or parsing **4** `double`: `"[d]#"`

### Allocation, fixed size and strings ###

To tell cargo not allocate the memory, but instead simply copy the parse result into an already existing fixed array you prepend the format string with `.`

**Detailed explination**

As you might have noticed in the examples above, you pass the address to the variable where you want to cargo to store the parsed result.

However, as seen above a C type string consists of a pointer `char *`. How does cargo know if it should allocate memory for this string, or to simply copy the result into an already existing string?

For this reason, strings are handled in a special way. By default if you parse a single argument of any given value type, such as `i`, `f`, `d` and so on, the parsed result will simply be stored as a value in them. But when it comes to strings, the default when specifiying `s` cargo will allocate memory for that string.

To override this behavior you can prepend the formatting string with a `.`, this will then make cargo treat the target variable it is given as a fixed sized array and copy the result into that rather than to allocate new space.

Since we now use a fixed array, we also need to append a `#` and pass the max length of the string as well.

```c
char str[32];
cargo_add_option(cargo, 0, "--opt", "description", ".s#", &str, 32);
```

To apply this on a list of integers:

```c
int vals[4];
size_t count;
cargo_add_option(cargo, 0, "--opt", "description", ".[i]#", &vals, &count, 4);
```

And a list of 5 fixed size strings with max length 32:

```c
char str[5][32]
size_t count;
cargo_add_option(cargo, 0, "--opt", "description",
                 ".[s#]#", &str, 32, &count, 5);
```

## Functions ##

Here you find the core API for cargo.

### cargo_init ###

```c
int cargo_init(cargo_t *ctx, cargo_flags_t flags, const char *progname)
```

Initializes a [`cargo_t`](api.md#cargo_t) context. See [Initializing cargo](api.md#initializing_cargo) for an example. You need to free this context using [`cargo_destroy`](api.md#cargo_destroy).

---

**Return value**: -1 on fatal error. 0 on success.

**ctx**: A pointer to a [`cargo_t`](api.md#cargo_t) context.

**flags**: Flags for setting global behavior for cargo. See [`cargo_flags_t`](api.md#cargo_flags_t).

**progname**: The name of the executable. Usually this will be set to `argv[0]`.

---

### cargo_destroy ###

```c
void cargo_destroy(cargo_t *ctx)
```

Destroys a [`cargo_t`](api.md#cargo_t) context.

---

**ctx**: A pointer to a [`cargo_t`](api.md#cargo_t) context.

---

### cargo_set_flags ###

```c
void cargo_set_flags(cargo_t ctx, cargo_flags_t flags)
```

Sets the flags of the [`cargo_t`](api.md#cargo_t) context.
See [`cargo_flags_t`](api.md#cargo_flags_t).

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: The flags [`cargo_flags_t`](api.md#cargo_flags_t).

---

### cargo_get_flags ###

```c
cargo_flags_t cargo_get_flags(cargo_t ctx)
```

Gets the flags of the [`cargo_t`](api.md#cargo_t) context.

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

---

### cargo_add_optionv ###

```c
int cargo_add_optionv(cargo_t ctx, cargo_option_flags_t flags,
					  const char *optnames, 
					  const char *description,
					  const char *fmt, va_list ap)
```

Variable arguments version of [`cargo_add_option`](api.md#cargo_add_option)

### cargo_add_option ###

```c
int cargo_add_option(cargo_t ctx, cargo_option_flags_t flags,
					 const char *optnames, const char *description,
					 const char *fmt, ...)
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: Option flags [`cargo_option_flags_t`](api.md#cargo_option_flags_t).

**optnames**: Option names in the form `"--alpha --al -a"`. The first will become the option name `--alpha`, the ones following will become aliases, `--al` and `-a`. It is also possible to add aliases using [`cargo_add_alias`](api.md#cargo_add_alias). Max [`CARGO_NAME_COUNT`](api.md#cargo_name_count) names allowed.

The names and aliases that start with a **prefix character** will become an option, the default one is [`CARGO_DEFAULT_PREFIX`](api.md#cargo_default_prefix) which is `"-"` unless it has been overridden. This can also be set with [`cargo_set_prefix`](api.md#cargo_set_prefix). Options are optional by default this can be changed by setting the [`CARGO_OPT_REQUIRED`](api.md#cargo_opt_required) flag.

If the name is not prepended with a prefix, it will become a **positional argument**. Positional arguments are required by default, this can be changed by setting [`CARGO_OPT_NOT_REQUIRED`](api.md#cargo_opt_not_required).

**description**: Description of the option.

**fmt**: Format string that tells cargo what arguments it should expect it will be passed. Just like how `printf` works. cargo will use this format definition when it parses the command line to know what it should attempt to parse. See [formatting language](api.md#formatting-language) for details.

---

### cargo_add_alias ###

```c
int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias);
```

This can be used to add an alias to an option name. So if you have an option with the name `"--option"`, you can add an alias `"-o"` for it by doing:

```c
cargo_add_alias(cargo, "--option", "-o");
```

Note that you can have max `CARGO_NAME_COUNT - 1` aliases for an option name.

Also note that this is usually best done directly when calling [`cargo_add_option`](api.md#cargo_add_option) instead.

### cargo_add_group ###

```c
int cargo_add_group(cargo_t ctx, cargo_group_flags_t flags, const char *name,
					const char *title, const char *description)
```

Adds a new option group. This can be used to group options together in the usage output.

To add a group:

```c
ret = cargo_add_group(cargo, 0, "group1", "The Group 1", "This group is 1st");
```

Both the group `title` and `description` can be `NULL`. If the `title` isn't
set the `name` will be used instead.

See [`cargo_group_flags_t`](api.md#cargo_group_flags_t) for flags. You can add options to the group by either using [`cargo_group_add_option`](api.md#cargo_group_add_option) or inline in [`cargo_add_option`](api.md#cargo_add_option).

### cargo_group_add_option ###

Use this to add an option to a group.

```c
ret = cargo_group_add_option(cargo, "group1", "--option");
```

It is also possible to do the same directly in [`cargo_add_option`](api.md#cargo_add_option):

```c
ret = cargo_add_option(cargo, 0, "<group1> --option",
                       "Description", "i", &val);
```

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
