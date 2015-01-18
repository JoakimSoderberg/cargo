
#ifndef __CARGO_H__
#define __CARGO_H__

#include <stdio.h>

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
		fprintf(stderr, fmt, ##__VA_ARGS__); \
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

typedef enum cargo_copy_type_e
{
	CARGO_STATIC = 0,
	CARGO_ALLOC = 1
} cargo_copy_type_t;

int cargo_init(cargo_t *ctx, size_t max_opts, 
				const char *progname, const char *description);

void cargo_destroy(cargo_t *ctx);

void cargo_set_description(cargo_t ctx, const char *description);

void cargo_set_epilog(cargo_t ctx, const char *epilog);

void cargo_add_help(cargo_t ctx, int add_help);

int cargo_add(cargo_t ctx,
				const char *opt,
				void *target,
				cargo_type_t type,
				const char *description);

int cargo_add_str(cargo_t ctx,
				const char *opt,
				void *target,
				size_t lenstr,
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

int cargo_addv_str(cargo_t ctx, 
				const char *opt,
				void *target,
				size_t *target_count,
				size_t lenstr,
				int nargs,
				const char *description);

int cargo_addv_alloc(cargo_t ctx, 
				const char *opt,
				void **target,
				size_t *target_count,
				int nargs,
				cargo_type_t type,
				const char *description);

int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv);

typedef enum cargo_format_e
{
	CARGO_FORMAT_RAW_HELP = (1 << 0),
	CARGO_FORMAT_RAW_DESCRIPTION = (1 << 1),
	CARGO_FORMAT_RAW_OPT_DESCRIPTION = (1 << 2),
	CARGO_FORMAT_HIDE_DESCRIPTION = (1 << 3),
	CARGO_FORMAT_HIDE_EPILOG = (1 << 4),
	CARGO_FORMAT_HIDE_SHORT = (1 << 5)
} cargo_format_t;

#define CARGO_DEFAULT_MAX_WIDTH 80

typedef struct cargo_usage_settings_s
{
	size_t max_width;
	cargo_format_t format;
} cargo_usage_settings_t;

int cargo_print_usage(cargo_t ctx);

int cargo_set_usage_settings(cargo_usage_settings_t settings);

int cargo_get_usage(cargo_t ctx, char **buf, size_t *buf_size);

char **cargo_get_unknown(cargo_t ctx, size_t *unknown_count);

char **cargo_get_args(cargo_t ctx, size_t *argc);

int cargo_add_alias(cargo_t ctx, const char *name, const char *alias);


#endif // __CARGO_H__
