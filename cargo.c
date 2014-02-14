
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

	char **args;
	size_t arg_count;
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

	if (nargs >= 0) o->max_target_count = nargs;
	else if (target_count) o->max_target_count = (*target_count);
	else o->max_target_count = 0;

	o->alloc = alloc;

	CARGODBG(1, " cargo_add %s, max_target_count = %lu\n",
				opt, o->max_target_count);

	return 0;
}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

int cargo_init(cargo_t *ctx, size_t max_opts,
				const char *progname, const char *description)
{
	cargo_s *c;
	assert(ctx);

	*ctx = (cargo_s *)calloc(1, sizeof(cargo_s));
	c = *ctx;

	if (!c)
		return -1;

	c->max_opts = max_opts;

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

		if ((*ctx)->args)
		{
			free((*ctx)->args);
			(*ctx)->args = NULL;
			(*ctx)->arg_count = 0;
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
		return 0;
	}

	errno = 0;

	switch (opt->type)
	{
		default: return -1;
		case CARGO_BOOL:
			CARGODBG(2, "%s", "      bool\n");
			((int *)opt->target)[opt->target_idx] = 1;
			break;
		case CARGO_INT:
			CARGODBG(2, "      int %s\n", val);
			((int *)opt->target)[opt->target_idx] = atoi(val);
			break;
		case CARGO_UINT:
			CARGODBG(2, "      uint %s\n", val);
			((unsigned int *)opt->target)[opt->target_idx]
													= strtoul(val, NULL, 10); 
			break;
		case CARGO_FLOAT:
			CARGODBG(2, "      float %s\n", val);
			((float *)opt->target)[opt->target_idx] = atof(val);
			break;
		case CARGO_DOUBLE:
			CARGODBG(2, "      double %s\n", val);
			((double *)opt->target)[opt->target_idx] = (double)atof(val);
			break;
		case CARGO_STRING:
			CARGODBG(2, "      str \"%s\"\n", val);
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

static int _cargo_is_another_option(cargo_t ctx, char *arg)
{
	int j;

	for (j = 0; j < ctx->opt_count; j++)
	{
		if (_cargo_is_option_name(&ctx->options[j], arg))
		{
			return 1;
		}
	}

	return 0;
}

static int _cargo_parse_option(cargo_t ctx, cargo_opt_t *opt, const char *name,
								int argc, char **argv, int i)
{
	int j;
	int args_to_look_for;

	// If we have at least 1 option, start looking for it.
	if (opt->nargs != 0)
	{
		if ((i + 1) >= argc)
		{
			printf("(%i+1) >= %d\n", i, argc);
			return -1;
		}

		i++;
	}

	if (opt->nargs == 0)
	{
		CARGODBG(1, "%s", "    No arguments\n");
		// Got no arguments, simply set the value to 1.
		if (_cargo_set_target_value(ctx, opt, name, argv[i]))
		{
			return -1;
		}
	}
	else
	{
		// Keep looking until the end of the argument list.
		if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) ||
			(opt->nargs == CARGO_NARGS_NONE_OR_MORE))
		{
			args_to_look_for = argc - i;
		}
		else
		{
			// Read (number of expected arguments) - (read so far).
			args_to_look_for = (opt->nargs - opt->target_idx);
		}

		// Look for arguments for this option.
		if ((i + args_to_look_for) > argc)
		{
			fprintf(stderr, "Not enough arguments for %s."
							" %d expected but got only %d\n", 
							name, opt->nargs, 
							(int)opt->target_idx + args_to_look_for);
			return -1;
		}

		CARGODBG(1, "  Parse %d option args for %s:\n", args_to_look_for, name);
		CARGODBG(1, "   Start %d, End %d\n", i, i + args_to_look_for);

		for (j = i; j < (i + args_to_look_for); j++)
		{

			CARGODBG(2, "    argv[%i]: %s\n", j, argv[j]);

			if (_cargo_is_another_option(ctx, argv[j]))
			{
				if ((j == i) && (opt->nargs != CARGO_NARGS_NONE_OR_MORE))
				{
					fprintf(stderr, "No argument specified for %s. "
									"%d expected.\n",
									name, 
									(opt->nargs > 0) ? opt->nargs : 1);
					return -1;
				}


				// We found another option, stop parsing arguments
				// for this option.
				CARGODBG(1, "%s", "    Found other option\n");
				break;
			}

			if (_cargo_set_target_value(ctx, opt, name, argv[j]))
			{
				return -1;
			}
		}

		i += j;
	}

	return i;
}

static const char *_cargo_check_options(cargo_t ctx,
					cargo_opt_t **opt,
					int argc, char **argv, int i)
{
	assert(opt);
	int j;
	const char *name = NULL;

	for (j = 0; j < ctx->opt_count; j++)
	{
		name = NULL;
		*opt = &ctx->options[j];

		if ((name = _cargo_is_option_name(*opt, argv[i])))
		{
			CARGODBG(2, "  Option argv[%i]: %s\n", i, name);

			return name;
		}
	}

	*opt = NULL;

	return NULL;
}

int cargo_parse(cargo_t ctx, int argc, char **argv)
{
	int i;
	int j;
	char *arg;
	const char *name;
	cargo_opt_t *opt = NULL;

	if (ctx->args)
	{
		free(ctx->args);
		ctx->args = NULL;
		ctx->arg_count = 0;
	}

	if (!(ctx->args = (char **)calloc(argc, sizeof(char *))))
	{
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		arg = argv[i];
		j = i;

		CARGODBG(1, "\nargv[%d] = %s\n", i, arg);

		if ((name = _cargo_check_options(ctx, &opt, argc, argv, i)))
		{
			// We found an option, parse any arguments it might have.
			if ((i = _cargo_parse_option(ctx, opt, name, argc, argv, i)) < 0)
			{
				return -1;
			}

		}
		else
		{
			ctx->args[ctx->arg_count] = argv[i];
			ctx->arg_count++;
		}

		#if CARGO_DEBUG
		{
			int k = 0;
			int ate = (i != j) ? (i - j): 1;

			CARGODBG(2, "    Ate %d args: ", ate);

			for (k = j; k < (j + ate); k++)
			{
				CARGODBG(2, "\"%s\" ", argv[k]);
			}

			CARGODBG(2, "%s", "\n");
		}
		#endif
	}

	return 0;
}

char **cargo_get_args(cargo_t ctx, size_t *argc)
{
	assert(ctx);

	if (argc)
	{
		*argc = ctx->arg_count;
	}

	return ctx->args;
}

// -----------------------------------------------------------------------------
// Tests.
// -----------------------------------------------------------------------------
#ifdef CARGO_TEST

typedef struct args_s
{
	int hello;
	int geese;
	int ducks[2];
	size_t duck_count;
} args_t;

int main(int argc, char **argv)
{
	int i;
	int ret = 0;
	cargo_t cargo;
	args_t args;
	char **extra_args;
	size_t extra_count;
	int default_geese = 3;

	cargo_init(&cargo, 32, argv[0], "The parser");

	ret = cargo_add(cargo, "--hello", &args.hello, 
				NULL,	// No count to return.
				0,		// No arguments.
				CARGO_BOOL,
				"Should we be greeted with a hello message?");

	args.geese = 3;
	ret = cargo_add(cargo, "--geese", &args.geese, NULL, 1, CARGO_INT,
				"How man geese live on the farm");

	args.ducks[0] = 6;
	args.ducks[1] = 4;
	args.duck_count = sizeof(args.ducks) / sizeof(args.ducks[0]);
	ret |= cargo_add(cargo, "--ducks", args.ducks, &args.duck_count, 2, CARGO_INT,
				"How man geese live on the farm");

	if (ret != 0)
	{
		fprintf(stderr, "Failed to add argument\n");
		return -1;
	}

	if (cargo_parse(cargo, argc, argv))
	{
		fprintf(stderr, "Error parsing!\n");
		ret = -1;
		goto fail;
	}

	if (args.hello)
	{
		printf("Hello! %d geese lives on the farm\n", args.geese);
		printf("Also %d + %d = %d ducks. Read %lu duck args\n", 
			args.ducks[0], args.ducks[1], args.ducks[0] + args.ducks[1],
			args.duck_count);
	}

	extra_args = cargo_get_args(cargo, &extra_count);

	for (i = 0; i < extra_count; i++)
	{
		printf("%s\n", extra_args[i]);
	}

fail:
	cargo_destroy(&cargo);

	return ret;
}

#endif

