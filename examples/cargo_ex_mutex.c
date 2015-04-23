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

    if (cargo_init(&cargo, 0, "%s", argv[0]))
    {
        fprintf(stderr, "Failed to init command line parsing\n");
        return -1;
    }

    cargo_set_description(cargo, LOREM_IPSUM);

    // Default group.
    ret |= cargo_add_option(cargo, 0, "--alpha -a", "Alpha flag", "D");
    ret |= cargo_add_option(cargo, 0, "--beta -b", "The beta thing", "D");
    ret |= cargo_add_option(cargo, 0, "--centauri -c", LOREM_IPSUM, "D");

    // First mutex group.
    ret |= cargo_add_mutex_group(cargo, 0, "mgroup1", "Mutex group one", NULL);
    ret |= cargo_add_option(cargo, 0, "<!mgroup1> --delta -d", LOREM_IPSUM, "D");
    ret |= cargo_add_option(cargo, 0, "<!mgroup1> --elastic -e", "Elastic", "D");
    assert(ret == 0);

    // First normal group.
    ret |= cargo_add_group(cargo, 0, "group1",
            "Group one", "This is an extra group. " LOREM_IPSUM);
    ret |= cargo_add_option(cargo, 0, "<group1> --fact -f", LOREM_IPSUM, "D");
    ret |= cargo_add_option(cargo, 0, "<group1> --great -g", LOREM_IPSUM, "D");
    ret |= cargo_add_option(cargo, 0, "<group1> --hello", "Hello there", "D");

    // Second mutex group.
    // (Add existing options to the group, one option required)
    ret |= cargo_add_mutex_group(cargo,
            CARGO_MUTEXGRP_ONE_REQUIRED,
            "mgroup2", "Mutex group two", NULL);
    ret |= cargo_mutex_group_add_option(cargo, "mgroup2", "--delta");
    ret |= cargo_mutex_group_add_option(cargo, "mgroup2", "--great");
    assert(ret == 0);

    // Third mutex group.
    // (Group the option in the usage like a normal group
    //  but not in the short usage)
    ret |= cargo_add_mutex_group(cargo,
            CARGO_MUTEXGRP_GROUP_USAGE |
            CARGO_MUTEXGRP_NO_GROUP_SHORT_USAGE,
            "mgroup3", "Mutex group three", "Only one of these allowed.");
    ret |= cargo_add_option(cargo, 0, "<!mgroup3> --ignore -i", "Ignore", "D");
    ret |= cargo_add_option(cargo, 0, "<!mgroup3> --joke -j", "Joke", "D");

    // Add an existing option to this group, it will be "stolen" from group1.
    ret |= cargo_mutex_group_add_option(cargo, "mgroup3", "--fact");

    // Fourth mutex group.
    // (Force order)
    ret |= cargo_add_mutex_group(cargo,
            CARGO_MUTEXGRP_ORDER_AFTER,
            "mgroup4", "Mutex group three", "These must be after --alpha");
    // All following options must be AFTER the first option added to the group.
    ret |= cargo_mutex_group_add_option(cargo, "mgroup4", "--alpha");

    ret |= cargo_mutex_group_add_option(cargo, "mgroup4", "--ignore");
    ret |= cargo_mutex_group_add_option(cargo, "mgroup4", "--joke");
    ret |= cargo_mutex_group_add_option(cargo, "mgroup4", "--great");
    ret |= cargo_mutex_group_add_option(cargo, "mgroup4", "--hello");

    cargo_set_epilog(cargo,
        "This is a text at the end a so called %s. It's a great place for "
        "examples and such.\n"
        "\n"
        "Here are some example command lines you can test:\n"
        "  %s --beta --alpha --delta --elastic\n"
        "  %s  --centauri --ignore --alpha --hello --great"
        , "epilog", argv[0], argv[0]);
    assert(ret == 0);

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        ret = -1;
        goto fail;
    }

fail:
    cargo_destroy(&cargo);

    return ret;
}
