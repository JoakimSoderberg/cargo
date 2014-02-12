
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "cargo.h"

typedef struct cargo_opt_s
{
	const char *name[CARGO_NAME_COUNT];
	size_t name_count;
	const char *description;
	int optional;
	cargo_type_t type;
	int nargs;
	int alloc;
	void *target;
	size_t target_idx;
	size_t *target_count;
	size_t max_target_count;
} cargo_opt_t;

typedef struct cargo_s
{
	const char *progname;
	const char *description;
	cargo_format_t format;

	int add_help;

	cargo_opt_t *options;
	size_t opt_count;
	size_t max_opts;
	const char *prefix;

	char *args;
	size_t arg_count;
	size_t max_args;
} cargo_s;

static size_t _cargo_get_type_size(cargo_type_t t)
{
	switch (t)
	{
		default:
		case CARGO_BOOL: 
		case CARGO_INT: return sizeof(int);
		case CARGO_UINT: return sizeof(unsigned int);
		case CARGO_FLOAT: return sizeof(float);
		case CARGO_DOUBLE: return sizeof(double);
		case CARGO_STRING: return sizeof(char *);
	}
}

static int _cargo_nargs_is_valid(int nargs)
{
	return (nargs >= 0) 
		|| (nargs == CARGO_NARGS_NONE_OR_MORE)
		|| (nargs == CARGO_NARGS_ONE_OR_MORE);
}

static char _cargo_is_prefix(cargo_t ctx, char c)
{
	int i;
	size_t prefix_len = strlen(ctx->prefix);

	for (i = 0; i < prefix_len; i++)
	{
		if (c == ctx->prefix[i])
		{
			return c;
		}
	}

	return 0;
}

static int _cargo_add(cargo_t ctx,
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description,
				int alloc)
{
	size_t opt_len;
	cargo_opt_t *o = NULL;

	if (!_cargo_nargs_is_valid(nargs))
		return -1;

	if (!opt)
		return -1;

	if ((opt_len = strlen(opt)) == 0)
		return -1;

	if (!target)
		return -1;

	if (!target_count && (nargs > 1))
		return -1;

	if (ctx->opt_count >= ctx->max_opts)
		return -1;


	// TODO: assert for argument conflicts.

	o = &ctx->options[ctx->opt_count];
	ctx->opt_count++;

	// Check if the option has a prefix
	// (this means it's optional).
	o->optional = _cargo_is_prefix(ctx, opt[0]);

	o->name[o->name_count++] = opt;
	o->nargs = nargs;
	o->target = target;
	o->type = type;
	o->description = description;
	o->target_count = target_count;
	o->max_target_count = target_count ? (*target_count) : 1;
	o->alloc = alloc;

	return 0;
}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

int cargo_init(cargo_t *ctx, size_t max_opts, size_t max_args,
				const char *progname, const char *description)
{
	cargo_s *c;
	assert(ctx);

	*ctx = (cargo_s *)calloc(1, sizeof(cargo_s));
	c = *ctx;

	if (!c)
		return -1;

	c->max_opts = max_opts;
	c->max_args = max_args;

	if (!(c->options = (cargo_opt_t *)calloc(max_opts, sizeof(cargo_opt_t))))
	{
		free(*ctx);
		*ctx = NULL;
		return -1;
	}

	c->progname = progname;
	c->description = description;
	c->add_help = 1;
	c->prefix = CARGO_DEFAULT_PREFIX;

	return 0;
}

void cargo_destroy(cargo_t *ctx)
{
	if (ctx)
	{
		if ((*ctx)->options)
		{
			free((*ctx)->options);
			(*ctx)->options = NULL;
		}

		free(*ctx);
		ctx = NULL;
	}
}

void cargo_set_prefix(cargo_t ctx, const char *prefix_chars)
{
	assert(ctx);
	ctx->prefix = prefix_chars;
}

void cargo_set_description(cargo_t ctx, const char *description)
{
	assert(ctx);
	ctx->description = description;
}

void cargo_set_epilog(cargo_t ctx, const char *epilog)
{
	assert(ctx);
}

void cargo_add_help(cargo_t ctx, int add_help)
{
	assert(ctx);
	ctx->add_help = add_help;
}

void cargo_set_format(cargo_t ctx, cargo_format_t format)
{
	assert(ctx);
	ctx->format = format;
}

int cargo_add(cargo_t ctx,
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, target_count, nargs, 
						type, description, 0);
}

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, target_count, nargs,
						type, description, 1);
}

static const char *_cargo_is_option_name(cargo_opt_t *opt, const char *arg)
{
	int i;
	const char *name;

	for (i = 0; i < opt->name_count; i++)
	{
		name = opt->name[i];

		if (!strcmp(name, arg))
		{
			return name;
		}
	}

	return NULL;
}

static int _cargo_set_target_value(cargo_t ctx, cargo_opt_t *opt,
									const char *name, char *val)
{
	if ((opt->type != CARGO_BOOL) 
		&& (opt->target_idx >= opt->max_target_count))
	{
		fprintf(stderr, "Too many arguments given for \"%s\", expected %lu "
						"but got %lu\n",
						name, opt->max_target_count, opt->target_idx);
		return -1;
	}

	switch (opt->type)
	{
		default: return -1;
		case CARGO_BOOL:
			((int *)opt->target)[opt->target_idx] = 1;
			break;
		case CARGO_INT:
			((int *)opt->target)[opt->target_idx] = atoi(val);
			break;
		case CARGO_UINT:
			((unsigned int *)opt->target)[opt->target_idx]
													= strtoul(val, NULL, 10); 
			break;
		case CARGO_FLOAT:
			((float *)opt->target)[opt->target_idx] = atof(val);
			break;
		case CARGO_DOUBLE:
			((double *)opt->target)[opt->target_idx] = (double)atof(val);
			break;
		case CARGO_STRING:
			((char **)opt->target)[opt->target_idx] = val;
			break;
	}

	if (errno != 0)
	{
		fprintf(stderr, "Invalid formatting %s\n", strerror(errno));
		return -1;
	}

	opt->target_idx++;

	if (opt->target_count)
	{
		*opt->target_count = opt->target_idx;
	}

	return 0;
}

static int _cargo_parse_option(cargo_t ctx, cargo_opt_t *opt, const char *name,
								int argc, char **argv, int i)
{
	int last_arg = (i + 1) + opt->nargs;

	if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) ||
		(opt->nargs == CARGO_NARGS_NONE_OR_MORE))
	{
		last_arg = argc;
	}

	if (opt->type == CARGO_BOOL)
	{
		if (_cargo_set_target_value(ctx, opt, name, argv[i]))
		{
			return -1;
		}
	}
	else
	{
		if (last_arg > argc)
			return -1;

		for (; i < last_arg; i++)
		{
			if (_cargo_set_target_value(ctx, opt, name, argv[i]))
			{
				return -1;
			}
		}
	}

	return 0;
}

static int _cargo_check_options(cargo_t ctx, int argc, char **argv, int i)
{
	int j;
	cargo_opt_t *opt;
	const char *name = NULL;

	for (j = 0; j < ctx->opt_count; j++)
	{
		name = NULL;
		opt = &ctx->options[j];

		if ((name = _cargo_is_option_name(opt, argv[i])))
		{
			if (_cargo_parse_option(ctx, opt, name, argc, argv, i))
			{
				return -1;
			}
			break;
		}
	}

	return 0;
}

int cargo_parse(cargo_t ctx, int argc, char **argv)
{
	int i;
	char *arg;

	for (i = 1; i < argc; i++)
	{
		arg = argv[i];

		if (_cargo_check_options(ctx, argc, argv, i))
		{
			return -1;
		}
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Tests.
// -----------------------------------------------------------------------------
#ifdef CARGO_TEST

typedef struct args_s
{
	int hello;
	int geese;
} args_t;

int main(int argc, char **argv)
{
	int ret = 0;
	cargo_t cargo;
	args_t args;
	int default_geese = 3;

	cargo_init(&cargo, 32, 32, argv[0], "The parser");

	ret = cargo_add(cargo, "--hello", &args.hello, 
				NULL,	// No count to return.
				0,		// No arguments.
				CARGO_BOOL,
				"Should we be greeted with a hello message?");

	args.geese = 3;
	ret = cargo_add(cargo, "--geese", &args.geese, NULL, 1, CARGO_INT,
				"How man geese live on the farm");

	if (cargo_parse(cargo, argc, argv))
	{
		fprintf(stderr, "Error parsing!\n");
		ret = -1;
		goto fail;
	}

	if (args.hello)
	{
		printf("Hello! %d geese lives on the farm\n", args.geese);
	}

fail:
	cargo_destroy(&cargo);

	return ret;
}

#endif

