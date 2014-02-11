
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cargo.h"

typedef struct cargo_opt_s
{
	const char *name;
	const char *description;
	cargo_type_t type;
	int nargs;
	int alloc;
	void *target;
	size_t *target_count;
	void *default_value;
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

static int _cargo_add(cargo_t ctx,
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				void *default_value,
				cargo_type_t type,
				const char *description,
				int alloc)
{
	cargo_opt_t *o = NULL;

	if (!_cargo_nargs_is_valid(nargs))
		return -1;

	if (!target)
		return -1;

	if (!target_count && (nargs > 0))
		return -1;

	if (ctx->opt_count >= ctx->max_opts)
		return -1;

	o = &ctx->options[ctx->opt_count];
	ctx->opt_count++;

	o->name = opt;
	o->nargs = nargs;
	o->target = target;
	o->type = type;
	o->description = description;
	o->default_value = default_value;
	o->target_count = target_count;
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

	return 0;
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
				void *default_value,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, target_count, nargs, 
						default_value, type, description, 0);
}

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				void *default_value,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, target_count, nargs, 
						default_value, type, description, 1);
}

int cargo_parse(cargo_t ctx, int argc, char **argv)
{
	return 0;
}

#ifdef CARGO_TEST

int main(int argc, char **argv)
{


	return 0;
}

#endif

