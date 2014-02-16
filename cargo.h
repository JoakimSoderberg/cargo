
#ifndef __CARGO_H__
#define __CARGO_H__

#include <stdio.h>

typedef enum cargo_format_e
{
	CARGO_FORMAT_RAW_HELP = (1 << 0),
	CARGO_FORMAT_RAW_DESCRIPTION = (1 << 1),
	CARGO_FORMAT_AUTO_CLEAN = (1 << 2)
} cargo_format_t;

typedef enum cargo_type_e
{
	CARGO_BOOL = 0,
	CARGO_INT = 1,
	CARGO_UINT = 2,
	CARGO_FLOAT = 3,
	CARGO_DOUBLE = 4,
	CARGO_STRING = 5
} cargo_type_t;

#ifdef CARGO_DEBUG
#define CARGODBG(level, fmt, ...) \
{ \
	if (level <= CARGO_DEBUG) \
	{ \
		fprintf(stderr, fmt, __VA_ARGS__); \
	} \
}
#else
#define CARGODBG(level, fmt, ...)
#endif

#define CARGO_NAME_COUNT 4
#define CARGO_DEFAULT_PREFIX "-"
#define CARGO_DEFAULT_MAX_OPTS 32

typedef struct cargo_s *cargo_t;

#define CARGO_NARGS_ONE_OR_MORE -1
#define CARGO_NARGS_NONE_OR_MORE -2

int cargo_init(cargo_t *ctx, size_t max_opts, 
				const char *progname, const char *description);

void cargo_destroy(cargo_t *ctx);

void cargo_set_description(cargo_t ctx, const char *description);

void cargo_set_epilog(cargo_t ctx, const char *epilog);

void cargo_add_help(cargo_t ctx, int add_help);

void cargo_set_format(cargo_t ctx, cargo_format_t format);

int cargo_add(cargo_t ctx,
				const char *opt,
				void *target,
				cargo_type_t type,
				const char *description);

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				void **target,
				cargo_type_t type,
				const char *description);

int cargo_addv(cargo_t ctx, 
				const char *opt,
				void *target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description);

int cargo_addv_alloc(cargo_t ctx, 
				const char *opt,
				void **target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description);

int cargo_parse(cargo_t ctx, int argc, char **argv);

int cargo_print_usage(cargo_t ctx);

int cargo_get_usage(cargo_t ctx, char **buf, size_t *buf_size);

char **cargo_get_args(cargo_t ctx, size_t *argc);



#endif // __CARGO_H__
