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

typedef struct rect_s
{
    int width;
    int height;
} rect_t; 

static int parse_rect_cb(cargo_t ctx, void *user, const char *optname,
                         int argc, char **argv)
{
    assert(ctx);
    assert(user);

    // We expect a rect_t structure to be passed to us.
    rect_t *u = (rect_t *)user;
    memset(u, 0, sizeof(rect_t));

    if (argc > 0)
    {
        if (sscanf(argv[0], "%dx%d", &u->width, &u->height) != 2)
        {
            cargo_set_error(ctx, 0, "Error, expected format 2x2 for %s\n", optname);
            return -1;
        }

        return 1; // We ate 1 argument.
    }

    return -1; // Error.
}

static int parse_rect_list_cb(cargo_t ctx, void *user,
                                     const char *optname,
                                     int argc, char **argv)
{
    int i;
    assert(ctx);
    assert(user);
    rect_t **u = (rect_t **)user;
    rect_t *rect;

    if (!(*u = calloc(argc, sizeof(rect_t))))
    {
        return -1;
    }

    rect = *u;

    for (i = 0; i < argc; i++)
    {
        if (sscanf(argv[i], "%dx%d", &rect[i].width, &rect[i].height) != 2)
        {
            cargo_set_error(ctx, 0, "Error, expected format 2x2 for %s\n", optname);
            return -1;
        }
    }

    return i; // How many arguments we ate.
}

static int parse_rect_static_list_cb(cargo_t ctx, void *user,
                                     const char *optname,
                                     int argc, char **argv)
{
    int i;
    assert(ctx);
    assert(user);
    rect_t **u = (rect_t **)user;
    rect_t *rects = (rect_t *)user;

    for (i = 0; i < argc; i++)
    {
        if (sscanf(argv[i], "%dx%d", &rects[i].width, &rects[i].height) != 2)
        {
            cargo_set_error(ctx, 0, "Error, expected format 2x2 for %s\n", optname);
            return -1;
        }
    }

    return i; // How many arguments we ate.
}

int main(int argc, char **argv)
{
    int ret = 0;
    cargo_t cargo;
    size_t i;
    const char **args = NULL;
    size_t args_count = 0;
    rect_t rect;
    rect_t *rects = NULL;
    size_t rect_count = 0;
    rect_t squares[4];
    size_t squares_count = 0;
    memset(&rect, 0, sizeof(rect));

    if (cargo_init(&cargo, 0, "%s", argv[0]))
    {
        fprintf(stderr, "Failed to init command line parsing\n");
        return -1;
    }

    ret |= cargo_add_option(cargo, 0, "--rect","The rect",
                            "c", parse_rect_cb, &rect);

    // Allocated array with "unlimited" count.
    ret |= cargo_add_option(cargo, 0, "--rectangles --rects -r", "Rectangles",
                            "[c]+", parse_rect_list_cb, &rects, &rect_count);

    // Fixed size array.
    ret |= cargo_add_option(cargo, 0, "--squares --sq -s", "Squares",
                            "[c]#", parse_rect_static_list_cb,
                            &squares, &squares_count,
                            sizeof(squares) / sizeof(squares[0])); // Max elements.
    assert(ret == 0);

    if (cargo_parse(cargo, 0, 1, argc, argv))
    {
        return -1;
    }

    args = cargo_get_args(cargo, &args_count);

    // Print remaining commandline arguments.
    for (i = 0; i < args_count; i++)
    {
        printf("Extra argument: %s\n", args[i]);
    }

    printf("Lone Rectangle: %d x %d\n", rect.width, rect.height);

    for (i = 0; i < rect_count; i++)
    {
        printf("Rectangle %lu: %d x %d\n",
              (i + 1), rects[i].width, rects[i].height);
    }

    for (i = 0; i < squares_count; i++)
    {
        printf("Square %lu: %d x %d\n",
              (i + 1), squares[i].width, squares[i].height);
    }


    cargo_destroy(&cargo);

    return 0;
}
