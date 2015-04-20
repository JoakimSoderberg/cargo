Parsing
=======
After succesfully [adding](adding.md) the options you want cargo to parse, you are now ready to tell it to parse something.

Here is a summary of how to initialize, add options and finally destroy cargo when you're done. We now want to add the actual usual step using [`cargo_parse`](api.md#cargo_parse).

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.
    int ret = 0;

    int *integers = NULL;
    size_t integers_count = 0;

    if (cargo_init(&cargo, 0, argv[0]))
    {
        return -1;
    }

    ret = cargo_add_option(cargo, 0, "--integers -i", "Integers", "[i]+",
                           &integers, &integers_count); 
    assert(ret == 0);

    // Do the parsing here!

    cargo_destroy(&cargo);
    if (integers) free(integers);
    return 0;
}
```

Normal case
-----------
In the normal case you simply want cargo to parse the commandline you give it and present any errors or invalid input it is given in a nice and helpful way.

To do this we can start to look at how [`cargo_parse`](api.md#cargo_parse) looks like:

```c
int cargo_parse(cargo_t ctx, cargo_flags_t flags,
                int start_index, int argc, char **argv)
```

As usual a [`cargo_t`](api.md#cargo_t) context is given, and any C programmer is familiar with what `argc` and `argv` is.

However `start_index` specifies the index into `argv` cargo should start parsing at. In most cases this is `1`, since at index `0` we have the program name.

The `flags` are the same flags passed to [`cargo_init`](api.md#cargo_init). Any flags passed to this function that aren't `0` will override those ones during this call to [`cargo_parse`](api.md#cargo_parse).

```c
ret = cargo_parse(cargo, 0, 1, argc, argv);
```

Here is a full example:

```c
int main(int argc, char **argv)
{
    cargo_t cargo; // The cargo context instance.
    int ret = 0;

    int *integers = NULL;
    size_t integers_count = 0;

    if (cargo_init(&cargo, 0, argv[0]))
    {
        return -1;
    }

    ret = cargo_add_option(cargo, 0, "--integers -i", "Integers", "[i]+",
                           &integers, &integers_count); 
    assert(ret == 0);

    if ((ret = cargo_parse(cargo, 0, 1, argc, argv)))
    {
        // By default cargo will output a short usage message and
        // any errors that occur while parsing.
        return -1;
    }

    if (integers) free(integers);
    cargo_destroy(&cargo);
    return 0;
}
```

Freeing what cargo allocates
----------------------------
As you might have noticed in the example above, by default it is up to you to free any memory that cargo has allocated. This is so that you are free to write a function that creates a cargo instance, parses the command line, destroys the cargo instance and finally returns the parsed arguments.

One exception what is mentioend above is if you call [`cargo_parse`](api.md#cargo_parse) more than once on the same cargo instance in which case everything allocated by the previous parse is cleaned up before parsing again.

Usually however, parsing more than once with the same cargo instance does not make much sense.

If you want to override this behaviour, and make cargo to free these variables automatically in [`cargo_destroy`](api.md#cargo_destroy), you can do this by passing the [`CARGO_AUTOCLEAN`](api.md#cargo_autoclean) flag to [`cargo_init`](api.md#cargo_init).

Customizing parse output messages
---------------------------------
As mentioned above by default the short version of the usage is shown by default, as well as any error message that occurs while parsing.

If you instead want to output your own error message, or for whatever reason do things in another order. You can change this behaviour by passing a set of [`cargo_flags_t`](api.md#cargo_flags_t) to [`cargo_init`](api.md#cargo_init).

To turn off any automatic error output you can pass [`CARGO_NOERR_OUTPUT`](api.md#cargo_noerr_output), and for no usage [`CARGO_NOERR_USAGE`](api.md#cargo_noerr_usage). When you do this, you can still fetch any error message using [`cargo_get_error`](api.md#cargo_get_error).

If you instead rather have cargo output the full usage on an error you can pass the [`CARGO_USAGE_FULL_USAGE`](api.md#cargo_usage_full_usage) flag to the [`cargo_set_internal_usage_flags`](api.md#cargo_set_internal_usage_flags) function. Or use any of the [`cargo_usage_t`](api.md#cargo_usage_t) flags like when using [`cargo_print_usage`](api.md#cargo_print_usage).

By default the error messages are printed to `stderr`, if you instead want cargo to print them to `stdout` you can set the [`CARGO_ERR_STDOUT`](api.md#cargo_err_stdout) flag.

Here are some examples on how this looks:

### Default behaviour:

```bash
$ cargo_ex_ints
Usage: bin/cargo_ex_ints [--sum] [--help] INTEGERS [INTEGERS ...]
Missing required argument "integers"

$ cargo_ex_ints 1 2 3 --sam
Usage: bin/cargo_ex_ints [--sum] [--help] INTEGERS [INTEGERS ...]
Unknown options:
1 2 3 --sam
      ~~~~~
--sam  (Did you mean --sum)?

```

### No error output

[`CARGO_NOERR_OUTPUT`](api.md#cargo_noerr_output)

```bash
$ cargo_ex_ints
Usage: bin/cargo_ex_ints [--sum] [--help] INTEGERS [INTEGERS ...]

$ cargo_ex_ints 1 2 3 --sam
Usage: bin/cargo_ex_ints [--sum] [--help] INTEGERS [INTEGERS ...]

```

### Long usage

`cargo_set_internal_usage_flags(cargo, CARGO_USAGE_FULL_USAGE);`

```bash
$ cargo_ex_ints
Usage: bin/cargo_ex_ints [--sum] [--help] INTEGERS [INTEGERS ...]

Process some integers.

Positional arguments:
   INTEGERS [INTEGERS ...]  An integer for the accumulator

Options:
  -s, --sum                 Sum the integers (default: find the max)
  -h, --help                Show this help

Missing required argument "integers"

$ cargo_ex_ints 1 2 3 --sam
Usage: bin/cargo_ex_ints [--sum ] [--help ] INTEGERS [INTEGERS ...]

Process some integers.

Positional arguments:
   INTEGERS [INTEGERS ...]  An integer for the accumulator

Options:
  -s, --sum                 Sum the integers (default: find the max)
  -h, --help                Show this help

Unknown options:
1 2 3 --sam
      ~~~~~
--sam  (Did you mean --sum)?

```

