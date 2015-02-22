
#ifndef __CARGO_H__
#define __CARGO_H__

#include <stdio.h>

// Please override any defaults in this header.
#ifdef CARGO_CONFIG
#include "cargo_config.h"
#endif

//
// Defaults start.
//

#ifndef CARGO_NAME_COUNT
#define CARGO_NAME_COUNT 4
#endif

#ifndef CARGO_DEFAULT_PREFIX
#define CARGO_DEFAULT_PREFIX "-"
#endif

#ifndef CARGO_DEFAULT_MAX_OPTS
#define CARGO_DEFAULT_MAX_OPTS 32
#endif

#ifndef CARGO_DEFAULT_MAX_WIDTH
#define CARGO_DEFAULT_MAX_WIDTH 80
#endif

#ifndef CARGO_AUTO_MAX_WIDTH
#define CARGO_AUTO_MAX_WIDTH 0
#endif

#ifndef CARGO_MAX_MAX_WIDTH
#define CARGO_MAX_MAX_WIDTH 1024
#endif

//
// Version.
//

#define CARGO_XSTRINGIFY(s) #s
#define CARGO_STRINGIFY(s) CARGO_XSTRINGIFY(s)

#define CARGO_MAJOR_VERSION 0
#define CARGO_MINOR_VERSION 1
#define CARGO_PATCH_VERSION 0

#define CARGO_VERSION                        \
	((CARGO_MAJOR_VERSION << 16) |           \
	(CARGO_MINOR_VERSION << 8)   |           \
	(CARGO_PATCH_VERSION))

#define CARGO_VERSION_STR                    \
	CARGO_STRINGIFY(CARGO_MAJOR_VERSION) "." \
	CARGO_STRINGIFY(CARGO_MINOR_VERSION) "." \
	CARGO_STRINGIFY(CARGO_PATCH_VERSION)

const char *cargo_get_version();


//
// Types.
//

typedef struct cargo_s *cargo_t;

// TODO: Keep these as enums?
typedef enum cargo_format_e
{
	CARGO_FORMAT_RAW_HELP				= (1 << 0),
	CARGO_FORMAT_RAW_DESCRIPTION 		= (1 << 1),
	CARGO_FORMAT_RAW_OPT_DESCRIPTION	= (1 << 2),
	CARGO_FORMAT_HIDE_DESCRIPTION		= (1 << 3),
	CARGO_FORMAT_HIDE_EPILOG			= (1 << 4),
	CARGO_FORMAT_HIDE_SHORT				= (1 << 5)
} cargo_format_t;

typedef enum cargo_fprint_flags_e
{
	CARGO_FPRINT_NOCOLOR				= (1 << 0),
	CARGO_FPRINT_NOARGS					= (1 << 1),
	CARGO_FPRINT_NOHIGHLIGHT			= (1 << 2)
} cargo_fprint_flags_t;

typedef enum cargo_option_flags_e
{
	CARGO_OPT_UNIQUE					= (1 << 0)
} cargo_option_flags_t;

#define CARGO_NARGS_ONE_OR_MORE 	-1
#define CARGO_NARGS_NONE_OR_MORE	-2 // TODO: Remove this?

//
// Functions.
//

int cargo_init(cargo_t *ctx, const char *progname);

void cargo_destroy(cargo_t *ctx);

int cargo_add_optionv_ex(cargo_t ctx, size_t flags, const char *optnames, 
					  const char *description, const char *fmt, va_list ap);

int cargo_add_optionv(cargo_t ctx, const char *optnames, 
					  const char *description, const char *fmt, va_list ap);

int cargo_add_option_ex(cargo_t ctx, size_t flags, const char *optnames,
					 const char *description, const char *fmt, ...);

int cargo_add_option(cargo_t ctx, const char *optnames,
					 const char *description, const char *fmt, ...);

int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias);

int cargo_set_metavar(cargo_t ctx, const char *optname, const char *metavar);

int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv);

void cargo_set_option_count_hint(cargo_t ctx, size_t option_count);

void cargo_set_max_width(cargo_t ctx, size_t max_width);

void cargo_set_description(cargo_t ctx, const char *description);

void cargo_set_epilog(cargo_t ctx, const char *epilog);

void cargo_set_auto_help(cargo_t ctx, int auto_help);

void cargo_set_format(cargo_t ctx, cargo_format_t format);

int cargo_fprint_usage(FILE *f, cargo_t ctx);

int cargo_print_usage(cargo_t ctx);

char *cargo_get_usage(cargo_t ctx, char *buf, size_t *buf_size);

char **cargo_get_unknown(cargo_t ctx, size_t *unknown_count);

char **cargo_get_args(cargo_t ctx, size_t *argc);

char *cargo_get_fprint_args(int argc, char **argv, int start, size_t flags,
							size_t highlight_count, ...);

char *cargo_get_vfprint_args(int argc, char **argv, int start, size_t flags,
							size_t highlight_count, va_list ap);

int cargo_fprint_args(FILE *f, int argc, char **argv, int start, size_t flags,
					  size_t highlight_count, ...);

#endif // __CARGO_H__
