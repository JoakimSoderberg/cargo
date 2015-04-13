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

typedef enum cmd_s
{
	NONE,
	ADD,
	APPEND,
	DELETE,
	CREATE
} cmd_t;

const char *cmd_to_str(cmd_t cmd)
{
	switch (cmd)
	{
		case NONE:   return "none";
		case ADD:    return "add";
		case APPEND: return "append";
		case DELETE: return "delete";
		case CREATE: return "create";
	}

	return NULL;
}

typedef struct args_s
{
	cmd_t cmd;
	char *value;
	char *filename;
	int verbose_flag;
} args_t;

static int parse_command_cb(cargo_t ctx, void *user, const char *optname,
						int argc, char **argv)
{
	args_t *a = (args_t *)cargo_get_context(ctx);
	assert(a);

	a->cmd = (cmd_t)user;
	if (argc > 0) a->value = argv[0];

	return argc;
}

int main(int argc, char **argv)
{
	size_t i;
	cargo_t cargo;
	int ret = 0;
	const char **remain;
	size_t remain_count;
	args_t args;

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));
	cargo_set_context(cargo, &args);

	ret |= cargo_add_option(cargo, 0, "--verbose -v", "Be more verbose",
							"b!", &args.verbose_flag);

	ret |= cargo_add_option(cargo, 0, "--brief -b", "Be more brief",
							"b=", &args.verbose_flag, 0);

	ret |= cargo_add_mutex_group(cargo, 0, "cmd", NULL, NULL);
	ret |= cargo_add_option(cargo, 0, "<!cmd> --add -a", "Add", "[c]#", parse_command_cb, ADD, NULL, 0);
	ret |= cargo_add_option(cargo, 0, "<!cmd> --append", "Append", "c0", parse_command_cb, APPEND);
	ret |= cargo_add_option(cargo, 0, "<!cmd> --delete -d", "Delete", "c", parse_command_cb, DELETE);
	ret |= cargo_add_option(cargo, 0, "<!cmd> --create -c", "Create", "c", parse_command_cb, CREATE);
	ret |= cargo_add_option(cargo, 0, "--file -f", "File", "s", &args.filename);

	assert(ret == 0);

	if (cargo_parse(cargo, 1, argc, argv)) return -1;

	if (args.verbose_flag) printf("Verbose flag is set %d\n", args.verbose_flag);

	if (args.filename)
	{
		printf("Got filename \"%s\"\n", args.filename);
		free(args.filename);
	}

	printf("option %s", cmd_to_str(args.cmd));
	if (args.value) printf(" with value \"%s\"", args.value);
	printf("\n");

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

	cargo_destroy(&cargo);

	return 0;
}

