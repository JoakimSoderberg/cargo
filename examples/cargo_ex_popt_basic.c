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

    ret |= cargo_add_option(cargo, 0, "--optiona -a", "Option A", "i", &optiona);
    ret |= cargo_add_option(cargo, 0, "--optionb -b", "Option B", "i", &optionb);
    ret |= cargo_add_option(cargo, 0, "--optionc -c", "Option C", "i", &optionc);
    ret |= cargo_add_option(cargo, 0, "--flag1 -f", "Flag 1", "b", &flag1);
    ret |= cargo_add_option(cargo, 0, "--flag2 -g", "Flag 2", "b", &flag2);
    ret |= cargo_add_option(cargo, 0, "args", "Remaining args", "[s]*", &args, &args_count);
    assert(ret == 0);

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        return -1;
    }

    // Print remaining commandline arguments.
    // args = cargo_get_args(cargo, &args_count);
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
