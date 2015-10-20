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

int main(int argc, char **argv)
{
    int ret = 0;
    cargo_t cargo;
    int advhelp = 0;
    int optiona = -1;
    int optionb = -1;
    int optionc = -1;
    int flag1 = 0;
    int flag2 = 0;
    size_t i;
    const char **args = NULL;
    size_t args_count = 0;

    if (cargo_init(&cargo, 0, "%s", argv[0]))
    {
        fprintf(stderr, "Failed to init command line parsing\n");
        return -1;
    }

    ret |= cargo_add_option(cargo, 0, "--advanced_help", "More help", "b", &advhelp);
    ret |= cargo_add_option(cargo, 0, "--optionb -b", "Option B", "i", &optionb);
    ret |= cargo_add_option(cargo, 0, "--optionc -c", "Option C", "i", &optionc);

    // Make this hidden and only show when --advanced_help is set.
    cargo_add_group(cargo, CARGO_GROUP_HIDE, "flags",
                    "Extra flags", "Here are some extra flags.");
    ret |= cargo_add_option(cargo, 0, "<flags> --flag1 -f", "Flag 1", "b", &flag1);
    ret |= cargo_add_option(cargo, 0, "<flags> --flag2 -g", "Flag 2", "b", &flag2);

    ret |= cargo_add_option(cargo, 0, "args", "Remaining args", "[s]*", &args, &args_count);
    assert(ret == 0);

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        return -1;
    }

    if (advhelp)
    {
        // Remove the group hide flag so it shows up in the usage.
        cargo_group_set_flags(cargo, "flags", 0);
        cargo_print_usage(cargo, 0);
        return -1;
    }

    // Print remaining commandline arguments.
    for (i = 0; i < args_count; i++)
    {
        printf("Extra argument: %s\n", args[i]);
    }

    printf("Final value of optiona: %d\n", optiona);
    printf("Final value of optionb: %d\n", optionb);
    printf("Final value of optionc: %d\n", optionc);
    printf("Final value of flag1: %d\n", flag1);
    printf("Final value of flag2: %d\n", flag2);

    cargo_destroy(&cargo);

    return 0;
}
