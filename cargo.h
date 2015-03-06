
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
// Colors.
//

#define CARGO_COLOR_BLACK			"\x1b[22;30m"
#define CARGO_COLOR_RED				"\x1b[22;31m"
#define CARGO_COLOR_GREEN			"\x1b[22;32m"
#define CARGO_COLOR_YELLOW			"\x1b[22;33m"
#define CARGO_COLOR_BLUE			"\x1b[22;34m"
#define CARGO_COLOR_MAGENTA			"\x1b[22;35m"
#define CARGO_COLOR_CYAN			"\x1b[22;36m"
#define CARGO_COLOR_GRAY			"\x1b[22;37m"
#define CARGO_COLOR_DARK_GRAY		"\x1b[01;30m"
#define CARGO_COLOR_LIGHT_RED		"\x1b[01;31m"
#define CARGO_COLOR_LIGHT_GREEN		"\x1b[01;32m"
#define CARGO_COLOR_LIGHT_BLUE		"\x1b[01;34m"
#define CARGO_COLOR_LIGHT_MAGNETA	"\x1b[01;35m"
#define CARGO_COLOR_LIGHT_CYAN		"\x1b[01;36m"
#define CARGO_COLOR_WHITE			"\x1b[01;37m"
#define CARGO_COLOR_RESET			"\x1b[0m"

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

typedef struct cargo_highlight_s
{
	int i;				// Index of highlight in argv.
	char *c;			// Highlight character (followed by color).
} cargo_highlight_t;

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
	CARGO_OPT_UNIQUE					= (1 << 0),
	CARGO_OPT_REQUIRED					= (1 << 1)
} cargo_option_flags_t;

typedef enum cargo_mutex_group_flags_e
{
	CARGO_MUTEXGRP_ONE_REQUIRED			= (1 << 0)
} cargo_mutex_group_flags_t;

typedef enum cargo_group_flags_e
{
	CARGO_GROUP_HIDE					= (1 << 0)
} cargo_group_flags_t;

typedef enum cargo_flags_e
{
	CARGO_AUTOCLEAN						= (1 << 0),
	CARGO_NOCOLOR 						= (1 << 1),
	CARGO_NOERR_OUTPUT					= (1 << 2),
	CARGO_NOERR_USAGE					= (1 << 3),
	CARGO_ERR_LONG_USAGE				= (1 << 4),
	CARGO_ERR_STDOUT					= (1 << 5)
} cargo_flags_t;

//
// Callback types.
//

typedef int (*cargo_custom_cb_t)(cargo_t ctx, void *user, const char *optname,
								int argc, char **argv);

//
// Functions.
//

int cargo_init_ex(cargo_t *ctx, const char *progname, cargo_flags_t flags);

int cargo_init(cargo_t *ctx, const char *progname);

void cargo_destroy(cargo_t *ctx);

void cargo_set_flags(cargo_t ctx, cargo_flags_t flags);

cargo_flags_t cargo_get_flags(cargo_t ctx);

int cargo_add_optionv_ex(cargo_t ctx, cargo_option_flags_t flags,
					  const char *optnames, 
					  const char *description,
					  const char *fmt, va_list ap);

int cargo_add_optionv(cargo_t ctx, const char *optnames, 
					  const char *description, const char *fmt, va_list ap);

int cargo_add_option_ex(cargo_t ctx, cargo_option_flags_t flags,
					const char *optnames, const char *description,
					const char *fmt, ...);

int cargo_add_option(cargo_t ctx, const char *optnames,
					 const char *description, const char *fmt, ...);

int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias);

int cargo_add_group(cargo_t ctx, cargo_group_flags_t flags, const char *name,
					const char *title, const char *description);

int cargo_group_add_option(cargo_t ctx, const char *group, const char *opt);

int cargo_add_mutex_group(cargo_t ctx,
						cargo_mutex_group_flags_t flags,
						const char *name);

int cargo_mutex_group_add_option(cargo_t ctx,
								const char *group,
								const char *opt);

int cargo_set_metavar(cargo_t ctx, const char *optname, const char *metavar);

int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv);

void cargo_set_option_count_hint(cargo_t ctx, size_t option_count);

void cargo_set_max_width(cargo_t ctx, size_t max_width);

void cargo_set_description(cargo_t ctx, const char *description);

void cargo_set_epilog(cargo_t ctx, const char *epilog);

void cargo_set_auto_help(cargo_t ctx, int auto_help);

void cargo_set_format(cargo_t ctx, cargo_format_t format);

int cargo_fprint_usage(cargo_t ctx, FILE *f);

int cargo_print_usage(cargo_t ctx);

// TODO: Return const these and store internally instead!
const char *cargo_get_usage(cargo_t ctx);

int cargo_fprint_short_usage(cargo_t ctx, FILE *f);

int cargo_print_short_usage(cargo_t ctx);

const char *cargo_get_short_usage(cargo_t ctx);

const char *cargo_get_error(cargo_t ctx);

const char **cargo_get_unknown(cargo_t ctx, size_t *unknown_count);

const char **cargo_get_args(cargo_t ctx, size_t *argc);

char *cargo_get_fprint_args(int argc, char **argv, int start,
							cargo_fprint_flags_t flags,
							size_t highlight_count, ...);

char *cargo_get_fprintl_args(int argc, char **argv, int start,
							cargo_fprint_flags_t flags,
							size_t highlight_count,
							const cargo_highlight_t *highlights);

char *cargo_get_vfprint_args(int argc, char **argv, int start,
							cargo_fprint_flags_t flags,
							size_t highlight_count, va_list ap);

int cargo_fprint_args(FILE *f, int argc, char **argv, int start,
					  cargo_fprint_flags_t flags,
					  size_t highlight_count, ...);

int cargo_fprintl_args(FILE *f, int argc, char **argv, int start,
					   cargo_fprint_flags_t flags, size_t highlight_count,
					   const cargo_highlight_t *highlights);

char **cargo_split_commandline(const char *args, int *argc);

void cargo_free_commandline(char ***argv, int argc);

#endif // __CARGO_H__
