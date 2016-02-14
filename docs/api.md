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

The default max width for the usage output if we cannot get the console width from the operating system. By default the max width is set to [`CARGO_AUTO_MAX_WIDTH`](api.md#cargo_auto_max_width).

This is used by [`cargo_set_max_width`](api.md#cargo_set_max_width).

### `CARGO_AUTO_MAX_WIDTH` ###

Sets the max width for the usage output to the console width.

### `CARGO_MAX_MAX_WIDTH` ###

The absolute max console width allowed, any value set via [`cargo_set_max_width`](api.md#cargo_set_max_width), will be capped to this.

### `CARGO_MAX_OPT_MUTEX_GROUP` ###

The max number of mutex groups an option is allowed to be a member of.

cargo version
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

A hexadecimal representation of the cargo version, as a 4-byte integer in the form `(MAJOR << 24) | (MINOR << 16) | (PATCH << 8)`.

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

### cargo_custom_f ###

```c
typedef int (*cargo_custom_f)(cargo_t ctx, void *user, const char *optname,
                                 int argc, char **argv);
```

This is the callback function for doing custom parsing as specified when using [`cargo_add_option`](api.md#cargo_add_option) and giving the `c` type specifier in the **format** string.

You can read more about adding custom parser callbacks in the [add options guide](adding.md#custom-parsing).

### cargo_validation_f ###

```c
typedef int (*cargo_validation_f)(cargo_t ctx,
                cargo_validation_flags_t flags,
                const char *opt, cargo_validation_t *vd,
                void *value);
```

This is a callback function that is called for each argument value of an option with a validation added to it.

The function is passed the [`cargo_validation_t`](api.md#cargo_validation_t) instance that contains the context needed to perform the validation.

The value is given as a `void *`, so you will have to cast it to the appropriate type: `char *str = ((char *)value);` or `int a = *((int *)value);`

### cargo_validation_destroy_f ###

```c
typedef void (*cargo_validation_destroy_f)(cargo_validation_t *vd);
```

If you create your own `cargo_validation_t` type, and add data to it, you might need to specify one of these to clean up after you.

## Formatting language ##

This is the language used by the [`cargo_add_option`](api.md#cargo_add_option) function. To help in learning this language cargo comes with a small helper program [`cargo_helper`](adding.md#help-with-format-strings) that lets you input a variable declaration such as `int *vals` and will give you examples of API calls you can use to parse it.

### Examples ###

To demonstrate how this language works here is a list of examples and what they mean. After that we can go into the details.

These examples is what you would pass as `fmt` string, and the `...` variable arguments to [`cargo_add_option`](api.md#cargo_add_option):

* `"i", &val`
  Parse a single integer into `int val`.
* `"b", &val`
  Parse an option as a flag without any arguments. Stores `1` in `val` if flag is set.
* `"b=", &val, 5`
  Parse an option as a flag. Stores 5 in `val` if flag is set.
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
  Parse and copy max 32 characters to `char str[32]`.
* `"[i]*", &vals, &count`
  Parse and allocate **zero or more** integers into `int *vals` and store the number of values parsed into `size_t count`.
* `"[f]+", &vals, &count`
  Parse and allocate **one or more** floats into `float *vals` and store the number of values parsed into `size_t count`
* `"[s]+", &strs, &count`
  Parse and allocate **one or more** strings into `char **strs` and store the number of strings parsed into `size_t count`
* `"[i]#", &vals, &count, 4`
  Parse and allocate max 4 integers into `int *vals` and store the number of integers parsed into `size_t count`
* `"[s]#", &strs, &count, 5`
  Parse and allocate max 5 strings into `char **strs` and store the number of strings parsed into `size_t count`
* `"[s#]#", &strs, 32, &count, 5`
  Parse and allocate max 5 strings of max 32 length, and store the number of strings parsed into `size_t count`.
* `".[i]#", &vals, &count, 4`
  Parse max 4 integers into `int vals[4]` and store the number of values parsed into `size_t count`.
* `".[i]+", &vals, &count, 4`
  Parse max 4 integers into `int vals[4]` and store the number of values parsed into `size_t count`.

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
```c
float val;
cargo_add_option(cargo, 0, "--opt", "description", "f", &val);
```

Parse a string (note that the memory for this will be allocated):
```c
char *str;
..., "s", &str);
```

### Optional value

If an option has an optional value, you can append `?`. If no value was specified on the commandline for the option, the extra string parameter is used `"0.5"` in the example below:
```c
float val;
cargo_add_option(cargo, 0, "--opt", "description", "f?", &val, "0.5");
```

That is `--opt` -> `val = 0.5f`, but `--opt 0.3` -> `val = 0.3f`.

**Note** This should be used with care if you are also parsing positional parameters, since this might "eat" one of those:

### Booleans / flags

The `b` specifier denotes a boolean value. By default you pass an `int` as the argument. If the flag is given in the commandline arguments, then a `1` will be stored in it.

This behaviour can be changed by appending a set of specifiers after `b`:

- `=` After the integer variable, specify a value that will be stored in it instead of the default `1`.
- `!` Allow multiple occurances of the given flag, count how many times it occurs and store it in the specified integer variable. `-v -v -v` and `-vvv` is equivalent. This is useful for `verbosity` flags and such.
- `|` Bitwise OR. This is similar to how `!` works, except instead of simply counting the number of occurances, this will do a bitwise OR operation. To do this, you specify a set of extra arguments, the first denotes how many values are available. Followed by a list of the actual `unsigned int` values. For each time the given option occurs an item is popped from the list and a bitwise OR operation is done with the value of the target variable. This can be useful if you want to set values in a bit mask for instance.
- `&` Works the same as `|` except that an bitwise AND is performed on the target value.
- `+` Same as `|` except that an addition is made on the target value for each value in the list.
- `_` Same as `|` except that for each repeat of the flag, the next value in the list overwrites the previous.

Some examples:

- Parse an ordinary flag and store `1` in `val` if set: `b, &val`.
  `"--opt"` -> `val = 1`
- Or parse the flag but store `5` in `val` if set: `"b=", &val, 5`
  `"--opt"` -> `val = 5`
- Or count the number of occurrances of the flag: `"b!", &val`
  `"--opt --opt --opt"` -> `val = 3`
- Or do a bitwise OR operation for each occurance of the flag: `"b|", &val, 3, (1 << 1), (1 << 3), (1 << 5)`
  `"--opt --opt"` -> `val = (1 << 1) | (1 << 3) = 10`
- Same as above but in a more easy to grasp use case:

```c
typedef enum debug_level_e
{
   NONE  = 0,
   ERROR = (1 << 0),
   WARN  = (1 << 1),
   INFO  = (1 << 2),
   DEBUG = (1 << 3)
} debug_level_t;

...
debug_level_t debug_level = NONE;

cargo_add_option(cargo, 0, "--verbosity -v", "Verbosity level",
                 "b|", &debug_level, 4, ERROR, WARN, INFO, DEBUG);

```

So `"-vvv"` would give a debug level of `INFO` (including `ERROR` and `WARN`).

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

For this reason, strings are handled in a special way. By default if you parse a single argument of any given value type, such as `i`, `f`, `d`, the parsed result will simply be stored "by value" in the target variable. However when it comes to strings, the default when specifiying the `s` format character is to allocate the memory for that string.

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
char strs[5][32];
size_t count;
cargo_add_option(cargo, 0, "--opt", "description",
                 ".[s#]#", &strs, 32, &count, 5);
```

An allocated list of allocated strings:

```c
char **strs = NULL;
size_t count;
cargo_add_option(cargo, 0, "--opt", "description",
                 "[s]+", &strs, &count);
```

You can also restrict the length of allocated strings:

```c
char **strs = NULL;
size_t count;
cargo_add_option(cargo, 0, "--opt", "description",
                 "[s#]+", &strs, 32, &count);
```

A bit more tricky case to parse is a static list that contains allocated strings:

```c
char *strs[5];
size_t count;
cargo_add_option(cargo, 0, "--opt", "description",
                 ".[s]#", &strs, &count, 5);
```

## Types ##

### cargo_t ###

This is the type of the cargo context. This type is opaque and should never be manipulated directly, only by using the cargo API functions.

To allocate a new instance [`cargo_init`](api.md#cargo_init) is used. And to destroy it use [`cargo_destroy`](api.md#cargo_destroy)


### cargo_type_t ###

This is an enum of the different types an option can be. This is only used
internally by the API. The reason this is a part of the public API is so that
it is possible to do some introspection.

### cargo_validation_t ###

This is a `struct` that defines a validation for an option. cargo comes with a set of existing validators, such as a range validator, and choices validator.

See [`cargo_add_validation`](api.md#cargo_add_validation) for details on how to add validation to an option.

To create your own validator you can create a function that returns an allocated version of this `struct` with a [`cargo_validation_f`](api.md#cargo_validation_f)) callback function as well as a [`cargo_validation_destroy_f`](api.md#cargo_validation_destroy_f) callback for cleaning up.

The validator instance you create will get passed to the validator when an argument value is being validated.

To add extra context to this `struct` you can simply put it at the top of your own struct:

```
typdef struct cargo_some_validation_s
{
  cargo_validation_t super;
  int something;
  char *other;
} cargo_some_validation_t;
```

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
cargo_init(&cargo, 0, "%s", argv[0]);
cargo_add_option(cargo, 0, "--opt", "Some string", "s", &str);
cargo_destroy(&cargo);
if (str) free(str); // We must free!
```

Autoclean:

```c
char *str = NULL;
cargo_t cargo;
cargo_init(&cargo, 0, "%s", argv[0]);
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

This flags turns of the printing of the short usage on error.

Note if you simply want to customize the usage output printed by cargo on internal errors you can set the usage flags using [`cargo_set_internal_usage_flags`](api.md#cargo_set_internal_usage_flags).

#### `CARGO_STDOUT_ERR` ####
cargo prints errors to `stderr` by default. This flag changes so that it prints to `stdout` instead.

#### `CARGO_NO_AUTOHELP` ####
This flag turns off the automatic creating of the `--help` option.

#### `CARGO_NO_FAIL_UNKNOWN` ####
Don't fail the parse when unknown options are found, simply add them to the list of unknown options.

You can still get a list of the unknown options using [`cargo_get_unknown`](api.md#cargo_get_unknown).

#### `CARGO_UNIQUE_OPTS` ####
By default if an options is specified more than once, the last value for the last occurrance is the one that counts, and will override anything specified earlier.

This option will instead give an error if any option is specified more than once (except special cases for boolean options).

This can be set on a per option basis as well using [`CARGO_OPT_UNIQUE`](api.md#cargo_opt_unique).

#### `CARGO_NOWARN` ####
Don't show warnings.

For example when [`CARGO_UNIQUE_OPTS`](api.md#cargo_unique_opts) is not set and an option is specified more than once, a warning will be shown that the first value is ignored. This suppresses this.

#### `CARGO_UNKNOWN_EARLY` ####
When parsing arguments cargo will by default do it in this order:

- Go through all arguments and try to parse them.
- Check for unknown options and fail if they're found.

This option instead moves this check to **before** the parsing is performed:

- Check for unknown options and fail if they're found.
- Go through all arguments and try to parse them.

Note that since we parse the arguments after we check for unknown options, using the option flag [`CARGO_OPT_STOP`](api.md#cargo_opt_stop) will work differently in regards to unknown options. Options found after the stop point will still be processed during the unknown check.

#### `CARGO_DEFAULT_LITERALS` ####
This enables string literals to be used as default values for all string options.

See [`CARGO_OPT_DEFAULT_LITERAL`](api.md#cargo_opt_default_literal) for details.

### cargo_usage_t ###

This is used to specify how the usage is output. These flags are used by the [`cargo_get_usage`](api.md#cargo_get_usage) function and friends.

#### `CARGO_USAGE_FULL` ####
Show the full usage. This is the default, same as specifying `0`.

Note that this includes the short usage as well. If you want the full usage but excluding the short usage you can use [`CARGO_USAGE_HIDE_SHORT`](api.md#cargo_usage_hide_short)

#### `CARGO_USAGE_SHORT` ####
Show only the short usage.

#### `CARGO_USAGE_RAW_DESCRIPTION` ####
The description passed to [`cargo_init`](api.md#cargo_init) or set using [`cargo_set_description`](api.md#cargo_set_description) will be displayed as is, and no automatic formatting is done by cargo.

#### `CARGO_USAGE_RAW_OPT_DESCRIPTIONS` ####
All option descriptions will be treated as raw. Note that this can be set on a per option basis as well using [`CARGO_OPT_RAW_DESCRIPTION`](api.md#cargo_opt_raw_description). cargo will not perform any automatic formatting on the option descriptions.

#### `CARGO_USAGE_RAW_EPILOG` ####
The epilog (text after all option descriptions) set using [`cargo_set_epilog`](api.md#cargo_set_epilog) will be displayed as is, and no automatic formatting is done by cargo.

#### `CARGO_USAGE_HIDE_DESCRIPTION` ####
Hides the description.

#### `CARGO_USAGE_HIDE_EPILOG` ####
Hides the epilog.

#### `CARGO_USAGE_HIDE_SHORT` ####
Hide the short usage information but show the rest.

#### `CARGO_USAGE_OVERRIDE_SHORT` ####
The program name specified in [`cargo_init`](api.md#cargo_init) overrides the automatically generated short usage.

#### `CARGO_USAGE_NO_STRIP_PROGNAME` ####
By default the program name passed to [`cargo_init`](api.md#cargo_init) will have the path stripped from it so that `/usr/bin/program` becomes simple `program`.

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

#### `CARGO_OPT_STOP` ####
Settings this flag for an option will cause the [`cargo_parse`](api.md#cargo_parse) to stop parsing any further arguments after the first occurance of that option in a given argument list.

To get the index the parser stopped at you can use [`cargo_get_stop_index`](api.md#cargo_get_stop_index).

Note that it will still process all remaining arguments, but it will not try to parse them as options or consume any of the values. Instead all remaining arguments are put in the **extra arguments** list that can be fetched [`cargo_get_args`](api.md#cargo_get_args) together with other remaining arguments.

Any options after the stop point won't show up in the unknown options list either, unless the [`CARGO_UNKNOWN_EARLY`](api.md#cargo_unknown_early) flag is used.

This can be useful if you're only parsing part of the arguments using one parser, and then want to pass the remaining arguments on to another parser to proccess the rest of the arguments. Simply pass the same `argv` and then use the `stop` index as the `start` index for the second parser.

#### `CARGO_OPT_STOP_HARD` ####
Must be combined with [`CARGO_OPT_STOP`](api.md#cargo_opt_stop) to work.

This will skip any checks for required variables after the parsing has been stopped.

This could be useful if you have something like `--advanced_help` where you simply want to show extended help, without having to specify any required variables.

For instance you could have a option group hidden by default using [`CARGO_GROUP_HIDE`](api.md#cargo_group_hide), and then on `--advanced_help` show it by removing that flag before showing usage.

#### `CARGO_OPT_HIDE` ####
Hides the option in the usage. See [`cargo_get_usage`](api.md#cargo_get_usage).

#### `CARGO_OPT_HIDE_SHORT` ####
Hides the option in the short usage. See [`cargo_get_usage`](api.md#cargo_get_usage).

#### `CARGO_OPT_DEFAULT_LITERAL` ###
Default values for strings normally needs to be a heap allocated string. However the more natural thing might be to simply use a string literal.

This enables you to use string literals as the default value for string options.

Note that this must be used in conjuction with [`CARGO_AUTOCLEAN`](api.md#cargo_autoclean). To set this behaviour for all options instead, use [`CARGO_DEFAULT_LITERALS`](api.md#cargo_default_literals)

See [default values](api.md#default-values) for more details and examples.

### cargo_mutex_group_flags_t ###

These flags control how a mutex group created using [`cargo_add_mutex_group`](api.md#cargo_add_mutex_group) behaves.

#### `CARGO_MUTEXGRP_ONE_REQUIRED` ####
By default none of the options in a mutex group is required.

This flag will require that at one of the members of the group is specified (but only one of course), otherwise an error is given.

Note that you probably want to make sure that the [`CARGO_OPT_NOT_REQUIRED`](api.md#cargo_opt_not_required) flag is set for all options that are part of the mutex group, otherwise you will get conflicting requirements.

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

#### `CARGO_MUTEXGRP_ORDER_BEFORE` ####
This flag enables you to force the order a set of options is parsed in. The first option added to this group is special. Any options added after it must be specified before it on the command line.

So if you have `--alpha --beta --centauri --delta` and add `--alpha` as the first option to the mutex group `mutex1` and set this flag on it. Any additional options added to that group must always be parsed before `--alpha`.

```c
int a = 0;
int b = 0;
int c = 0;
int d = 0;

cargo_add_mutex_group(cargo, CARGO_MUTEXGRP_ORDER_BEFORE,
                            "mutex1", "Mutex group 1", NULL);

cargo_add_option(cargo, 0, "<!mutex1> --alpha -a", "Description", "b", &a);
cargo_add_option(cargo, 0, "<!mutex1> --beta -b", "Description", "b", &b);
cargo_add_option(cargo, 0, "<!mutex1> --centauri -c", "Description", "b", &c);
cargo_add_option(cargo, 0, "--delta -d", "Description", "b", &d);

cargo_parse(cargo, 0, 1, argc, argv);
```

So if you input `"--alpha --beta --centauri --delta"`:

```bash
Usage: program [--help HELP] [--alpha] [--beta] [--centauri] [--delta]
--alpha --beta --centauri --delta
^^^^^^^ ~~~~~~ ~~~~~~~~~~
These options must all be specified before "--alpha":
--beta, --centauri
```

#### `CARGO_MUTEXGRP_ORDER_AFTER` ####
Same as [`CARGO_MUTEXGRP_ORDER_BEFORE`](api.md#cargo_mutexgrp_order_after) except that the rest of the variables in the mutex group must be parsed after the first one.


### cargo_group_flags_t ###

These flags are used to specify the behaviour of groups added using [`cargo_add_group`](api.md#cargo_add_group)

#### `CARGO_GROUP_HIDE` ####
This hides the group in the usage.

A use case for this might be an `--advanced_help` that unhides the group and prints the usage.

#### `CARGO_GROUP_RAW_DESCRIPTION` ####
This turns of any automatic formatting for the group description.


### cargo_parse_result_t ###

This is the [`cargo_parse`](api.md#cargo_parse) return values. This is different from most other API return values since knowing the reason that a parse failed is usually important to be able to show a relevant error message.

You can also get the internal error message that cargo displays by default by using [`cargo_get_error`](api.md#cargo_get_error).

#### (0) `CARGO_PARSE_OK` ####
For a successful parse this is returned.

#### (-1) `CARGO_PARSE_UNKNOWN_OPTS` ####
If the parse fails because there are unknown options in the given command line this will be returned.

Unknown options are defined as arguments prepended with a [prefix](api.md#cargo_set_prefix) character that was not added using [`cargo_add_option`](api.md#cargo_add_option).

You can get the list of unknown options found using [`cargo_get_unknown`](api.md#cargo_get_unknown).

Note that you can tell cargo not to fail on unknown options by setting the flag [`CARGO_NO_FAIL_UNKNOWN`](api.md#cargo_no_fail_unknown).

#### (-2) `CARGO_PARSE_NOMEM` ####
If cargo runs out of memory this will be returned when parsing.

#### (-3) `CARGO_PARSE_FAIL_OPT` ####
When cargo fails to parse an option for some reason.

#### (-4) `CARGO_PARSE_MISS_REQUIRED` ####
If any required option is missing.

See [`CARGO_OPT_REQUIRED`](api.md#cargo_opt_required) and [`CARGO_OPT_NOT_REQUIRED`](api.md#cargo_opt_not_required).

#### (-5) `CARGO_PARSE_MUTEX_CONFLICT` ####
When a mutex group conflict occurs. Either that at least one option is required in a mutex group. Or that more than one in the mutex group has been specified at the same time.

See [`CARGO_MUTEXGRP_ONE_REQUIRED`](api.md#cargo_mutexgrp_one_required)

#### (-6) `CARGO_PARSE_MUTEX_CONFLICT_ORDER` ####
An order mutex group rule has been broken.

See [`CARGO_MUTEXGRP_ORDER_BEFORE`](api.md#cargo_mutexgrp_order_before) and See [`CARGO_MUTEXGRP_ORDER_AFTER`](api.md#cargo_mutexgrp_order_before)

#### (-7) `CARGO_PARSE_OPT_ALREADY_PARSED` ####
If an option has already been parsed before (specified more than once) and either [`CARGO_OPT_UNIQUE`](api.md#cargo_opt_unique) for the option is set. Alternatively if [`CARGO_UNIQUE_OPTS`](api.md#cargo_unique_opts) is set.

#### (-8) `CARGO_PARSE_CALLBACK_ERR` ####
If a custom option user callback parses an option and indicates that an error has occurred.

### cargo_err_flags_t ###

These are flags for the [`cargo_set_error`](api.md#cargo_set_error) function.

#### `CARGO_ERR_APPEND` ####
Append to the error string instead of overwriting it.

### cargo_width_flags_t ###
These flags are used with [`cargo_get_width`](api.md#cargo_get_width) that fetches the width of the usage/console.

#### `CARGO_WIDTH_USED` ####
Return the width that is used internally by cargo.

#### `CARGO_WIDTH_RAW` ####
Return the raw console width as reported by the operating system.

Note that this may fail and `-1` will be returned instead.


### cargo_validation_flags_t ###

Flags for [`cargo_add_validation`](api.md#cargo_add_validation). Currently not used.

#### CARGO_VALIDATION_NONE ####

### cargo_validate_choices_flags_t ###

Flags for the validation function 

#### `CARGO_VALIDATE_CHOICES_NONE` ####
Same as 0, no flags.

#### `CARGO_VALIDATE_CHOICES_CASE_SENSITIVE` ####
When validating a list of string choices, this makes the comparison case sensitive.

#### `CARGO_VALIDATE_CHOICES_SET_EPSILON` ####
When alidating `float` or `double` a function to compare values *near* to the list of choices with a given epsilon. Det default value is `CARGO_DEFAULT_EPSILON`. But if this flag is set, the first value in the argument list is instead a new epsilon value that overrides the default.

## Functions ##

Here you find the core API for cargo.

### cargo_init ###

```c
int cargo_init(cargo_t *ctx, cargo_flags_t flags, const char *progname_fmt, ...);
```

Initializes a [`cargo_t`](api.md#cargo_t) context. See [Initializing cargo](api.md#initializing_cargo) for an example. You need to free this context using [`cargo_destroy`](api.md#cargo_destroy).

The program name passed will be displayed in the short usage part. You can format this using `printf` formatting. This can also be set using [`cargo_set_progname`](api.md#cargo_set_progname).

If you want to specify your own short usage instead of the one cargo generates automatically you can set the usage_flag [`CARGO_USAGE_OVERRIDE_SHORT`](api.md#cargo_usage_override_short) when calling [`cargo_get_usage`](api.md#cargo_get_usage) and friends, as well as for [`cargo_set_internal_usage_flags`](api.md#cargo_set_internal_usage_flags).

---

**Return value**: -1 on fatal error. 0 on success.

**ctx**: A pointer to a [`cargo_t`](api.md#cargo_t) context.

**flags**: Flags for setting global behavior for cargo. See [`cargo_flags_t`](api.md#cargo_flags_t).

**progname**: The name of the executable. Usually this will be set to `argv[0]`. But this is also a printf style formatting string for more advanced uses.

**...**: Formatting arguments for `progname`.

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

Note that you can have max [`CARGO_NAME_COUNT - 1`](api.md#cargo_name_count) aliases for an option name.

Also note that this is usually best done directly when calling [`cargo_add_option`](api.md#cargo_add_option) instead.

You cannot add aliases to positional arguments (options starting without a prefix character).

### cargo_add_group ###

```c
int cargo_add_group(cargo_t ctx,
                    cargo_group_flags_t flags,
                    const char *name,
					          const char *title,
                    const char *description, ...)
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_group_flags_t`](api.md#cargo_group_flags_t) for flags.

**name**: The group name used to identify the group, `group_name`.

**title**: The title shown in the usage output `Group Name`.

**description**: printf style format string for the description shown in the usage for the group.

**...**: Format arguments.

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
            const char *description, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: See [`cargo_mutex_group_flags_t`](api.md#cargo_mutex_group_flags_t) for flags.

**name**: The name of the mutex group.

**title**: The title of the mutex group.

**description**: printf style format string for description of the mutex group.

**...**: Formatting arguments.

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
int cargo_mutex_group_set_metavar(cargo_t ctx,
                  const char *mutex_group,
                  const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the mutex group.

**fmt**: printf style format string for the meta variable name.

**...**: Formatting arguments.

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

### cargo_mutex_group_set_metavarv ###

```c
int cargo_mutex_group_set_metavarv(cargo_t ctx,
                   const char *mutex_group,
                   const char *fmt, va_list ap);
```

Variadic version of [`cargo_mutex_group_set_metavar`](api.md#cargo_mutex_group_set_metavar).


### cargo_set_option_description ###

```c
int cargo_set_option_description(cargo_t ctx,
                 char *optname, const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**optname**: The option name you want to set the metavar for.

**fmt**: Printf format string.

**...**: Variable arguments for printf.

---

This sets an options description with printf formatting avaialable.

### cargo_set_option_descriptionv ###

```c
int cargo_set_option_descriptionv(cargo_t ctx,
                  char *optname, const char *fmt, va_list ap);
```

Variadic version of [`cargo_set_option_description`](api.md#cargo_set_option_description).

### cargo_set_metavar ###

```c
int cargo_set_metavar(cargo_t ctx,
          const char *optname,
          const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**optname**: The option name you want to set the metavar for.

**fmt**: printf style format string for the meta variable name.

**...**: Formatting arguments.

---

Change the default "metavar" that is shown in the usage output to identify the values for an option.

The default is to simply use the option name in uppercase:

```c
--option OPTION
```

So you can change this to whatever you want:

```c
ret = cargo_set_metavar(cargo, "--option", "THE%s", "VALUE");
```

Which gives:

```c
--option THEVALUE
```

Note that this function allows you to use `printf` formatting.

### cargo_set_metavarv ###

```c
int cargo_set_metavarv(cargo_t ctx,
          const char *optname,
          const char *fmt, va_list ap);
```

Variadic version of [`cargo_set_metavar`](api.md#cargo_set_metavar).


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

By default on an error only the short usage is shown together with the error. If you want the long error you would set [`CARGO_USAGE_FULL`](api.md#cargo_usage_full) flag here. Or any of the [`cargo_usage_t`](api.md#cargo_usage_t) flags to customize the output.

### cargo_parse ###

```c
cargo_parse_result_t cargo_parse(cargo_t ctx, cargo_flags_t flags,
                                 int start_index, int argc, char **argv);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: These flags will override the global flags set in [`cargo_init`](api.md#cargo_init) if non-zero.

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
ret = cargo_parse(cargo, 0, 1, argc, argv);
```

By default cargo will try to parse the arguments it is given, and if there is an error it will output it to `stderr` including a short usage message.

If you want to override this behaviour, you can change this behaviour by setting the [`cargo_flags_t`](api.md#cargo_flags_t).

You can turn it off completely and instead use [`cargo_get_usage`](api.md#cargo_get_usage), [`cargo_get_error`](api.md#cargo_get_error) and [`cargo_get_unknown`](api.md#cargo_get_unknown) to customize the output however you want.

The return value for this is more specific and contains different reasons found in the [`cargo_parse_result_t`](api.md#cargo_parse_result_t) enum. If the parse was successful, [`CARGO_PARSE_OK`](api.md#0-cargo_parse_ok) defined as `0` is returned.

**Return value**
The return values for this function is defined in the [`cargo_parse_result_t`](api.md#cargo_parse_result_t) enum.

For errors a negative value is always returned, however instead of simply using `-1` for all errors like the rest of the API, there are different values for each error type.

For example if the reason for the failed parse is that unknown options where found, [`CARGO_PARSE_UNKNOWN_OPTS`](api.md#-1-cargo_parse_unknown_opts) will be returned, and you can use that knowledge to get the list of unknown options using [`cargo_get_unknown`](api.md#cargo_get_unknown).

Note that by default cargo adds a `--help` option. When this is specified in a command line cargo will return [`CARGO_PARSE_SHOW_HELP`](api.md#cargo_parse_show_help) which is defined as `1`, so that you know that you should quit the program even though no error occurred. This will not happen if the [`CARGO_NO_AUTOHELP`](api.md#cargo_no_autohelp) flag is set in [`cargo_init`](api.md#cargo_init).


### cargo_set_prefix ###

```c
void cargo_set_prefix(cargo_t ctx, const char *prefix_chars);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**prefix_chars**: A string containing the prefix characters.

---

This will set the prefix characters that cargo will use. By default this is set to [`CARGO_DEFAULT_PREFIX`](api.md#cargo_default_prefix) which is `"-"` unless it has been overriden in `"cargo_config.h"`.

For instance you can allow both `"-"` and `"+"` by setting this to `"-+"`. So then you can add an option such as `"--option"` or `"++option"`.

### cargo_set_max_width ###

```c
void cargo_set_max_width(cargo_t ctx, size_t max_width);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**max_width**: The max width for the usage output.

---

Sets the max width that the usage output must fit inside. By default this is set to [`CARGO_AUTO_MAX_WIDTH`](api.md#cargo_auto_max_width) or `0`. In this mode cargo will attempt to set the max width to the current width of the console it is running in.

If it fails to get the console width from the operating system it will fall back to using [`CARGO_DEFAULT_MAX_WIDTH`](api.md#cargo_default_max_width) which is `80` characters unless it has been overridden.

The max width allowed for this is [`CARGO_MAX_MAX_WIDTH`](api.md#cargo_max_max_width).

### cargo_get_width ###

```c
int cargo_get_width(cargo_t ctx, cargo_width_flags_t flags);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: [`cargo_width_flags_t`](api.md#cargo_width_flags_t) flags.

---

This will return the max width for the usage that is used by cargo. This always returns a positive value. See [`cargo_set_max_width`](api.md#cargo_set_max_width) for details.

If you instead want the raw console width that the OS reports you can use the flag [`CARGO_WIDTH_RAW`](api.md#cargo_width_raw). Note that this may fail and will in that case return -1.


### cargo_set_progname ###

```c
void cargo_set_progname(cargo_t ctx, const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**fmt**: Format string for the usage program name.

**...**: Format arguments.

---

Sets the program name used in the short usage message.

This is the same as you can set using [`cargo_init`](api.md#cargo_init).

Note that by default this will be followed by the automatically generated option usage, unless [`CARGO_USAGE_OVERRIDE_SHORT`](api.md#cargo_usage_override_short) is specified. See [`cargo_usage_t`](api.md#cargo_usage_t) for more usage flags.

### cargo_set_prognamev ###

```c
void cargo_set_prognamev(cargo_t ctx, const char *fmt, va_list ap);
```

Variadic version of [`cargo_set_progname`](api.md#cargo_set_progname).

### cargo_set_description ###

```c
void cargo_set_description(cargo_t ctx, const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**fmt**: Format string for the usage description.

**...**: Format arguments.

---

This sets the description shown first in the usage output, before the list of options.

### cargo_set_descriptionv ###

```c
void cargo_set_descriptionv(cargo_t ctx, const char *fmt, va_list ap);
```

Variadic version of [`cargo_set_description`](api.md#cargo_set_description).

### cargo_set_epilog ###

```c
void cargo_set_epilog(cargo_t ctx, const char *fmt, ...);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**fmt**: printf style formatting for the usage epilog.

**...**: Formatting arguments.

---

This sets the epilog, the text shown after the list of options.

### cargo_set_epilogv ###

```c
void cargo_set_epilogv(cargo_t ctx, const char *fmt, va_list ap);
```

Variadic verison of [`cargo_set_epilog`](api.md#cargo_set_epilog)

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

### cargo_set_error ###

```c
void cargo_set_error(cargo_t ctx,
                     cargo_err_flags_t flags,
                     const char *fmt, ...)
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: Flags [`cargo_err_flags_t`](api.md#cargo_err_flags_t). Not used at the moment.

**fmt**: Format string, same as for `printf`.

**...**: Variable arguments for format string.

---

This is meant to be used inside of [`custom callback`](api.md#cargo_custom_f) function to set errors when parsing custom values.

### cargo_set_errorv ###

```c
void cargo_set_errorv(cargo_t ctx,
                      cargo_err_flags_t flags,
                      const char *fmt,
                      va_list ap)
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: Flags [`cargo_err_flags_t`](api.md#cargo_err_flags_t). Not used at the moment.

**fmt**: Format string, same as for `printf`.

**ap**: Variable arguments for format string.

---

Varargs version of [`cargo_set_error`](api.md#cargo_set_error).

### cargo_get_error ###

```c
const char *cargo_get_error(cargo_t ctx);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

---

This will return any error that was set by [`cargo_parse`](api.md#cargo_parse) in the last call to it. This is useful if you want to customize exactly how the error is shown. By default this will be printed to `stderr`. See [`cargo_flags_t`](api.md#cargo_flags_t) to turn that behaviour off.

Please note that cargo is responsible for freeing this string, so if you want to keep it make sure you create a copy.

### cargo_get_stop_index ###

```c
int cargo_get_stop_index(cargo_t ctx);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

---

Gets the index where the parse was stopped. This will either be the end index passed in `argc` or it can be the index of an option with the [`CARGO_OPT_STOP`](api.md#cargo_opt_stop) flag set.

This can be useful when using multiple parsers, or simply wanting to stop parsing for some other reason. See details [`CARGO_OPT_STOP`](api.md#cargo_opt_stop).

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

Note that if you call [`cargo_parse`](api.md#cargo_parse) again the pointer returned by this will become invalid.

### cargo_get_unknown_copy ###

```c
const char **cargo_get_unknown_copy(cargo_t ctx, size_t *unknown_count);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**unknown_count**: A pointer to a `size_t` where the number of unknown options passed to [`cargo_parse`](api.md#cargo_parse) will be returned.

---

Same as [`cargo_get_unknown`](api.md#cargo_get_unknown) except that it returns a copy of the list. It's the callers responsibility to clean this up. There's a helper function for this [`cargo_free_commandline`](api.md#cargo_free_commandline).


### cargo_get_args ###

```c
const char **cargo_get_args(cargo_t ctx, size_t *argc);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**argc**: A pointer to a `size_t` where the number of arguments passed to [`cargo_parse`](api.md#cargo_parse) that were not consumed is retruned.

---

This will return any remaining arguments left after [`cargo_parse`](api.md#cargo_parse) has parsed the arguments passed to it.

### cargo_get_args_copy ###

```c
char **cargo_get_args_copy(cargo_t ctx, size_t *argc);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**argc**: A pointer to a `size_t` where the number of arguments passed to [`cargo_parse`](api.md#cargo_parse) that were not consumed is retruned.

---

Same as [`cargo_get_args`](api.md#cargo_get_args) except that it returns a copy of the list. It's the callers responsibility to clean this up. There's a helper function for this [`cargo_free_commandline`](api.md#cargo_free_commandline).

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

This can be used to pass your own group context to a [`cargo_custom_f`](api.md#cargo_custom_f) when parsing a custom argument.

### cargo_get_group_context ###

```c
void *cargo_get_group_context(cargo_t ctx, const char *group);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**group**: The name of the group to get the context for. If this is `NULL`, the context is set for the default group that all options are added to by default.

---

This can be used to fetch the context or a given option group when parsing custom arguments in a [`cargo_custom_f`](api.md#cargo_custom_f) callback function.

To set this context for a group see [`cargo_set_group_context`](api.md#cargo_set_group_context).

Since you are not passed an options group name in the [`cargo_custom_f`](api.md#cargo_custom_f) callback, you can use [`cargo_get_option_group`](api.md#cargo_get_option_group) to get it given the option name.

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

### cargo_get_option_type ###

```c
cargo_type_t cargo_get_option_type(cargo_t ctx, const char *opt);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**opt**: The option name to get the list of mutex groups for.

---

This returns the [`cargo_type_t`](api.md#cargo_type_t) type of a given option.

If the option name is invalid -1 is returned.

### cargo_add_validation ###

```c
int cargo_add_validation(cargo_t ctx, cargo_validation_flags_t flags,
                         const char *opt, cargo_validation_t *vd);
```

---

**ctx**: A [`cargo_t`](api.md#cargo_t) context.

**flags**: The [`cargo_validation_flags_t`](api.md#cargo_validation_flags_t)s to use.

**opt**: The option name.

**vd**: Pointer to an allocated [`cargo_validation_t`](api.md#cargo_validation_t) instance. This will be freed by cargo.

---

This can be used to add validation to an option. Of course this is optional, and you can simply do the validation manually yourself. However this exists so that you can get a more integrated error handling, which lets cargo highlight the problematic argument and other nice things.

To create the [`cargo_validation_t`](api.md#cargo_validation_t), cargo comes with a set of built-in validations.

For validating a range of numbers:

- [`cargo_validate_int_range`](api.md#cargo_validate_int_range)
- [`cargo_validate_uint_range`](api.md#cargo_validate_uint_range)
- [`cargo_validate_longlong_range`](api.md#cargo_validate_uint_range)
- [`cargo_validate_ulonglong_range`](api.md#cargo_validate_uint_range)
- [`cargo_validate_float_range`](api.md#cargo_validate_uint_range)
- [`cargo_validate_double_range`](api.md#cargo_validate_uint_range)

For validating that a variable is one of a set of choices:

- [`cargo_validate_choices`](api.md#cargo_validate_choices)

### cargo_create_validator ###

```c
cargo_validation_t *cargo_create_validator(const char *name,
                       cargo_validation_f validator,
                       cargo_validation_destroy_f destroy,
                       cargo_type_t types,
                       void *user);
```

---

**name**: Name for the validator (used internally for error message only).

**validator**: [`cargo_validation_f`](api.md#cargo_validation_f) callback used for validating option values.

**destroy**: [`cargo_validation_destroy_f`](api.md#cargo_validation_destroy_f) callback used for cleaning up any allocated resources in the user context.

**types**: Supported types for this validator, this can contain a OR:ed list of [`cargo_type_t`](api.md#cargo_type_t) types.

**user**: The user context that is used when validating in the validator callback.

---

This is only meant to be used when specifying your own custom validator. The validator function that creates your validator should use this to specify the callbacks and context that is used.

The returned struct can then be passed to [`cargo_add_validation`](api.md#cargo_add_validation).

### cargo_validator_get_context ###

```c
void *cargo_validator_get_context(cargo_validation_t *validator);
```

---

**validator**: The validator that you want the context for.

---

This returns the user context pointer that was specified in [`cargo_create_validator`](api.md#cargo_create_validator)


### cargo_validate_int_range ###

```c
cargo_validation_t *cargo_validate_int_range(int min, int max);
```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_uint_range ###

```c
cargo_validation_t *cargo_validate_uint_range(int min, int max);
```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_longlong_range ###

```c
cargo_validation_t *cargo_validate_longlong_range(long long int min,
                                                  long long int max);

```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_ulonglong_range ###

```c
cargo_validation_t *cargo_validate_ulonglong_range(unsigned long long int min,
                                                   unsigned long long int max);
```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_float_range ###

```c
cargo_validation_t *cargo_validate_float_range(float min, float max,
                                               float epsilon);
```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

**epsilon**: Epsilon (max difference) to use when comparing. Default uses [`CARGO_DEFAULT_EPSILON`](api.md#cargo_default_epsilon)

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_double_range ###

```c
cargo_validation_t *cargo_validate_double_range(double min, double max,
                                                double epsilon);
```

---

**min**: Minimum value (inclusive) allowed in the range.

**max**: Maximum value (inclusive) allowed in the range.

**epsilon**: Epsilon (max difference) to use when comparing. Default uses [`CARGO_DEFAULT_EPSILON`](api.md#cargo_default_epsilon)

---

Validates a range for an option.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_validate_choices ###

```c
cargo_validation_t *cargo_validate_choices(
                      cargo_validate_choices_flags_t flags,
                      cargo_type_t type,
                      size_t count, ...);
```

---

**flags**: [`cargo_validate_choices_flags_t`](api.md#cargo_validate_choices_flags_t)

**type**: [`cargo_type_t`](api.md#cargo_type_t)

**count**: Number of choices that will follow in the `...` argument list.

**...**: The list of choices. As many as defined in `count`.

---

Validates that the option values specified are one of the values given in this list.

Note that since comparing floating point values can give a mismatch if an exact comparison is made, so an **epsilon** is used (max difference allowed between the values being compared).

By default [`CARGO_DEFAULT_EPSILON`](api.md#cargo_default_epsilon) is used. You can override this by setting the flag [`CARGO_VALIDATE_CHOICES_SET_EPSILON`](api.md#cargo_validate_choices_set_epsilon) and specifying the new **epsilon** as the first argument in the list of arguments.

For string lists use the [`CARGO_VALIDATE_CHOICES_CASE_SENSITIVE`](api.md#cargo_validate_choices_case_sensitive) for case sensitive comparison.

See [`cargo_add_validation`](api.md#cargo_add_validation).


### cargo_set_memfunctions ###

```c
void cargo_set_memfunctions(cargo_malloc_f malloc_replacement,
                            cargo_realloc_f realloc_replacement,
                            cargo_free_f free_replacement);
```

---

**malloc_replacement**: Replacement function for malloc.

**realloc_replacement**: Replacement function for realloc.

**free_replacement**: Replacement function for free.

---

This is used to change the memory allocation functions used by cargo.

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

### cargo_splitcmd_flags_t ###

Flags for the [`cargo_split_commandline`](api.md#cargo_split_commandline) function.

Currently this exist only to avoid breaking API changes in the future, by adding a flag to change any internal behaviour that might be needed.

#### `CARGO_SPLITCMD_DEFAULT` ####
Uses the OS specific version for splitting command lines in Windows and Unix.

## Utility functions ##

These are not a part of the core, but are nice to have.

### cargo_fprintf ###

```c
void cargo_fprintf(FILE *fd, const char *fmt, ...)
```

---

**fd**: File descriptor to print to.

**fmt**: Printf format string.

**...**: Format arguments.

---

This behaves just like the normal `fprintf` except that on **Windows** this also supports the ANSI color codes (Unix has native support for that).

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

cargo has a set of predefined colors with the `CARGO_COLOR_*` macros. But any ANSI color code can be used.

Note that cargo internally also supports outputting these ANSI colors on **Windows** which does not have native ANSI console color support normally.

If you prefer to return the output the result without any colors applied you can pass the [`CARGO_FPRINT_NOCOLOR`](api.md#cargo_fprint_nocolor) flag.

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
char **cargo_split_commandline(cargo_splitcmd_flags_t flags,
                               const char *args, int *argc);
```

---

**flags**: Currently does nothing, might change in the future. Use `0`.

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

This can be used to free and `NULL` an `argv` array that was split using [`cargo_split_commandline`](api.md#cargo_split_commandline).

Note that this function will both free and set `argv` to `NULL`.

```c
char **argv = NULL;
int argc;

argv = cargo_split_commandline(0, "some --command line", &argc);

...

// Free and NULL argv.
cargo_free_commandline(&argv, argc);
```

