
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "cargo.h"

static const char *_cargo_type_map[] = 
{
	"bool",
	"int",
	"uint",
	"float",
	"double",
	"string"
};

typedef struct cargo_opt_s
{
	const char *name[CARGO_NAME_COUNT];
	size_t name_count;
	const char *description;
	int optional;
	cargo_type_t type;
	int nargs;
	int alloc;
	void **target;
	size_t target_idx;
	size_t *target_count;
	size_t max_target_count;
} cargo_opt_t;

typedef struct cargo_s
{
	const char *progname;
	const char *description;
	cargo_format_t format;

	int i;
	int argc;
	char **argv;

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
				void **target,
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
	{
		CARGODBG(1, "%s", "Null option name\n");
		return -1;
	}

	if ((opt_len = strlen(opt)) == 0)
	{
		CARGODBG(1, "%s", "Option name has length 0\n");
		return -1;
	}

	if (!target)
	{
		CARGODBG(1, "%s", "target NULL\n");
		return -1;
	}

	if (!target_count && (nargs > 1))
	{
		CARGODBG(1, "%s", "target_count NULL, when nargs > 1\n");
		return -1;
	}

	if (ctx->opt_count >= ctx->max_opts)
	{
		CARGODBG(1, "%s", "Null option name\n");
		return -1;
	}

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

	// By default "nargs" is the max number of arguments the option
	// should parse. But when allocating the space internally
	// and nargs is set to CARGO_NARGS_ONE_OR_MORE the max allowed
	// value is specified by the value in "target_count", or if that is 0 the
	// size_t max value is used.
	if (nargs >= 0)
	{
		o->max_target_count = nargs;
	}
	else if (target_count)
	{
		if (*target_count == 0)
			o->max_target_count = SIZE_MAX;
		else
			o->max_target_count = (*target_count);
	}
	else
	{
		o->max_target_count = 0;
	}

	o->alloc = alloc;

	if (alloc)
	{
		*(o->target) = NULL;
	}

	CARGODBG(1, " cargo_add %s, max_target_count = %lu\n",
				opt, o->max_target_count);

	return 0;
}

static const char *_cargo_is_option_name(cargo_opt_t *opt, const char *arg)
{
	int i;
	const char *name;

	for (i = 0; i < opt->name_count; i++)
	{
		name = opt->name[i];

		CARGODBG(3, "    Check name \"%s\"\n", name);

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
	void *target;

	if ((opt->type != CARGO_BOOL) 
		&& (opt->target_idx >= opt->max_target_count))
	{
		return 1;
	}

	if ((opt->type < CARGO_BOOL) || (opt->type > CARGO_STRING))
		return -1;

	if (opt->alloc)
	{
		// Allocate the memory needed.
		if (!*(opt->target))
		{
			void **new_target;
			int alloc_count = opt->nargs; 

			if (opt->nargs <= 0)
			{
				// In this case we don't want to preallocate everything
				// since we might have "unlimited" arguments.
				// CARGO_NARGS_ONE_OR_MORE
				// CARGO_NARGS_NONE_OR_MORE
				// TODO: Don't allocate all of these right away.
				alloc_count = ctx->argc - ctx->i;
			}

			if (!(new_target = (void **)calloc(alloc_count,
						_cargo_get_type_size(opt->type))))
			{
				fprintf(stderr, "Out of memory!\n");
				return -1;
			}

			CARGODBG(1, "Allocated %dx %s!\n",
					opt->nargs, _cargo_type_map[opt->type]);

			*(opt->target) = new_target;
		}

		target = *(opt->target);
	}
	else
	{
		// Just a normal pointer.
		target = (void *)opt->target;
	}

	errno = 0;

	switch (opt->type)
	{
		default: return -1;
		case CARGO_BOOL:
			CARGODBG(2, "%s", "      bool\n");
			((int *)target)[opt->target_idx] = 1;
			break;
		case CARGO_INT:
			CARGODBG(2, "      int %s\n", val);
			((int *)target)[opt->target_idx] = atoi(val);
			break;
		case CARGO_UINT:
			CARGODBG(2, "      uint %s\n", val);
			((unsigned int *)opt->target)[opt->target_idx]
													= strtoul(val, NULL, 10); 
			break;
		case CARGO_FLOAT:
			CARGODBG(2, "      float %s\n", val);
			((float *)target)[opt->target_idx] = atof(val);
			break;
		case CARGO_DOUBLE:
			CARGODBG(2, "      double %s\n", val);
			((double *)target)[opt->target_idx] = (double)atof(val);
			break;
		case CARGO_STRING:
			CARGODBG(2, "      str \"%s\"\n", val);
			((char **)target)[opt->target_idx] = val;
			break;
	}

	if (errno != 0)
	{
		fprintf(stderr, "Failed to parse \"%s\", expected %s. %s.\n", 
				val, _cargo_type_map[opt->type], strerror(errno));
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
								int argc, char **argv)
{
	int start = 0;
	int opt_arg_count = 0;

	// If we have at least 1 option, start looking for it.
	if (opt->nargs != 0)
	{
		if ((ctx->i + 1) >= argc)
		{
			printf("(%i+1) >= %d\n", ctx->i, argc);
			return -1;
		}

		start = ctx->i + 1;
	}

	if (opt->nargs == 0)
	{
		CARGODBG(1, "%s", "    No arguments\n");
		// Got no arguments, simply set the value to 1.
		if (_cargo_set_target_value(ctx, opt, name, argv[ctx->i]) < 0)
		{
			return -1;
		}
	}
	else
	{
		int ret;
		int j;
		int args_to_look_for;

		// Keep looking until the end of the argument list.
		if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) ||
			(opt->nargs == CARGO_NARGS_NONE_OR_MORE))
		{
			args_to_look_for =   argc - start;
		}
		else
		{
			// Read (number of expected arguments) - (read so far).
			args_to_look_for = (opt->nargs - opt->target_idx);
		}

		// Look for arguments for this option.
		if ((start + args_to_look_for) > argc)
		{
			fprintf(stderr, "Not enough arguments for %s."
							" %d expected but got only %d\n", 
							name, opt->nargs, 
							argc - start);
			return -1;
		}

		CARGODBG(1, "  Parse %d option args for %s:\n", args_to_look_for, name);
		CARGODBG(1, "   Start %d, End %d\n", ctx->i, ctx->i + args_to_look_for);

		// Read until we find another option, or we've "eaten" the
		// arguments we want.
		for (j = start; j < (start + args_to_look_for); j++)
		{
			CARGODBG(2, "    argv[%i]: %s\n", j, argv[j]);

			if (_cargo_is_another_option(ctx, argv[j]))
			{
				if ((j == ctx->i) && (opt->nargs != CARGO_NARGS_NONE_OR_MORE))
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

			if ((ret = _cargo_set_target_value(ctx, opt, name, argv[j])) < 0)
			{
				return -1;
			}

			// If we have exceeded opt->max_target_count
			// for CARGO_NARGS_NONE_OR_MORE or CARGO_NARGS_ONE_OR_MORE
			// we should stop so we don't eat all the remaining arguments.
			if (ret)
				break;
		}

		opt_arg_count = j - start;
	}

	return opt_arg_count;
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

static int _cargo_find_option_name(cargo_t ctx, const char *name, 
									int *opt_i, int *name_i)
{
	int i;
	int j;
	cargo_opt_t *opt;

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		for (j = 0; j < opt->name_count; j++)
		{
			if (!strcmp(opt->name[j], name))
			{
				*opt_i = i;
				*name_i = j;
				return 0;
			}
		}
	}

	return -1; 
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
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, (void **)target, NULL, (type != CARGO_BOOL),
						type, description, 0);
}

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				void **target,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, NULL, (type != CARGO_BOOL),
						type, description, 1);
}


int cargo_addv(cargo_t ctx, 
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, (void **)target, target_count,
						nargs, type, description, 0);
}

int cargo_addv_alloc(cargo_t ctx, 
				const char *opt,
				void **target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, target_count,
						nargs, type, description, 1);
}

int cargo_parse(cargo_t ctx, int argc, char **argv)
{
	int start;
	int opt_arg_count;
	char *arg;
	const char *name;
	cargo_opt_t *opt = NULL;

	ctx->argc = argc;
	ctx->argv = argv;

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

	for (ctx->i = 1; ctx->i < argc; ctx->i++)
	{
		arg = argv[ctx->i];
		start = ctx->i;

		CARGODBG(1, "\nargv[%d] = %s\n", ctx->i, arg);

		if ((name = _cargo_check_options(ctx, &opt, argc, argv, ctx->i)))
		{
			// We found an option, parse any arguments it might have.
			if ((opt_arg_count = _cargo_parse_option(ctx, opt, name,
													argc, argv)) < 0)
			{
				return -1;
			}

			ctx->i += opt_arg_count;
		}
		else
		{
			// Normal argument.
			ctx->args[ctx->arg_count] = argv[ctx->i];
			ctx->arg_count++;
		}

		#if CARGO_DEBUG
		{
			int k = 0;
			int ate = (ctx->i != start) ? (ctx->i - start): 1;

			CARGODBG(2, "    Ate %d args: ", ate);

			for (k = start; k < (start + ate); k++)
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

int cargo_add_alias(cargo_t ctx, const char *name, const char *alias)
{
	int opt_i;
	int name_i;
	cargo_opt_t *opt;
	assert(ctx);

	if (_cargo_find_option_name(ctx, name, &opt_i, &name_i))
	{
		return -1;
	}

	CARGODBG(1, "Found option \"%s\"\n", name);

	opt = &ctx->options[opt_i];

	if ((opt->name_count + 1) >= CARGO_NAME_COUNT)
	{
		return -1;
	}

	opt->name[opt->name_count] = alias;
	opt->name_count++;

	CARGODBG(1, "  Added alias \"%s\"\n", alias);

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
	int ducks[2];
	size_t duck_count;

	float arne;
	double weise;
	char *awesome;

	char *poems[20];
	size_t poem_count;

	char **tut;
	size_t tut_count;

	char **blurp;
	size_t blurp_count;
} args_t;

int main(int argc, char **argv)
{
	int i;
	int ret = 0;
	cargo_t cargo;
	args_t args;
	char **extra_args;
	size_t extra_count;

	cargo_init(&cargo, 32, argv[0], "The parser");

	ret = cargo_add(cargo, "--hello", &args.hello, CARGO_BOOL,
				"Should we be greeted with a hello message?");

	args.geese = 3;
	ret = cargo_add(cargo, "--geese", &args.geese, CARGO_INT,
				"How man geese live on the farm");

	args.ducks[0] = 6;
	args.ducks[1] = 4;
	args.duck_count = sizeof(args.ducks) / sizeof(args.ducks[0]);
	ret |= cargo_addv(cargo, "--ducks", (void **)&args.ducks, &args.duck_count,
				 2, CARGO_INT, "How man geese live on the farm");

	args.arne = 4.4f;
	ret |= cargo_add(cargo, "--arne", &args.arne, CARGO_FLOAT,
				"Arne");
	cargo_add_alias(cargo, "--arne", "-a");

	args.poem_count = 0;
	ret |= cargo_addv(cargo, "--poems", args.poems, &args.poem_count, 3,
				CARGO_STRING,
				"The poems");

	ret |= cargo_addv_alloc(cargo, "--tut", (void **)&args.tut, &args.tut_count, 
							5, CARGO_STRING, "Tutiness");

	args.blurp_count = 0;
	ret |= cargo_addv_alloc(cargo, "--blurp", (void **)&args.blurp, &args.blurp_count, 
							CARGO_NARGS_ONE_OR_MORE, CARGO_STRING, "Blurp");

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

	printf("Arne %f\n", args.arne);

	printf("Poems:\n");
	for (i = 0; i < args.poem_count; i++)
	{
		printf("  %s\n", args.poems[i]);
	}

	printf("Tut %lu:\n", args.tut_count);
	for (i = 0; i < args.tut_count; i++)
	{
		printf("  %s\n", args.tut[i]);
	}

	printf("Blurp %lu:\n", args.blurp_count);
	for (i = 0; i < args.blurp_count; i++)
	{
		printf("  %s\n", args.blurp[i]);
	}

	if (args.hello)
	{
		printf("Hello! %d geese lives on the farm\n", args.geese);
		printf("Also %d + %d = %d ducks. Read %lu duck args\n", 
			args.ducks[0], args.ducks[1], args.ducks[0] + args.ducks[1],
			args.duck_count);
	}

	extra_args = cargo_get_args(cargo, &extra_count);
	printf("\nExtra arguments:\n");

	for (i = 0; i < extra_count; i++)
	{
		printf("%s\n", extra_args[i]);
	}

fail:
	cargo_destroy(&cargo);
	if (args.tut)
		free(args.tut);

	return ret;
}

#endif

