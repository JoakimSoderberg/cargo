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
#include "cargo.h"

typedef int (*accumulator_f)(int *integers, size_t count);

int max_ints(int *integers, size_t count)
{
	size_t i;
	int m = 0;
	for (i = 0; i < count; i++)
		if (integers[i] > m) m = integers[i];
	return m;
}

int sum_ints(int *integers, size_t count)
{
	size_t i;
	int s = 0;
	for (i = 0; i < count; i++) s += integers[i];
	return s;
}

int main(int argc, char **argv)
{
	cargo_t cargo;
	int ret = 0;
	int *integers = NULL;
	size_t integer_count = 0;
	accumulator_f accumulator = max_ints;
	int sum_flag = 0;

	printf("cargo version v%s\n", cargo_get_version());

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	cargo_set_description(cargo, "Process some integers.");

	ret |= cargo_add_option(cargo, 0, "integers",
							"An integer for the accumulator",
							"[i]+", &integers, &integer_count);
	ret |= cargo_add_option(cargo, 0, "--sum -s",
							"Sum the integers (default: find the max)",
							"b", &sum_flag);
	assert(ret == 0);

	if (cargo_parse(cargo, 1, argc, argv)) return -1;
	if (sum_flag) accumulator = sum_ints;
	printf("%d\n", accumulator(integers, integer_count));
	cargo_destroy(&cargo);
	free(integers);
	return 0;
}
