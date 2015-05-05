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

typedef enum debug_level_e
{
   NONE  = 0,
   ERROR = (1 << 0),
   WARN  = (1 << 1),
   INFO  = (1 << 2),
   DEBUG = (1 << 3)
} debug_level_t;

typedef enum debug_level2_e
{
   NONE2 = 0,
   ERROR2,
   WARN2,
   INFO2,
   DEBUG2
} debug_level2_t;

int main(int argc, char **argv)
{
	int ret = 0;
	cargo_t cargo;
	debug_level_t debug_level = NONE;
	debug_level2_t debug_level2 = NONE2;

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	// Combine flags using OR.
	ret |= cargo_add_option(cargo, 0,
			"--verbose -v", "Verbosity level",
			"b|", &debug_level, 4, ERROR, WARN, INFO, DEBUG);

	// Store the flags and overwrite previous value.
	ret |= cargo_add_option(cargo, 0,
			"--loud -l", "Loudness level",
			"b_", &debug_level2, 4, ERROR2, WARN2, INFO2, DEBUG2);

	if (cargo_parse(cargo, 0, 1, argc, argv))
	{
		return -1;
	}

	printf("debug level: 0x%x\n", debug_level);
	if (debug_level & ERROR) printf("ERROR\n");
	if (debug_level & WARN)  printf("WARN\n");
	if (debug_level & INFO)  printf("INFO\n");
	if (debug_level & DEBUG) printf("DEBUG\n");

	printf("debug level2: %d\n", debug_level2);
	if (debug_level2 == ERROR2) printf("ERROR2\n");
	if (debug_level2 == WARN2)  printf("WARN2\n");
	if (debug_level2 == INFO2)  printf("INFO2\n");
	if (debug_level2 == DEBUG2) printf("DEBUG2\n");

	cargo_destroy(&cargo);

	return 0;
}
