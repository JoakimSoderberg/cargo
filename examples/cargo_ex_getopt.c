#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cargo.h"

static int verbose_flag;

static int print_command(cargo_t ctx, void *user, const char *optname,
						int argc, char **argv)
{
	printf("option %s", optname);
	if (argc > 0) printf(" with value \"%s\"", argv[0]);
	printf("\n");

	return argc;
}

int main(int argc, char **argv)
{
	size_t i;
	cargo_t cargo;
	int ret = 0;
	const char **remain;
	size_t remain_count;
	char *filename = NULL;

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	ret |= cargo_add_option(cargo, 0, "--verbose -v", "Be more verbose",
							"b!", &verbose_flag);

	ret |= cargo_add_option(cargo, 0, "--brief -b", "Be more brief",
							"b=", &verbose_flag, 0);

	ret |= cargo_add_mutex_group(cargo, 0, "command");
	ret |= cargo_add_option(cargo, 0, "--add -a", "Add", "[c]#", print_command, NULL, NULL, 0);
	ret |= cargo_add_option(cargo, 0, "--append", "Append", "c0", print_command);
	ret |= cargo_add_option(cargo, 0, "--delete -d", "Delete", "c", print_command);
	ret |= cargo_add_option(cargo, 0, "--create -c", "Create", "c", print_command);
	ret |= cargo_add_option(cargo, 0, "--file -f", "File", "s", &filename);

	assert(ret == 0);

	if (cargo_parse(cargo, 1, argc, argv)) return -1;

	if (verbose_flag) printf("Verbose flag is set %d\n", verbose_flag);

	if (filename)
	{
		printf("Got filename \"%s\"\n", filename);
		free(filename);
	}

	// Print remaining options.
	remain = cargo_get_args(cargo, &remain_count);

	if (remain_count > 0)
	{
		for (i = 0; i < remain_count; i++)
		{
			printf("%s ", remain[i]);
		}

		printf("\n");
	}

	return 0;
}

