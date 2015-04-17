cargo API reference
===================

## Table of contents ##

[TOC]

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

This is the language used by the [`cargo_add_option`](api.md#cargo_add_option) function. To help in learning this language cargo comes with a small helper program [`cargo_helper`](adding.md#help-with-format-strings) that lets you input a variable declaration such as `int *vals` and will give you examples of API calls you can use to parse it.

### Examples ###

To demonstrate how this language works here is a list of examples and what they mean. After that we can go into the details.

These examples is what you would pass as `fmt` string, and the `...` variable arguments to [`cargo_add_option`](api.md#cargo_add_option):

* `"i", &val`
  Parse a single integer into `int val`.
* `"b", &val`
  Parse an option as a flag without any arguments. Stores `1` in `val` by default.
* `"b=", &val, 5`
  Parse an option as a flag. Stores 5 in `val`.
* `"b!", &val`
  Parse an option as a flag. Allow multiple occurances, count them and store the result in `val`.
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
- `L` long long integer `long long int`
- `U` unsigned long long integer `unsigned long long int`
- `f` float `float`
- `d` double `double`
- `s` string `char *`
- `c` custom callback (you supply your own parse function).
- `D` Parses nothing (can be useful together with mutex groups).

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

### Booleans / flags

The `b` specifier denotes a boolean value. By default you pass an `int` as the argument. If the flag is given in the commandline arguments, then a `1` will be stored in it.

This behaviour can be changed by appending a set of specifiers after `b`:

- `=` After the integer variable, specify a value that will be stored in it instead of the default `1`.
- `!` Allow multiple occurances of the given flag, count how many times it occurs and store it in the specified integer variable. `-v -v -v` and `-vvv` is equivalent. This is useful for `verbosity` flags and such.

Parse an ordinary flag and store `1` in `val` if set: `b, &val`
Or parse the flag but store `5` in `val` if set: `b, &val, 5`
Or count the number of occurrances of the flag: `b!, &val`

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

## Types ##

### cargo_t ###

This is the type of the cargo context. This type is opague and should never be manipulated directly, only by using the cargo API functions.

To allocate a new instance [`cargo_init`](api.md#cargo_init) is used. And to destroy it use [`cargo_destroy`](api.md#cargo_destroy)



## Flags ##

### cargo_flags_t ###

These flags are global for the cargo instance and set when calling [`cargo_init`](api.md#cargo_init) or [`cargo_set_flags`](api.md#cargo_set_flags)

#### `CARGO_AUTOCLEAN` ####
By default it is up to the caller to free any option values that are returned by cargo when parsing.

This flag makes cargo `free` the variables that it has allocated while parsing automatically.

Note that this does not include any variables parsed in custom callbacks.

Default behaviour:

```c
char *str = NULL;
cargo_t cargo;
cargo_init(&cargo, 0, argv[0]);
cargo_add_option(cargo, 0, "--opt", "Some string", "s", &str);
cargo_destroy(&cargo);
if (str) free(str); // We must free!
```

Autoclean:

```c
char *str = NULL;
cargo_t cargo;
cargo_init(&cargo, 0, argv[0]);
cargo_add_option(cargo, CARGO_AUTOCLEAN, "--opt", "Some string", "s", &str);
cargo_destroy(&cargo); // str is freed here.
```

#### `CARGO_NOCOLOR` ####
Turn off color output for any cargo output.

#### `CARGO_NOERR_OUTPUT` ####
By default cargo will print any parse error automatically to `stderr`.

This turns this off. Use this if you want to customize the error output.

[`cargo_get_error`](api.md#cargo_get_error) can be used to get the error.

#### `CARGO_NOERR_USAGE` ####
Whenever cargo prints parse errors internally it will also print the short usage information.

This flags does not print the short usage on error.

Note if you simply want to customize the usage output printed by cargo on internal errors you can set the usage flags using [`cargo_set_internal_usage_flags`](api.md#cargo_set_internal_usage_flags).

#### `CARGO_ERR_STDOUT` ####
cargo prints errors to `stderr` by default. This flag changes so that it prints to `stdout` instead.

#### `CARGO_NO_AUTOHELP` ####
This flag turns off the automatic creating of the `--help` option.

### cargo_usage_t ###

This is used to specify how the usage is output. These flags are used by the [`cargo_get_usage`](api.md#cargo_get_usage) function and friends.

#### `CARGO_USAGE_FULL_USAGE` ####
Show the full usage. This is the default, same as specifying `0`.

Note that this includes the short usage as well. If you want the full usage but excluding the short usage you can use [`CARGO_USAGE_HIDE_SHORT`](api.md#cargo_usage_hide_short)

#### `CARGO_USAGE_SHORT_USAGE` ####
Show only the short usage.

#### `CARGO_USAGE_RAW_DESCRIPTION` ####
The description passed to [`cargo_init`](api.md#cargo_init) or set using [`cargo_set_description`](api.md#cargo_set_description) will be displayed as is, and no automatic formatting is done by cargo.

#### `CARGO_USAGE_RAW_OPT_DESCRIPTIONS` ####
All option descriptions will be treated as raw. Note that this can be set on a per option basis as well using [`CARGO_OPT_RAW_DESCRIPTION`](api.md#cargo_opt_raw_description). Cargo will not perform any automatic formatting on the option descriptions.

#### `CARGO_USAGE_RAW_EPILOG` ####
The epilog (text after all option descriptions) set using [`cargo_set_epilog`](api.md#cargo_set_epilog) will be displayed as is, and no automatic formatting is done by cargo.

#### `CARGO_USAGE_HIDE_DESCRIPTION` ####
Hides the description.

#### `CARGO_USAGE_HIDE_EPILOG` ####
Hides the epilog.

#### `CARGO_USAGE_HIDE_SHORT` ####
Hide the short usage information but show the rest.


### cargo_option_flags_t ###

These flags are passed to [`cargo_add_option`](api.md#cargo_add_option) when adding a new option.

#### `CARGO_OPT_UNIQUE` ####
The default behaviour for an option that is specified more than once is to use the last value specified. So `-a 1 -b 2 -a 3` results in `-a` containing `3` after parsing.

This setting makes cargo not allow this, and instead give an error when specifying an option more than once.

#### `CARGO_OPT_REQUIRED` ####
By default any option prepended with prefix characters `--option` are considered optional (hence the name option), this flag turns off this behavior and makes it required.

#### `CARGO_OPT_NOT_REQUIRED` ####
When adding an option that is not prepended by any prefix characters `argument` it is considered to be a **positional** argument, and required.

This flag turns off this behaviour and makes it not required.

#### `CARGO_OPT_RAW_DESCRIPTION` ####
This makes the option description considered literal by cargo, and no automatic formatting will be performed.

To enable this for all options instead the [`CARGO_USAGE_RAW_OPT_DESCRIPTIONS`](api.md#cargo_usage_raw_opt_descriptions) flag can be passed to [`cargo_init`](api.md#cargo_init)


### cargo_mutex_group_flags_t ###

These flags control how a mutex group created using [`cargo_add_mutex_group`](api.md#cargo_add_mutex_group) behaves.

#### `CARGO_MUTEXGRP_ONE_REQUIRED` ####
By default none of the options in a mutex group is required.

This flag will require that at one of the members of the group is specified (but only one of course), otherwise an error is given.

Note that you probably want to make sure that the [`CARGO_OPT_NOT_REQUIRED`](api.md#CARGO_OPT_NOT_REQUIRED) flag is set for all options that are part of the mutex group, otherwise you will get conflicting requirements.

#### `CARGO_MUTEXGRP_GROUP_USAGE` ####
By default any members that are part of a mutex group are not shown together, but rather in whatever order they were added in.

This flag will instead group them and show the description given to the group in [`cargo_add_mutex_group`](api.md#cargo_add_mutex_group).

Note that the options grouped like this will not be shown in their normal group/position.

#### `CARGO_MUTEXGRP_NO_GROUP_SHORT_USAGE` ####
Mutex group variables are by default shown grouped together like this `{--opt1, --opt2, --opt3}` to indicate only one of them should be picked.

This flag turns off this behaviour and shows the variables in the normal way `--opt1 --opt2 --opt3`.

Note that when having options in multiple mutex groups this flag might be useful, since otherwise options will show up multiple times in the short usage. When for example `--opt1` is in two mutex groups: `{--opt1, --opt2, --opt3} {--opt1, --opt4, --opt5}` compared to `--opt1 --opt2 --opt3 --opt4 --opt5`.

Another way of overriding this is to simply display the variables in the mutex group completely as you like by setting it manually using [`cargo_mutex_group_set_metavar`](api.md#cargo_mutex_group_set_metavar)

#### `CARGO_MUTEXGRP_RAW_DESCRIPTION` ####
This turns of any automatic formatting for the mutex group description.


### cargo_group_flags_t ###

These flags are used to specify the behaviour of groups added using [`cargo_add_group`](api.md#cargo_add_group)

#### `CARGO_GROUP_HIDE` ####
This hides the group in the usage.

A use case for this might be an `--advanced_help` that unhides the group and prints the usage.

#### `CARGO_GROUP_RAW_DESCRIPTION` ####
This turns of any automatic formatting for the group description.


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

Adds an option for cargo to parse.

### cargo_add_alias ###

```c
int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**optname**: The name of the option that the alias should be added to.

**alias**: The name of the alias.

---

This can be used to add an alias to an option name. So if you have an option with the name `"--option"`, you can add an alias `"-o"` for it by doing:

```c
ret = cargo_add_alias(cargo, "--option", "-o");
```

Note that you can have max [`CARGO_NAME_COUNT - 1`](api.md#CARGO_NAME_COUNT) aliases for an option name.

Also note that this is usually best done directly when calling [`cargo_add_option`](api.md#cargo_add_option) instead.

You cannot aliases to positional arguments (options starting without a prefix character).

### cargo_add_group ###

```c
int cargo_add_group(cargo_t ctx, cargo_group_flags_t flags, const char *name,
					const char *title, const char *description)
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_group_flags_t`](api.md#cargo_group_flags_t) for flags.

**name**: The group name used to identify the group, `group_name`.

**title**: The title shown in the usage output `Group Name`.

**description**: The description shown in the usage for the group.

---

Adds a new option group. This can be used to group options together in the usage output.

To add a group:

```c
ret = cargo_add_group(cargo, 0, "group1", "The Group 1", "This group is 1st");
```

Both the group `title` and `description` can be `NULL`. If the `title` isn't
set the `name` will be used instead.

 You can add options to the group by either using [`cargo_group_add_option`](api.md#cargo_group_add_option) or inline in [`cargo_add_option`](api.md#cargo_add_option).

### cargo_group_add_option ###

```c
int cargo_group_add_option(cargo_t ctx, const char *group, const char *opt);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The group name you want to add the option to.

**opt**: The name of the option you want to add to the group.

---

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

```c
int cargo_group_set_flags(cargo_t ctx, const char *group,
						   cargo_group_flags_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The group name you want to change the flags for.

**flags**: See [`cargo_group_flags_t`](api.md#cargo_group_flags_t) for flags.

---

Sets the flags for a group.

### cargo_add_mutex_group ###

```c
int cargo_add_mutex_group(cargo_t ctx,
            cargo_mutex_group_flags_t flags,
            const char *name,
            const char *title,
            const char *description);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_mutex_group_flags_t`](api.md#cargo_mutex_group_flags_t) for flags.

**name**: The name of the mutex group.

**title**: The title of the mutex group.

**description**: A description of the mutex group.

---

This creates a mutually exclusive group. Only one of the options in this group is allowed to be specified as an argument.

If you specify the `CARGO_MUTEXGRP_ONE_REQUIRED` flag, one of the flag *has* to be specified.

### cargo_mutex_group_add_option ###

```c
int cargo_mutex_group_add_option(cargo_t ctx,
								const char *group,
								const char *opt);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the mutex group.

**opt**: The name of the option to add to the mutex group.

---

Adds an option to a mutex group.

### cargo_mutex_group_set_metavar ###

```c
int cargo_mutex_group_set_metavar(cargo_t ctx, const char *mutex_group, const char *metavar);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the mutex group.

**metavar**: The meta variable name.

---

This sets the meta variable name for a given mutex group. 

By default the individual variables are shown in the short usage printed by [`cargo_get_usage`](api.md#cargo_get_usage) and friends.

Setting this will show whatever is set in `metavar` instead of all the variable names.

In the example below if `--beta` and `--centauri` are in a mutex group this is what is shown in the usage by default:

```
Usage: program [--alpha ALPHA] {--beta BETA, --centauri}
```

So setting the `metavar` can override this to whatever you want:

```c
cargo_mutex_group_set_metavar(cargo, "mutexgroup", "VARS")
```

which yields:

```
Usage: program [--alpha ALPHA] VARS
```

For another setting related how the mutex group is shown in the usage see [`CARGO_MUTEXGRP_NO_GROUP_SHORT_USAGE`](api.md#cargo_mutexgrp_no_group_short_usage).

### cargo_set_metavar ###

```c
int cargo_set_metavar(cargo_t ctx, const char *optname, const char *metavar);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**optname**: The option name you want to set the metavar for.

**metavar**: The meta variable name.

---

Change the default "metavar" that is shown in the usage output to identify the values for an option.

The default is to simply use the option name in uppercase:

```c
--option OPTION 
```

So you can change this to whatever you want:

```c
ret = cargo_set_metavar(cargo, "--option", "THEVALUE");
```

Which gives:

```c
--option THEVALUE
```

### cargo_set_internal_usage_flags ###

```c
void cargo_set_internal_usage_flags(cargo_t ctx, cargo_usage_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_usage_t`](api.md#cargo_usage_t).

---

This sets the internal usage flags used when cargo automatically outputs errors and shows the usage when it gets invalid input in [`cargo_parse`](api.md#cargo_parse).

These are the same flags that you set when calling [`cargo_get_usage`](api.md#cargo_get_usage).

By default on an error only the short usage is shown together with the error. If you want the long error you would set [`CARGO_USAGE_FULL_USAGE`](api.md#CARGO_USAGE_FULL_USAGE) flag here. Or any of the [`cargo_usage_t`](api.md#cargo_usage_t) flags to customize the output.

### cargo_parse ###

```c
int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**start_index**: What index into `argv` should cargo start parsing from.

**argc**: The number of arguments in `argv`.

**argv**: A list of strings containing the arguments to parse.

---

This is where cargo does its business parsing the command line arguments passed to it.

Usually the first argument in `argv` contains the program executable, but you can pass anything to cargo, so you can specify the `start_index` into `argv` that cargo should start parsing from. Usually this will be at index `1`.

```c
argv = { "the/program", "--option", "5" };
  // Start parsing here ^
argc = 3;
...
ret = cargo_parse(cargo, 1, argc, argv);
```

By default cargo will try to parse the arguments it is given, and if there is an error it will output it to `stderr` including a short usage message.

If you want to override this behaviour, you can change this behaviour by setting the [`cargo_flags_t`](api.md#cargo_flags_t).

You can turn it off completely and instead use [`cargo_get_usage`](api.md#cargo_get_usage), [`cargo_get_error`](api.md#cargo_get_error) and [`cargo_get_unknown`](api.md#cargo_get_unknown) to customize the output however you want.

### cargo_set_option_count_hint ###

```c
void cargo_set_option_count_hint(cargo_t ctx, size_t option_count);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**option_count**: The number of options you will add to cargo.

---

This gives cargo a hint about how many options you will be adding so that it can allocate the correct number of options right away instead of having to do a reallocation.

### cargo_set_prefix ###

```c
void cargo_set_prefix(cargo_t ctx, const char *prefix_chars);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**prefix_chars**: A string containing the prefix characters.

---

This will set the prefix characters that cargo will use. By default this is set to [`CARGO_DEFAULT_PREFIX`](api.md#CARGO_DEFAULT_PREFIX) which is `"="` unless it has been overriden in `"cargo_config.h"`.

For instance you can allow both `"-"` and `"+"` by setting this to `"-+"`. So then you can add an option such as `"--option"` or `"++option"`.

### cargo_set_max_width ###

```c
void cargo_set_max_width(cargo_t ctx, size_t max_width);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**max_width**: The max width for the usage output.

---

Sets the max width that the usage output must fit inside. By default this is set to [`CARGO_AUTO_MAX_WIDTH`](api.md#CARGO_AUTO_MAX_WIDTH) or `0`. In this mode cargo will attempt to set the max width to the current width of the console it is running in.

If it fails to get the console width from the operating system it will fall back to using [`CARGO_DEFAULT_MAX_WIDTH`](api.md#CARGO_DEFAULT_MAX_WIDTH) which is `80` characters unless it has been overridden.

The max width allowed for this is [`CARGO_MAX_MAX_WIDTH`](api.md#CARGO_MAX_MAX_WIDTH). 

### cargo_set_description ###

```c
void cargo_set_description(cargo_t ctx, const char *description);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**description**: The usage description.

---

This sets the description shown first in the usage output, before the list of options.

### cargo_set_epilog ###

```c
void cargo_set_epilog(cargo_t ctx, const char *epilog);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**epilog**: The usage epilog.

---

This sets the epilog, the text shown after the list of options.

### cargo_fprint_usage ###

```c
int cargo_fprint_usage(cargo_t ctx, FILE *f, cargo_usage_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**f**: A file pointer to print to.

**flags**: See [`cargo_usage_t`](api.md#cargo_usage_t).

---

This is a convenience function and does the same thing as doing:

```c
fprintf(f, "%s\n", cargo_get_usage(cargo, flags));
```

### cargo_print_usage ###

```c
int cargo_print_usage(cargo_t ctx, cargo_usage_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_usage_t`](api.md#cargo_usage_t).

---

This is a convenience function and does the same thing as doing:

```c
printf("%s\n", cargo_get_usage(cargo, flags));
```

### cargo_get_usage ###

```c
const char *cargo_get_usage(cargo_t ctx, cargo_usage_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_usage_t`](api.md#cargo_usage_t).

---

This returns a string containing the usage for the given cargo context. 

Please note that cargo is responsible for freeing this string, so if you want to keep it make sure you create a copy.

### cargo_get_error ###

```c
const char *cargo_get_error(cargo_t ctx);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

---

This will return any error that was set by [`cargo_parse`](api.md#cargo_parse) in the last call to it. This is useful if you want to customize exactly how the error is shown. By default this will be printed to `stderr`. See [`cargo_flags_t`](api.md#cargo_flags_t) to turn that behaviour off.

Please note that cargo is responsible for freeing this string, so if you want to keep it make sure you create a copy.

### cargo_get_unknown ###

```c
const char **cargo_get_unknown(cargo_t ctx, size_t *unknown_count);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**unknown_count**: A pointer to a `size_t` where the number of unknown options passed to [`cargo_parse`](api.md#cargo_parse) will be returned.

---

This will return a list of strings containing the unknown options that were passed to the last call to [`cargo_parse`](api.md#cargo_parse).

Please note that cargo is responsible for freeing this string, so if you want to keep it make sure you create a copy of each string in the returned array.

### cargo_get_args ###

```c
const char **cargo_get_args(cargo_t ctx, size_t *argc);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**argc**: A pointer to a `size_t` where the number of arguments passed to [`cargo_parse`](api.md#cargo_parse) that were not consumed is retruned.

---

This will return any remaining arguments left after [`cargo_parse`](api.md#cargo_parse) has parsed the arguments passed to it.

### cargo_set_context ###

```c
void cargo_set_context(cargo_t ctx, void *user);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**user**: A `void` pointer to user context data.

---

This sets a global user context for the cargo parser. This can then be used in the custom callback functions when parsing. You can get this using [`cargo_get_context`](api.md#cargo_get_context)

### cargo_get_context ###

```c
void *cargo_get_context(cargo_t ctx);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

---

This returns the global user context set using [`cargo_set_context`](api.md#cargo_set_context)

### cargo_set_group_context ###

```c
int cargo_set_group_context(cargo_t ctx, const char *group, void *user);
```
---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the group to set the context for. If this is `NULL`, the context is set for the default group.

**user**: A pointer to a user specified data structure.

---

You can use this function to save a context for an option group. You can later get this context using [`cargo_get_group_context`](api.md#cargo_get_group_context).

Passing `NULL` as the `group` name adds the context to the default group. This is the group all options are added to by default unless another group is specified.

This can be used to pass your own group context to a [`cargo_custom_cb_t`](api.md#cargo_custom_cb_t) when parsing a custom argument.

### cargo_get_group_context ###

```c
void *cargo_get_group_context(cargo_t ctx, const char *group);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the group to get the context for. If this is `NULL`, the context is set for the default group that all options are added to by default.

---

This can be used to fetch the context or a given option group when parsing custom arguments in a [`cargo_custom_cb_t`](api.md#cargo_custom_cb_t) callback function.

To set this context for a group see [`cargo_set_group_context`](api.md#cargo_set_group_context).

Since you are not passed an options group name in the [`cargo_custom_cb_t`](api.md#cargo_custom_cb_t) callback, you can use [`cargo_get_option_group`](api.md#cargo_get_option_group) to get it given the option name.

```c

int the_parse_callback(cargo_t ctx, void *user, const char *optname,
                      int argc, char **argv)
{
    const char *group = cargo_get_option_group(ctx, optname);
    my_group_ctx_t *grp_ctx = cargo_get_group_context(ctx, group);

    ... // Use your group context.

    return argc;
}
```


### cargo_set_mutex_group_context ###

```c
int cargo_set_mutex_group_context(cargo_t ctx,
                const char *mutex_group,
                void *user);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**mutex_group**: The name of the mutex group to set the context for.

**user**: A pointer to a user specified data structure.

---

This sets a user context for a given mutex group, just like [`cargo_set_group_context`](api.md#cargo_set_group_context) does for normal groups.

The only difference is that there is no default mutex group, so passing a `NULL group is not valid.

### cargo_get_mutex_group_context ###

```c
void *cargo_get_mutex_group_context(cargo_t ctx, const char *mutex_group);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**mutex_group**: The name of the mutex group to get the context for.

---

This gets the context for the given mutex group. Just like [`cargo_get_group_context`](api.md#cargo_get_group_context) does for normal groups.

Note however that an option can be a member of multiple mutex groups at once, so when getting them you will be given a list instead of a single group name. See [`cargo_get_option_mutex_groups`](api.md#cargo_get_option_mutex_groups)


### cargo_get_option_group ###

```c
const char *cargo_get_option_group(cargo_t ctx, const char *opt);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**opt**: The option name to get the group for.

---

Gets the group a given option is associated with.

### cargo_get_option_mutex_groups ###

```c
const char **cargo_get_option_mutex_groups(cargo_t ctx,
                    const char *opt,
                    size_t *count);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**opt**: The option name to get the list of mutex groups for.

**count**: Pointer to a `size_t` variable where the list count will be returned.

---

This will get the list of mutex groups associated with a given option.

Note that the list is kept internally in cargo and should not be freed by the caller.

## Utility flags ##

### cargo_fprint_flags_t ###

These flags are used by the `fprint_args` family of functions used to highlight errors in the command line.

#### `CARGO_FPRINT_NOCOLOR` ####
This turns off the ANSI color output.

#### `CARGO_FPRINT_NOARGS` ####
If the command line arguments should be shown or not in the output. That is, if only the highlight should be shown.

Without this flag:

```
--alpha abc --beta def ghi --crazy banans
^^^^^^^     ~~~~~~ ---
```

With this flag:

```
^^^^^^^     ~~~~~~ ---
```

#### `CARGO_FPRINT_NOHIGHLIGHT` ####
If the highlight should be hidden.

With this flag:

```
--alpha abc --beta def ghi --crazy banans
```

Without this flag:

```
--alpha abc --beta def ghi --crazy banans
^^^^^^^     ~~~~~~ ---
```

## Utility functions ##

These are not a part of the core, but are nice to have.

### cargo_get_fprint_args ###

```c
char *cargo_get_fprint_args(int argc, char **argv, int start,
              cargo_fprint_flags_t flags,
              size_t max_width,
              size_t highlight_count, ...);
```
---

**argc**: The number of arguments in `argv`.

**argv**: A list of arguments.

**start**: At what index into `argv` to start printing the arguments.

**flags**: See [`cargo_fprint_flags_t`](api.md#cargo_fprint_flags_t).

**max_width**: The max width to use for the arguments. This is so that we don't try to show a list of arguments that wraps on to a new line, because then the highlights would point to the wrong thing. This works the same as for [`cargo_set_max_width`](api.md#cargo_set_max_width).

**highlight_count**: The number of highlights to print. This must match the number of varargs combinations following.

**...**: A set of pairs of arguments containing an index and a highlight character in a `char *` string. After the highlight character you can optionally specify an ANSI color string that decides the color of the highlight.

---

This will return a string containing the output from an `argv` array as well as highlight characters under a set of specified arguments in this array.

This is used by cargo internally to output useful error messages, highlighting exactly what arguments are invalid.

For example:

```bash
Usage: bin/cargo_ex_ints [--sum ] [--help ] INTEGERS [INTEGERS ...]
Unknown options:
--sum --nonsense 1 2 3
      ~~~~~~~~~~
```

You do this by passing a variable set of arguments containing an index into `argv` as well as a highlight character, in the example above `"~"`.

Here is an example of how it is called:

```c
char *s;
s = cargo_get_fprint_args(argc, argv,
							start,	// start index into argv.
							0,		// flags.
							3,		// highlight_count
							        // (how many highlights below).
							0, "^"CARGO_COLOR_RED,    // 1. Highlight index 0
							2 ,"~",                   // 2. Highlight index 2
							4, "*"CARGO_COLOR_CYAN);  // 3. Highlight index 4
...
free(s);
```

This will produce the following output:

```c
program first second --third 123
^^^^^^^       ~~~~~~         ***
```

As you can see, you pass it an index, for instance `4`, as well as a highlight character `"*"`. In the example above, by appending a color after the highlight character this will be used when printing the highlight character.

Cargo has a set of predefined colors with the `CARGO_COLOR_*` macros. But any ANSI color code can be used.

Note that cargo internally also supports outputting these ANSI colors on **Windows** which does not have native ANSI console color support normally.

If you prefer to return the output the result without any colors applied you can pass the [`CARGO_FPRINT_NOCOLOR`](api.md#CARGO_FPRINT_NOCOLOR) flag.

### cargo_get_fprintl_args ###

```c
char *cargo_get_fprintl_args(int argc, char **argv, int start,
							cargo_fprint_flags_t flags,
							size_t highlight_count,
							const cargo_highlight_t *highlights);
```

---

**argc**: The number of arguments in `argv`.

**argv**: A list of arguments.

**start**: At what index into `argv` to start printing the arguments.

**flags**: See [`cargo_fprint_flags_t`](api.md#cargo_fprint_flags_t).

**max_width**: The max width to use for the arguments. This is so that we don't try to show a list of arguments that wraps on to a new line, because then the highlights would point to the wrong thing. This works the same as for [`cargo_set_max_width`](api.md#cargo_set_max_width).

**highlight_count**: The number of highlights to print. This must match the number of varargs combinations following.

**highlights**: An array of [`cargo_highlight_t`](api.md#cargo_highlight_t) structs containing an index and a highlight character in a `char *` string. After the highlight character you can optionally specify an ANSI color string that decides the color of the highlight.

---

This does the same things as [`cargo_get_fprint_args`](api.md#cargo_get_fprint_args) except that it takes an array of [`cargo_highlight_t`](api.md#cargo_highlight_t) as arguments instead.

This allows you to dynamically highlight any number of arguments in the given `argv` array.

---

### cargo_get_vfprint_args ###

```c
char *cargo_get_vfprint_args(int argc, char **argv, int start,
							cargo_fprint_flags_t flags,
							size_t highlight_count, va_list ap);
```

The var args version of [`cargo_get_fprint_args`](api.md#cargo_get_fprint_args).

### cargo_fprint_args ###

```c
int cargo_fprint_args(FILE *f, int argc, char **argv, int start,
					  cargo_fprint_flags_t flags,
					  size_t highlight_count, ...);
```

Convenience function for [`cargo_get_fprint_args`](api.md#cargo_get_fprint_args) that prints to a given `FILE *` instead.

This will print the proper ANSI color even on **Windows**.

### cargo_fprintl_args ###

```c
int cargo_fprintl_args(FILE *f, int argc, char **argv, int start,
					   cargo_fprint_flags_t flags, size_t highlight_count,
					   const cargo_highlight_t *highlights);
```

Convenience function for [`cargo_get_fprintl_args`](api.md#cargo_get_fprintl_args) that prints to a given `FILE *` instead.

This will print the proper ANSI color even on **Windows**.

### cargo_split_commandline ###

```c
char **cargo_split_commandline(const char *args, int *argc);
```

---

**args**: A string containing a list of arguments you want to split into an `argv` array.

**argc**: A pointer to an `int` that the number of arguments found in `args`.

---

This can be used to split a command line string into an `argv` array that you then can pass to [`cargo_parse`](api.md#cargo_parse).

Internally this uses [`wordexp`](http://linux.die.net/man/3/wordexp) on Unix systems and [`CommandLineToArgvW`](https://msdn.microsoft.com/en-us/library/windows/desktop/bb776391%28v=vs.85%29.aspx) on Windows.

### cargo_free_commandline ###

```c
void cargo_free_commandline(char ***argv, int argc);
```

---

**argv**: A pointer to an array of arguments strings.

**argc**: The number of argument strings fround in `argv`.

---

This can be used to free and `NULL` an `argv` array that was split using [`cargo_split_commandline`](api.md#cargo_split_commandline)

