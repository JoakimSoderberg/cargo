
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cargo.h"

typedef struct cargo_opt_s
{

} cargo_opt_t;

typedef struct cargo_s
{
	const char *progname;
	const char *description;
} cargo_s;

static size_t _cargo_get_type_size(cargo_type_t t)
{
	switch (t)
	{
		default:
		case CARGO_DEFAULT:
		case CARGO_BOOL: 
		case CARGO_INT: return sizeof(int);
		case CARGO_UINT: return sizeof(unsigned int);
		case CARGO_FLOAT: return sizeof(float);
		case CARGO_DOUBLE: return sizeof(double);
		case CARGO_STRING: return sizeof(char *);
	}
}

static int _cargo_add(cargo_t ctx,
				const char *opt,
				int nargs,
				void *default_value,
				void *target,
				cargo_type_t type,
				const char *description,
				int alloc)
{
	size_t tsize = _cargo_get_type_size(type);
	return 0;
}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

int cargo_init(cargo_t *ctx, const char *progname, const char *description)
{
	assert(ctx);
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
}

void cargo_set_format(cargo_t ctx, cargo_format_t format)
{
	assert(ctx);
}

int cargo_add(cargo_t ctx,
				const char *opt,
				int nargs,
				void *default_value,
				void *target,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, nargs, default_value, 
						target, type, description, 0);
}

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				int nargs,
				void *default_value,
				void *target,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, nargs, default_value, 
						target, type, description, 1);
}
