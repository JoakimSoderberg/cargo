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

#define LOREM_IPSUM                                                         \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do"       \
    " eiusmod tempor incididunt ut labore et dolore magna aliqua. "         \
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "   \
    "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "    \
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "  \
    "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "    \
    "culpa qui officia deserunt mollit anim id est laborum."

int main(int argc, char **argv)
{
    int ret = 0;
    cargo_t cargo;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int e = 0;
    int f = 0;

    if (cargo_init(&cargo, 0, "%s", argv[0]))
    {
        fprintf(stderr, "Failed to init command line parsing\n");
        return -1;
    }

    // Default group.
    ret |= cargo_add_option(cargo, 0, "--beta -b", "The beta thing", "i", &b);
    ret |= cargo_add_option(cargo, 0, "--centauri -c", LOREM_IPSUM, "i", &c);

    // First group.
    ret |= cargo_add_group(cargo, 0, "group1",
                    "Group one", "Here are some extra flags.");
    ret |= cargo_add_option(cargo, 0, "<group1> --delta -d",
                            "The delta " LOREM_IPSUM, "b", &d);
    ret |= cargo_add_option(cargo, 0, "<group1> --elastic -e",
                            "The elastic rubber band", "b", &e);
    assert(ret == 0);

    // Second group.
    ret |= cargo_add_group(cargo, 0, "group2",
                    "Group two", "This is an extra group. " LOREM_IPSUM);
    ret |= cargo_add_option(cargo, 0, "<group2> --fact -f", LOREM_IPSUM, "i", &f);
    ret |= cargo_add_option(cargo, 0, "<group2> --great -g", LOREM_IPSUM, "D");
    ret |= cargo_add_option(cargo, 0, "<group2> --hello", "Hello there", "D");

    assert(ret == 0);

    if (argc < 2)
    {
        cargo_print_usage(cargo, CARGO_USAGE_SHORT);
    }

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        return -1;
    }

    printf("Alpha: %d\n", a);
    printf("Beta: %d\n", b);
    printf("Centauri: %d\n", c);
    printf("Delta: %d\n", d);
    printf("Elastic: %d\n", e);

    cargo_destroy(&cargo);

    return 0;
}
