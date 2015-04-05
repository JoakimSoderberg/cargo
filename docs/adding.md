Table of contents
=================

[TOC]

Initializing cargo
==================

The first thing you need to do is to setup a cargo instance. This is done by declaring a [`cargo_t`](api.md#cargo_t) struct. Note that this struct is opaque and meant to only be manipulated using the cargo API. This instance will be referred to as the **context** in the rest of this documentation.

You should always check the return value of any cargo API call.

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.
    ...
}
```

Before you can use this instance you need to initialize it using [`cargo_init`](api.md#cargo_init).

```c
int cargo_init(cargo_t *ctx, cargo_flags_t flags, const char *progname); 
```

The `progname` is the program name you want cargo to display in usage information and such. In most cases this will be `argv[0]`.

```c
cargo_init(&cargo, 0, argv[0]);
```

Similarly when you are done using cargo you need to free up any resources used by cargo via the [`cargo_destroy`](api.md#cargo_destroy) api call.

```c
void cargo_destroy(cargo_t *ctx);
```

Simply call:

```c
cargo_destroy(&cargo);
```

So in summary to setup, use and destroy the cargo instance. This time with error checking:

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.

    if (cargo_init(&cargo, 0, argv[0]))
    {
        return -1;
    }

    // Use the cargo api and run your program ...

    cargo_destroy(&cargo);
    return 0;
}
```

Adding options
==============
The entire point of cargo is that you give it a set of options that it should look for when parsing a given set of commandline arguments.

To do this you use the [`cargo_add_option`](api.md#cargo_add_option) function. As you can see the function takes a variable set of arguments just like `printf`, what these arguments contain depends on the contents of the `fmt` string.

```c
int cargo_add_option(cargo_t ctx, cargo_option_flags_t flags,
                     const char *optnames, const char *description,
                     const char *fmt, ...);
```

### Context

As with all of the cargo API it takes a [`cargo_t`](api.md#cargo_t) context instance as the first argument.

### Flags

The option `flags` let you set some specific flags for the option, such as it being required [`CARGO_OPT_REQUIRED`](api.md#CARGO_OPT_REQUIRED), or the opposite for *positional* arguments [`CARGO_OPT_NOT_REQUIRED`](api.md#CARGO_OPT_NOT_REQUIRED). See the [API reference](api.md#cargo_option_flags_t) for details.

### Option names

The `optnames` argument specifies the command line option name, for instance `"--myoption"`. You can also pass a set of aliases to this option, like this `"--myoption -m"`. You can add up to [`CARGO_NAME_COUNT`](api.md#CARGO_NAME_COUNT) (which defaults to 4) option names/aliases to an option, see [getting started](gettingstarted.md) for details on how to raise this if needed.

In the examples above the `'-'` character is referred to as the option **prefix**, in this example we use the default character as specified by [`CARGO_DEFAULT_PREFIX`](api.md#CARGO_DEFAULT_PREFIX). However if you want to support another prefix character you can change it using [`cargo_set_prefix`](api.md#cargo_set_prefix). It is possible to specify multiple prefix characters.

Adding an option that doesn't start with a prefix character will make it a **positional argument**. These are not allowed to have any aliases (that would make no sense anyway).

By default all *options* are **optional** and all *positional* arguments are **required**. It is possible to make either of these required/optional see the [API reference](api.md) for details.

### Description

The `description` is self explanatory. Note that this will be automatically formatted unless you explicitly set it not to via the  [`CARGO_FORMAT_RAW_OPT_DESCRIPTION`](api.md#CARGO_FORMAT_RAW_OPT_DESCRIPTION) flag. More on that in the [API reference](api.md#cargo_format_t)

### Format

Here is where you specify what this cargo option should look for when parsing the commandline arguments it is passed. The function takes a variable set of arguments. What those arguments should be is defined by the `fmt` argument, just like how `printf` works.

To make learning how this formatting language works as easy as possible, cargo comes with a small helper program `cargo_helper` that can help you with this. See [getting started](gettingstarted.md) on how to compile this.

For a full description of the formatting language, please see the [API reference](api.md#format). In short, you specify a type specifier such as `i` for and `int`, `s` for a string (`char *`), `d` for `double` and so on. As well as specifiers that tells us if you're parsing a list, a single value, one or more values etc.

Here's an example of using [`cargo_add_option`](api.md#cargo_add_option) to parse an integer:

```c
int integer = 5; // Default value should be 5.
...
ret = cargo_add_option(cargo, 0, "--integer -i", "An integer", "i", &integer);
assert(ret == 0); // This should never happen. Must be a bug!
```

Another example for adding an option that reads an array of integers. The `[]` around the `i` indicates that we are parsing a list of items. Parsing a list means we always have to give two arguments to cargo, first the destination of the list, in this case `int *integers`, as well as a variable that cargo can use to return the number of succesfully parsed items. This is given as a `size_t integer_count`.

The `+` at the end means that cargo should parse **one or more** items. The default behaviour is to allocate the list of items.

```c
int *integers;
size_t integer_count;
...
ret = cargo_add_option(cargo, 0, "--integers -i", "Integers", "[i]+",
                       &integers, &integer_count);
assert(ret == 0);
```

Other options instead of using [`+`](api.md#plus) is [`*`](api.md#star) and [`#`](api.md#hash).

`*` works the same as `+` and means **zero or more** items. `#` means that we should parse a fixed amount of items. To do this we must specify the max number of items as an argument as well, in this example `4`:

```c
int *integers;
size_t integer_count;
...
ret = cargo_add_option(cargo, 0, "--integers -i", "Integers", "[i]#",
                       &integers, &integer_count, 4);
assert(ret == 0);
```

When parsing using both [`*`](api.md#star) and [`+`](api.md#plus) the default behaviour of allocating the list on the heap makes sense. However, when you parse a list of a known size you often want to allocate it statically. To do this, you prepend the format string with a [`.`](api.md#.). So for example to parse the same `4` integers into a fixed size array:

```c
int integers[4]; // Fixed size.
size_t integer_count;
...
ret = cargo_add_option(cargo, 0, "--integers -i", "Integers", ".[i]#", // Dot
                       &integers, &integer_count, 4);
assert(ret == 0);
```

********************************************************************************
**Note:** A call to [`cargo_add_option`](api.md#cargo_add_option) should never fail. If it fails, either there is a bug in your code, the system is out of memory, or there is a bug in cargo itself. For this reason it is good to always `assert` that the return value is `0`. See [getting started](gettingstarted.md#debugging-cargo) for more information about debugging.
********************************************************************************

Option aliases
==============
As seen in the examples below, when calling [`cargo_add_option`](api.md#cargo_add_option) you usually pass any aliases with the option name, for example: `"--integers -i"`.

It is also possible to do this via a separate function call to [`cargo_add_alias`](api.md#cargo_add_alias):

```c
int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias);
```

Automatic --help option
=======================
People in need wants `--help`, so by default cargo will create this option for you. If you for some reason hate helping people you can turn this behaviour off by passing [`CARGO_NO_AUTOHELP`](api.md#CARGO_NO_AUTOHELP) to [`cargo_init`](api.md#cargo_init)

Groups
======
It is possible to tell cargo to group your options when printing usage, these groups can have a title and a description, and any options in them are displayed together. If an option doesn't have a group it is put in the default group that has no name which is displayed first.

To create a group you use [`cargo_add_group`](api.md#cargo_add_group):

```c
int cargo_add_group(cargo_t ctx, cargo_group_flags_t flags, const char *name,
                    const char *title, const char *description);
```

The only argument that is required is the `name`. If no `title` is given, the `name` is used instead. Also the `description` is optional.

Add options to groups
---------------------
You can add options to a group in two ways. The first is using the [`cargo_group_add_option`](api.md#cargo_group_add_option) function.

```c
int cargo_group_add_option(cargo_t ctx, const char *group, const char *opt);
```

This function simply takes a `group` name, as well as an `opt`ion name.

The other way to add an option to a group is to do it directly in the [`cargo_add_option`](api.md#cargo_add_option) call by prepending the group name enclodes in brackets, like this: `"<group1> --integers -i"

Here's an example of both methods:

```c
ret = cargo_add_option(cargo, 0, "<group1> --integers -i", 
                       "Integers", "[i]+",
                       &integers, &integer_count);
```

or with a separate function call:

```c
ret = cargo_add_option(cargo, 0, "--integers -i", 
                       "Integers", "[i]+",
                       &integers, &integer_count);

ret |= cargo_group_add_option(cargo, "group1", "--integers");
```

Mutually exclusive groups
=========================
If you have a set of options that cannot be used together, you can put them in a mutually exclusive group (mutex group).

For example, say you have three options `--alpha`, `--beta` and `--centauri`. The user will only be allowed to specify one of them at one time.

You can create a mutex group by using [`cargo_add_mutex_group`](api.md#cargo_add_mutex_group):

```c
int cargo_add_mutex_group(cargo_t ctx,
                        cargo_mutex_group_flags_t flags,
                        const char *name);
```

And then add options to it using [`cargo_mutex_group_add_option`](api.md#cargo_mutex_group_add_option):

```c
int cargo_mutex_group_add_option(cargo_t ctx,
                                const char *group,
                                const char *opt);
```

So here we create the three options and put them in a mutex group:

```c
int i = 0;
int j = 0;
int k = 0;
ret = cargo_add_mutex_group(cargo, 0, "mutex_group1");

ret |= cargo_add_option(cargo, 0, "--alpha", "The alpha", "i?", &j);
ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--alpha");

ret |= cargo_add_option(cargo, 0, "--beta -b", "The beta", "i", &i);
ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--beta");

ret |= cargo_add_option(cargo, 0, "--centauri", "The centauri", "i?", &k);
ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--centauri");

cargo_assert(ret == 0, "Failed to add options");
```

Here is the resulting output:

```
$ program --alpha --beta 123 456
Usage: program [--alpha ALPHA [ALPHA ...]] [--beta BETA]
               [--centauri CENTAURI [CENTAURI ...]] [--help ]
--alpha --beta 123 456
~~~~~~~ ~~~~~~
Only one of these variables allowed at the same time:
--alpha, --beta, --centauri
```

One required
------------

If you want to require that one of the options is always picked, you can pass the flag [`CARGO_MUTEXGRP_ONE_REQUIRED`](api.md#CARGO_MUTEXGRP_ONE_REQUIRED) to [`cargo_add_mutex_group`](api.md#cargo_add_mutex_group).

```c
ret = cargo_add_mutex_group(cargo,
                            CARGO_MUTEXGRP_ONE_REQUIRED,
                            "mutex_group1");
cargo_assert(ret == 0, "Failed to add mutex group");

ret |= cargo_add_option(cargo, 0, "--alpha", "The alpha", "i", &j);
ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--alpha");

ret |= cargo_add_option(cargo, 0, "--beta -b", "The beta", "i", &i);
ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--beta");

// Don't add this to the mutex group.
ret |= cargo_add_option(cargo, 0, "--centauri", "The centauri", "i?", &k);

cargo_assert(ret == 0, "Failed to add options");
```

And the resulting output if we don't at least specify one of `--alpha` or `--beta`:

```bash
$ program --centauri 123 456
Usage: program [--alpha ALPHA] [--beta BETA]
               [--centauri CENTAURI [CENTAURI ...]] [--help ]
One of these variables is required:
--alpha, --beta
```

Custom parsing
==============
As you have seen in the previous examples, parsing native types such as `int`, `float`, `double` or a string is possible. But what if you want to parse something more advanced, a timestamp for example?

Here's where custom callbacks come in to play. Instead of simply passing the address to the variable you want cargo to put whatever it parses into, you also specify a custom callback that cargo should call when it parses an option.

The callback function of type [`cargo_custom_cb_t`](api.md#cargo_custom_cb_t) looks like this:

```c
typedef int (*cargo_custom_cb_t)(cargo_t ctx, void *user, const char *optname,
                                 int argc, char **argv);
```

The `void *user` is a user defined pointer that will be passed on to the callback function by cargo.

`char *optname` is the option name of the variable being parsed.

`argc` and `argv` are the arguments that cargo has parsed for the option being parsed (it is not the entire command line). So for instance if cargo is given the command line `--alpha --beta 1 2 3 --sigma 4 5` and we're parsing the `--beta` option. Then `argc` will be `3` and `argv` will contain `[1, 2, 3]`.

Note that any memory you allocate in this callback, you will have to free, even if [`CARGO_AUTOCLEAN`](api.md#CARGO_AUTOCLEAN) is enabled. Since cargo does not have any knowledge where this is stored or how to free it.

Example
-------
So let's say we want to parse a rectangle value wher the dimensions are specified by the width and height in this format: `WxH`. And we want to put it in our struct `rect_t`:

```c
typedef struct rect_s
{
    int width;
    int height;
} rect_t;
```

So let use define a callback that parses such a rectangle:

```c
static int parse_rect_cb(cargo_t ctx, void *user, const char *optname,
                    int argc, char **argv)
{
    assert(ctx);
    assert(user);

    // We expect a rect_t structure to be passed to us.
    rect_t *u = (rect_ *)user;
    memset(u, 0, sizeof(rect_t));

    if (argc > 0)
    {
        if (sscanf(argv[0], "%dx%d", &u->width, &u->height) != 2)
        {
            return -1;
        }

        return 1; // We ate 1 argument.
    }

    return -1; // Error.
}
```

If an error occurs `-1` should be returned. Otherwise, how many of the arguments that you were given that were parsed should be returned.

So now we have a callback that can parse our rectangle. Let's use it. Instead of specifying the type such as `i` for `int` we instead add a `c` for **custom callback**.

As arguments we first pass our `parse_rect_cb` callback function, and then the `user` data we want cargo to pass to it at parse time.

```c
rect_t rect;

ret = cargo_add_option(cargo, 0, "--rect","The rect",
                        "c", parse_rect_cb, &rect);

ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
```

List example
------------
This works just like any other type, so you can of course parse a list of  values usign a custom callback as well, and using a `[c]+` format string for the example above. This time allocating an array of `rect_t`:s.

```c
static int _test_cb_array(cargo_t ctx, void *user, const char *optname,
                                int argc, char **argv)
{
    int i;
    assert(ctx);
    assert(user);
    rect_t **u = (rect_t **)user;
    rect_t *rect;

    if (!(*u = calloc(argc, sizeof(_test_data_t))))
    {
        return -1;
    }

    rect = *u;

    for (i = 0; i < argc; i++)
    {
        if (sscanf(argv[i], "%dx%d", &rect[i].width, &rect[i].height) != 2)
        {
            return -1;
        }
    }

    return i; // How many arguments we ate.
}
```

Help with format strings
========================
Ok, so you got the basics down on how to parse some integers for an option. However, learning some new formatting language kind of sucks.

But wait! There's more!... Let's use the `cargo_helper` program. (Again see [getting started](gettingstarted.md) for compilation help).

Here are the usage instructions:

```bash
$ bin/cargo_helper
bin/cargo_helper <variable declaration>
```

So let's use our earlier example with a list of integers, we simply pass `"int *integers"` as argument and we get two examples:

```bash
$ bin/cargo_helper "int *integers"
int *integers;
size_t integers_count;
cargo_add_option(cargo, 0, "--integers -i", "Description of integers", "[i]#",
        &integers, &integers_count, 128); // Allocated with max length 128.
cargo_add_option(cargo, 0, "--integers -i", "Description of integers", "[i]+",
        &integers, &integers_count); // Allocated unlimited length.
```

Or with a fixed array of `4` integers:

```bash
bin/cargo_helper "int integers[4]"
int integers[4];
size_t integers_count;
cargo_add_option(cargo, 0, "--integers -i", "Description of integers", ".[i]#",
                 &integers, &integers_count, 4);
```

Now let's try some more advanced things, like parsing strings.

An allocated string:

```bash
$ bin/cargo_helper "char *str"
char *str;
cargo_add_option(cargo, 0, "--str -s", "Description of str", "s", &str);
```

A fixed size string:

```bash
$ bin/cargo_helper "char str[32]"
char str[32];
cargo_add_option(cargo, 0, "--str -s", "Description of str", ".s", &str, 32);
```

A list of strings:

```bash
$ bin/cargo_helper "char **strs"
char **strs;
size_t strs_count;
cargo_add_option(cargo, 0, "--strs -s", "Description of strs", "[s]#",
                 &strs, &strs_count, 128); // Allocated with max length 128.
cargo_add_option(cargo, 0, "--strs -s", "Description of strs", "[s]+",
                 &strs, &strs_count); // Allocated unlimited length.
```





