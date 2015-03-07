#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cargo.h"

typedef int (*accumulator_f)(int *integers, size_t count);

int max(int *integers, size_t count)
{
	size_t i;
	int max = 0;
	for (i = 0; i < count; i++)
		if (integers[i] > max) max = integers[i];
	return max;
}

int sum(int *integers, size_t count)
{
	size_t i;
	int sum = 0;
	for (i = 0; i < count; i++) sum += integers[i];
	return sum;
}

int main(int argc, char **argv)
{
	cargo_t cargo;
	int ret = 0;
	int *integers = NULL;
	size_t integer_count = 0;
	accumulator_f accumulator = max;
	int sum_flag = 0;

	if (cargo_init(&cargo, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	cargo_set_description(cargo, "Process some integers.");

	ret |= cargo_add_option(cargo,
							"integers", "An integer for the accumulator",
							"[i]+", &integers, &integer_count);
	ret |= cargo_add_option(cargo, "--sum -s",
							"Sum the integers (default: find the max)",
							"b", &sum_flag);
	assert(ret == 0);

	if (cargo_parse(cargo, 1, argc, argv)) return -1;
	if (sum_flag) accumulator = sum;
	printf("%d\n", accumulator(integers, integer_count));
	cargo_destroy(&cargo);
	free(integers);
	return 0;
}
