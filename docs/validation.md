Table of contents
=================

[TOC]

Introduction
============

You've now managed to add the options you want to parse to cargo. There is some validation made to the number of arguments an option accepts, but sometimes you want some restrictions to the values that are allowed. This is where validations come in.

Doing it manually
=================

Of course the most common practice is to never involve cargo, and simply check the results that cargo parsed by yourself:

```c
cargo_init(...)
...
ret |= cargo_add_option(cargo, 0, "--integers",
                            "A list of integers",
                            "[i]+", &integers, &integer_count);

...
cargo_parse(...)
...

for (i = 0; i < integer_count; i++)
{
    if ((integers[i] < -40) && (integers[i] > 40))
    {
        fprintf(stderr, "Integer %d out of bounds!\n", integers[i]);
        goto fail;
    }
}
```

But there is no clear correlation between the cargo option and the validation of it when doing it like this. If you have a lot of options it would be nicer to declare the validation in the same place as the option.

Using cargo for validation
==========================

cargo comes with a set of built-in validators that you can add to your option as you declare them. You can also create your own of course. Here's the same example as above but with cargo validation:

```c
cargo_init(...)
...
ret |= cargo_add_option(cargo, 0, "--integers",
                        "A list of integers",
                        "[i]+", &integers, &integer_count);

ret |= cargo_add_validation(cargo, 0, "--integers", 
                            cargo_validate_int_range(-40, 40));
...
cargo_parse(...)
...

```

This will then automatically be run by cargo as each option value is parsed, and you will get the same pretty errors with the invalid argument highlighted and so on.

Adding a validation
===================

To add a validation you use the [`cargo_add_validation`](api.md#cargo_add_validation) function:

```c
int cargo_add_validation(cargo_t ctx, cargo_validation_flags_t flags,
                         const char *opt, cargo_validation_t *vd);
```

The [`cargo_validation_t`](api.md#cargo_validation_t) struct above is created by a validation function. cargo will `free` this when cleaning up, so you should not try to free it yourself. To enable using one validation for multiple options, this struct is reference counted.

Note that a validator always defines what [`cargo_type_t`](api.md#cargo_type_t) types that it supports. So if you try to add it to an option of an unsupported type you will get an error.

Built-in validators
===================

cargo comes with two validators, one for validating a range, and one for a list of choices.

Range validator
---------------

The range validator however comes with one function for each number type cargo supports:

* [`cargo_validate_int_range(int min, int max)`](api.md#cargo_validate_int_range)
* [`cargo_validate_uint_range(unsigned int min unsigned int max)`](api.md#cargo_validate_uint_range)
* [`cargo_validate_longlong_range(long long int min, long long int max)`](api.md#cargo_validate_longlong_range)
* [`cargo_validate_ulonglong_range(unsigned long long int min, unsigned long long int max)`](api.md#cargo_validate_ulonglong_range)
* [`cargo_validate_float_range(float min, float max, float epsilon)`](api.md#cargo_validate_float_range)
* [`cargo_validate_double_range(double min, double max, double epsilon)`](api.md#cargo_validate_double_range)

The float validators takes an extra argument `epsilon` that decides how close a value is allowed to be, to be considered equal (since floating point comparisons are not an exact science). You can use the [`CARGO_DEFAULT_EPSILON`](api.md#cargo_default_epsilon) here.

Choices validator
-----------------

This validator also supports all number types, but also including strings.

```c
cargo_validate_choices(cargo_validate_choices_flags_t flags,
                       cargo_type_t type,
                       size_t count, ...);
```

Here we instead pass the [`type`](api.md#cargo_type_t) as an argument, which decides what type of choices that are then listed in the varargs list `...`.

When it comes to floating point values again, an epsilon value is used to get a "near enough" comparison between the values, by default [`CARGO_DEFAULT_EPSILON`](api.md#cargo_default_epsilon) is used for this. But if you want to override this, you can use the flag [`CARGO_VALIDATE_CHOICES_SET_EPSILON`](api.md#cargo_validate_choices_set_epsilon), and then specify the epsilon as the first option after `count`.

A list of string choices is by default compared without case sensitivity. If you want to turn it on you can use the flag [`CARGO_VALIDATE_CHOICES_CASE_SENSITIVE`](api.md#cargo_validate_choices_case_sensitive)

Note that the implementation of how `long long` is hanled differs between platforms. It is highly recommended that you explicitly cast each argument passed to the range validators to the corresponding type, so that you can be sure that the compiler doesn't decide to pass an `int` instead of a `long long int`.

Creating your own validation
============================

For the types supported by cargo most validations are solved by the range and choices validators, or by creating a [custom option parser](api.md#custom-parsing) (since you can do the validation in the parse callback).

However, if you still need your own validator you can simply create your own. Let's say we want to create a validator that only allows even values for integers and they cannot be above a set max.

First we define our own validation struct that we will use as a context when performing the validation. This is where we place the `max` value:

```c
typedef struct even_validation_s
{
    int max; // We can use this for our own context.
} even_validation_t;
```

Next we will need a callback function that does the actual validation of the argument values. This function has the following signature:

```c
typedef int (*cargo_validation_f)(cargo_t ctx,
                                cargo_validation_flags_t flags,
                                const char *opt, cargo_validation_t *vd,
                                void *value);
```

So let's add that:

```c
int even_validation_validate_cb(cargo_t ctx,
                      cargo_validation_flags_t flags,
                      const char *opt, cargo_validation_t *vd,
                      void *value)
{
    even_validation_t *ev = NULL;
    int v = *((int *)value);

    // Retrieve our context struct.
    ev = (even_validation_t *)cargo_validator_get_context(vd);

    if (v > ev->max)
    {
        cargo_set_error(ctx, 0, "Value of %s cannot be above %d",
                       opt, ev->max);
        return -1;
    }

    if ((v % 2) != 0)
    {
        cargo_set_error(ctx, 0, "Value of %s must be even", opt);
        return -1;
    }

    return 0; // Success!
}
```

Our `even_validation_t` struct doesn't contain any allocated memory, but for the sake of things we will still add a `cargo_validation_destroy_f` callback that cargo will call when `free`ing the validator. It has this signature:

```c
typedef void (*cargo_validation_destroy_f)(void *user);
```

Our destroy callback:

```c
void even_validation_destroy_cb(void *user)
{
    even_validation_t *ev = (even_validation_t *)user;

    // Here we would free anything that we allocated in `ev`.
}
```

(Note that this destroy callback won't be called until the last reference to the validator is destroyed).

Finally we will need a function that creates the validation. This can have any signature we like except that it needs to return a `cargo_validation_t *` that it has allocated:

```c
cargo_validation_t *even_validate_int(int max)
{
    even_validation_t *ev = NULL;
    cargo_validation_t *v = NULL;

    // Make sure we zero the entire struct.
    if (!(ev = calloc(1, sizeof(even_validation_t))))
    {
        return NULL;
    }

    if (!(v = cargo_create_validator(
            // Give the validator a name. This is used for internal
            // error messages by cargo if CARGO_DEBUG is turned on.
            "even",
            even_validation_validate_cb,
            even_validation_destroy_cb,
            // Set the types we support we could add multiple types here
            CARGO_INT,
            // Add our context.
            ev)))
    {
        free(ev);
        return NULL;
    }

    // Set our own setting in the context!
    ev->max = max;

    return v;
}
```

So to use this we would do:

```c
ret |= cargo_add_option(cargo, 0, "--integers",
                        "A list of integers",
                        "[i]+", &integers, &integer_count);

ret |= cargo_add_validation(cargo, 0, "--integers", 
                            even_validate_int(20));
```

Running this we would get the following errors on invalid input:

```bash
$ theprogram --integers 3 2 50


Usage: theprogram [--help] [--integers INTEGERS [INTEGERS ...]]
--integers 3 2 50
           ~
Value of --integers must be even


$ theprogram --integers 4 2 50

Usage: theprogram [--help] [--integers INTEGERS [INTEGERS ...]]
--integers 4 2 50
               ~~
Value 50 of --integers cannot be above 20
```
