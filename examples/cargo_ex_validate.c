//
// The MIT License (MIT)
//
// Copyright (c) 2015 Joakim Soderberg <joakim.soderberg@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cargo.h"

typedef struct even_validation_s
{
    cargo_validation_t super; // We "inherit" the base struct.
                              // NOTE! This must be at the top.

    int max;                  // We can use this for our own context.
} even_validation_t;

int even_validation_validate_cb(cargo_t ctx,
                      cargo_validation_flags_t flags,
                      const char *opt, cargo_validation_t *vd,
                      void *value)
{
    even_validation_t *ev = (even_validation_t *)vd;
    int v = *((int *)value);

    if (v > ev->max)
    {
        cargo_set_error(ctx, 0, "Value %d of %s cannot be above %d",
                        v,opt, ev->max);
        return -1;
    }

    if ((v % 2) != 0)
    {
        cargo_set_error(ctx, 0, "Value %d of %s must be even", v,
         opt);
        return -1;
    }

    return 0; // Success!
}

void even_validation_destroy_cb(cargo_validation_t *vd)
{
    even_validation_t *ev = (even_validation_t *)vd;

    // Here we would free anything that we allocated in `ev`.
}

cargo_validation_t *even_validate_int(int max)
{
    even_validation_t *ev = NULL;

    // Make sure we zero the entire struct.
    if (!(ev = calloc(1, sizeof(even_validation_t))))
    {
        return NULL;
    }

    // Give the validator a name. This is used for internal
    // error messages by cargo if CARGO_DEBUG is turned on.
    ev->super.name = "even";

    // Setup the callbacks.
    ev->super.validator = even_validation_validate_cb;
    ev->super.destroy = even_validation_destroy_cb;

    // Set the types we support we could add multiple types here
    // CARGO_INT | CARGO_STRING and so on...
    ev->super.types = CARGO_INT;

    // Set our own setting!
    ev->max = max;

    return (cargo_validation_t *)ev;
}

int main(int argc, char **argv)
{
    cargo_t cargo;
    int ret = 0;
    int *integers = NULL;
    size_t i;
    size_t integer_count = 0;

    if (cargo_init(&cargo, 0, "%s", argv[0]))
    {
        fprintf(stderr, "Failed to init command line parsing\n");
        return -1;
    }

    ret |= cargo_add_option(cargo, 0, "--integers",
                        "A list of integers",
                        "[i]+", &integers, &integer_count);

    ret |= cargo_add_validation(cargo, 0, "--integers",
                                even_validate_int(20));

    assert(ret == 0);

    if (argc < 2)
    {
        cargo_print_usage(cargo, 0);
        return -1;
    }

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        return -1;
    }

    for (i = 0; i < integer_count; i++)
    {
        printf("%2lu: %d\n", i+1, integers[i]);
    }

    cargo_destroy(&cargo);

    return 0;
}

