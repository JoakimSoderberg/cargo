
#ifndef __CARGO_H__
#define __CARGO_H__

typedef enum cargo_format_e
{
	CARGO_FORMAT_RAW_HELP = (1 << 0),
	CARGO_FORMAT_RAW_DESCRIPTION = (1 << 1),
	CARGO_FORMAT_AUTO_CLEAN = (1 << 2)
} cargo_format_t;

typedef enum cargo_type_e
{
	CARGO_DEFAULT = 0,
	CARGO_BOOL = 1,
	CARGO_INT = 2,
	CARGO_UINT = 3,
	CARGO_FLOAT = 4,
	CARGO_DOUBLE = 5,
	CARGO_STRING = 6
} cargo_type_t;

typedef struct cargo_s *cargo_t;

#define CARGO_NARGS_ONE_OR_MORE -1
#define CARGO_NARGS_NONE_OR_MORE -2

int cargo_init(cargo_t *ctx, const char *progname, const char *description);

void cargo_set_description(cargo_t ctx, const char *description);

void cargo_set_epilog(cargo_t ctx, const char *epilog);

void cargo_add_help(cargo_t ctx, int add_help);

void cargo_set_format(cargo_t ctx, cargo_format_t format);

int cargo_add(cargo_t ctx,
				const char *opt,
				int nargs,
				void *default_value,
				void *target,
				cargo_type_t type,
				const char *description);

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				int nargs,
				void *default_value,
				void *target,
				cargo_type_t type,
				const char *description);



int cargo_destroy(cargo_t *ctx);

#endif // __CARGO_H__
