Using cargo API
===============

The first thing you need to do is to setup a cargo instance. This is done
by declaring a `cargo_t` struct. Note that this struct is opaque and meant to
only be manipulated using the cargo API. This instance will be referred to as
the **context** in the rest of this documentation.

You should always check the return value of any cargo API call.

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.
    ...
}
```

Before you can use this instance you need to initialize it using `cargo_init`.

```c
int cargo_init(cargo_t *ctx, const char *progname); 
```

The `progname` is the program name you want cargo to display in usage
information and such. In most cases this will be `argv[0]`.

```c
cargo_init(&cargo, argv[0]);
```

Similarly when you are done using cargo you need to free up any resources used
by cargo via the `cargo_destroy` api call.

```c
void cargo_destroy(cargo_t *ctx);
```

Simply call:

```c
cargo_destroy(&cargo);
```

So in summary to setup, use and destroy the cargo instance. This time with
error checking:

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.

    if (cargo_init(&cargo, argv[0]))
    {
        return -1;
    }

    // Use the cargo api and run your program ...

    cargo_destroy(&cargo);
    return 0;
}
```

Adding options
--------------
The entire point of cargo is that you give it a set of options that it should
look for when parsing a given set of commandline arguments.

To do this you use the `cargo_add_option` function. As you can see the function
takes a variable set of arguments just like `printf`, what these arguments
contain depends on the contents of the `fmt` string.

```c
int cargo_add_option(cargo_t ctx, const char *optnames,
                     const char *description, const char *fmt, ...);
```

**Context**

As with all of the cargo API it takes a `cargo_t` context instance as the
first argument.

**Option names**

The `optnames` argument specifies the command line option name, for instance
`"--myoption"`. You can also pass a set of aliases to this option, like this
`"--myoption -m"`. You can add up to `CARGO_NAME_COUNT` (which defaults to 4)
option names/aliases to an option, see [getting started](gettingstarted.md)
for details on how to raise this if needed.

In the examples above the `'-'` character is referred to as the
option **prefix**, in this example we use the default character as specified
by `CARGO_DEFAULT_PREFIX`. However if you want to support another prefix
character you can change it using `cargo_set_prefix`. It is possible to specify
multiple prefix characters.

Adding an option that doesn't start with a prefix character will make it a
**positional argument**. These are not allowed to have any aliases (that would
make no sense anyway).

By default all *options* are **optional** and all *positional* arguments are
**required**. It is possible to make either of these required/optional
see the [API reference](api.md) for details.

**Description**

The `description` is self explanatory. Note that this will be automatically
formatted unless you explicitly set it not to via the 
`CARGO_FORMAT_RAW_OPT_DESCRIPTION` flag. More on that in the
[API reference](api.md)

**Format**

Here is where you specify what this cargo option should look for when parsing
the commandline arguments it is passed. The function takes a variable set of
arguments. What those arguments should be is defined by the `fmt` argument,
just like how `printf` works.

To make learning how this formatting language works as easy as possible, cargo
comes with a small helper program `cargo_helper` that can help you with this.
See [getting started](gettingstarted.md) on how to compile this.

For a full description of the formatting language, please see the
[API reference](api.md).

Here's an example of using `cargo_add_option` to parse an integer:

```c
int integer = 5; // Default value should be 5.
...
ret = cargo_add_option(cargo, "--integer -i", "An integer", "i", &integer);
assert(ret == 0); // This should never happen. Must be a bug!
```

Another example for adding an option that reads an array of integers.
The `[]` around the `i` indicates that we are parsing a list of items.
Parsing a list means we always have to give two arguments to cargo, first
the destination of the list, in this case `int *integers`, as well as a variable
that cargo can use to return the number of succesfully parsed items. This is
given as a `size_t integer_count`.

The `+` at the end means that cargo should parse **one or more** items.
The default behaviour is to allocate the list of items.

```c
int *integers;
size_t integer_count;
...
ret = cargo_add_option(cargo, "--integers -i", "Integers", "[i]+",
                       &integers, &integer_count);
assert(ret == 0);
```

Other options instead of using `+` is `*` and `#`.

`*` works the same as `+` and means **zero or more** items.
`#` means that we should parse a fixed amount of items. To do this we must
specify the max number of items as an argument as well, in this example `4`:

```c
int *integers;
size_t integer_count;
...
ret = cargo_add_option(cargo, "--integers -i", "Integers", "[i]#",
                       &integers, &integer_count, 4);
assert(ret == 0);
```

When parsing using both `*` and `+` the default behaviour of allocating the
list on the heap makes sense. However, when you parse a list of a known size
you often want to allocate it statically. To do this, you prepend the format
string with a `.`. So for example to parse the same `4` integers into a fixed
size array:

```c
int integers[4]; // Fixed size.
size_t integer_count;
...
ret = cargo_add_option(cargo, "--integers -i", "Integers", ".[i]#", // Added dot
                       &integers, &integer_count, 4);
assert(ret == 0);
```

********************************************************************************
**Note:** A call to `cargo_add_option` should never fail. If it fails, either 
      there is a bug in your code, the system is out of memory, or there is a
      bug in cargo itself. For this reason it is good to always `assert` that
      the return value is `0`.
      See [getting started](gettingstarted.md#debugging-cargo) for more
      information about debugging.
********************************************************************************

Help with format strings
------------------------
Ok, so you got the basics down on how to parse some integers for an option.
However, learning some new formatting language kind of sucks.

But wait! There's more!... Let's use the `cargo_helper` program.
(Again see [getting started](gettingstarted.md) for compilation help).

Here are the usage instructions:

```bash
$ bin/cargo_helper
bin/cargo_helper <variable declaration>
```

So let's use our earlier example with a list of integers, we simply pass
`"int *integers"` as argument and we get two examples:

```bash
$ bin/cargo_helper "int *integers"
int *integers;
size_t integers_count;
cargo_add_option(cargo, "--integers -i", "Description of integers", "[i]#",
        &integers, &integers_count, 128); // Allocated with max length 128.
cargo_add_option(cargo, "--integers -i", "Description of integers", "[i]+",
        &integers, &integers_count); // Allocated unlimited length.
```

Or with a fixed array of `4` integers:

```bash
bin/cargo_helper "int integers[4]"
int integers[4];
size_t integers_count;
cargo_add_option(cargo, "--integers -i", "Description of integers", ".[i]#",
                 &integers, &integers_count, 4);
```

Now let's try some more advanced things, like parsing strings.

An allocated string:

```bash
$ bin/cargo_helper "char *str"
char *str;
cargo_add_option(cargo, "--str -s", "Description of str", "s", &str);
```

A fixed size string:

```bash
$ bin/cargo_helper "char str[32]"
char str[32];
cargo_add_option(cargo, "--str -s", "Description of str", ".s", &str, 32);
```

A list of strings:

```bash
$ bin/cargo_helper "char **strs"
char **strs;
size_t strs_count;
cargo_add_option(cargo, "--strs -s", "Description of strs", "[s]#",
                 &strs, &strs_count, 128); // Allocated with max length 128.
cargo_add_option(cargo, "--strs -s", "Description of strs", "[s]+",
                 &strs, &strs_count); // Allocated unlimited length.
```





