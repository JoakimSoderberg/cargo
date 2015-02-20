#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include "cargo.h"
#include <stdarg.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif // _WIN32

#ifdef CARGO_DEBUG
// TODO: Add an inline macro as well...
#define CARGODBG(level, fmt, ...)											\
{																			\
	if (level <= CARGO_DEBUG)												\
	{																		\
		fprintf(stderr, "[cargo.c:%4d]: " fmt, __LINE__, ##__VA_ARGS__);	\
	}																		\
}

#define CARGODBGI(level, fmt, ...)											\
{																			\
	if (level <= CARGO_DEBUG)												\
	{																		\
		fprintf(stderr, fmt, ##__VA_ARGS__);								\
	}																		\
}

#else
#define CARGODBG(level, fmt, ...)
#define CARGODBGI(level, fmt, ...)
#endif

#ifndef va_copy
  #ifdef __va_copy
    #define va_copy __va_copy
  #else
    #define va_copy(a, b) memcpy(&(a), &(b), sizeof(va_list))
  #endif
#endif

int cargo_get_console_width()
{
	#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		return -1;
	}

	CARGODBG(3, "Console width: %d\n",
		(int)(csbi.srWindow.Right - csbi.srWindow.Left));

	return (int)(csbi.srWindow.Right - csbi.srWindow.Left);

	#else // Unix

	struct winsize w;
	ioctl(0, TIOCGWINSZ , &w);

	if (!ioctl(0, TIOCGWINSZ , &w))
	{
		return w.ws_col;
	}

	return -1;
	#endif // _WIN32
}

int cargo_vsnprintf(char *buf, size_t buflen, const char *format, va_list ap)
{
	int r;
	if (!buflen)
		return 0;
	#if defined(_MSC_VER) || defined(_WIN32)
	r = _vsnprintf(buf, buflen, format, ap);
	if (r < 0)
		r = _vscprintf(format, ap);
	#elif defined(sgi)
	/* Make sure we always use the correct vsnprintf on IRIX */
	extern int      _xpg5_vsnprintf(char * __restrict,
		__SGI_LIBC_NAMESPACE_QUALIFIER size_t,
		const char * __restrict, /* va_list */ char *);

	r = _xpg5_vsnprintf(buf, buflen, format, ap);
	#else
	r = vsnprintf(buf, buflen, format, ap);
	#endif
	buf[buflen - 1] = '\0';
	return r;
}

#if CARGO_HELPER // Only used by this...
int cargo_snprintf(char *buf, size_t buflen, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = cargo_vsnprintf(buf, buflen, format, ap);
	va_end(ap);
	return r;
}
#endif

char *cargo_strndup(const char *s, size_t n)
{
	char *res;
	size_t len = strlen(s);

	if (n < len)
	{
		len = n;
	}

	if (!(res = (char *) malloc(len + 1)))
	{
		return NULL;
	}

	res[len] = '\0';
	return (char *)memcpy(res, s, len);
}

typedef struct cargo_str_s
{
	char *s;
	size_t l;
	size_t offset;
} cargo_str_t;

int cargo_vappendf(cargo_str_t *str, const char *format, va_list ap)
{
	int ret;
	assert(str);

	if ((ret = cargo_vsnprintf(&str->s[str->offset],
			(str->l - str->offset), format, ap)) < 0)
	{
		return -1;
	}

	str->offset += ret;

	return ret;
}

int cargo_appendf(cargo_str_t *str, const char *fmt, ...)
{
	int ret;
	va_list ap;
	assert(str);
	va_start(ap, fmt);
	ret = cargo_vappendf(str, fmt, ap);
	va_end(ap);
	return ret;
}

typedef enum cargo_type_e
{
	CARGO_BOOL = 0,
	CARGO_INT = 1,
	CARGO_UINT = 2,
	CARGO_FLOAT = 3,
	CARGO_DOUBLE = 4,
	CARGO_STRING = 5
} cargo_type_t;

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
	char *name[CARGO_NAME_COUNT];
	size_t name_count;
	const char *description;
	const char *metavar;
	int optional;
	cargo_type_t type;
	int nargs;
	int alloc;
	void **target;
	size_t target_idx;
	size_t *target_count;
	size_t lenstr;
	size_t max_target_count;
} cargo_opt_t;

typedef struct cargo_s
{
	const char *progname;
	const char *description;
	const char *epilog;
	size_t max_width;
	cargo_format_t format;

	int i;	// argv index.
	int j;	// sub-argv index (when getting arguments for options)
	int argc;
	char **argv;

	int add_help;
	int help;

	cargo_opt_t *options;
	size_t opt_count;
	size_t max_opts;
	const char *prefix;

	char **unknown_opts;
	size_t unknown_opts_count;

	char **args;
	size_t arg_count;
} cargo_s;

static size_t _cargo_get_type_size(cargo_type_t t)
{
	assert((t >= CARGO_BOOL) && (t <= CARGO_STRING));

	switch (t)
	{
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

static int _cargo_starts_with_prefix(cargo_t ctx, const char *arg)
{
	return (strpbrk(arg, ctx->prefix) == arg);
}

static char _cargo_is_prefix(cargo_t ctx, char c)
{
	size_t i;
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

static int _cargo_find_option_name(cargo_t ctx, const char *name, 
									int *opt_i, int *name_i)
{
	size_t i;
	size_t j;
	cargo_opt_t *opt;

	// TODO: Hmm how about for positional arguments?
	if (!_cargo_starts_with_prefix(ctx, name))
		return -1;

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		for (j = 0; j < opt->name_count; j++)
		{
			if (!strcmp(opt->name[j], name))
			{
				if (opt_i) *opt_i = i;
				if (name_i) *name_i = j;
				return 0;
			}
		}
	}

	return -1; 
}

static int _cargo_add(cargo_t ctx,
				const char *opt,
				void **target,
				size_t *target_count,
				size_t lenstr,
				int nargs,
				cargo_type_t type,
				const char *description,
				int alloc)
{
	size_t opt_len;
	cargo_opt_t *o = NULL;
	char *optcpy = NULL;

	CARGODBG(2, "_cargo_add: %s\n", opt);

	if (!_cargo_nargs_is_valid(nargs))
	{
		CARGODBG(1, "nargs is invalid %d\n", nargs);
		return -1;
	}

	if (!opt)
	{
		CARGODBG(1, "Null option name\n");
		return -1;
	}

	if ((opt_len = strlen(opt)) == 0)
	{
		CARGODBG(1, "%s", "Option name has length 0\n");
		return -1;
	}

	if ((type != CARGO_STRING) && (nargs == 1) && alloc)
	{
		CARGODBG(1, "%s: Cannot allocate for single nonstring value\n", opt);
		return -1;
	}

	if (!target)
	{
		CARGODBG(1, "%s: target NULL\n", opt);
		return -1;
	}

	if (!target_count && (nargs > 1))
	{
		CARGODBG(1, "%s: target_count NULL, when nargs > 1\n", opt);
		return -1;
	}

	if (!ctx->options)
	{
		if (!(ctx->options = calloc(ctx->max_opts, sizeof(cargo_opt_t))))
		{
			CARGODBG(1, "Out of memory\n");
			return -1;
		}
	}

	if (!_cargo_find_option_name(ctx, opt, NULL, NULL))
	{
		CARGODBG(1, "%s already exists\n", opt);
		return -1;
	}

	if (ctx->opt_count >= ctx->max_opts)
	{
		cargo_opt_t *new_options = NULL;
		CARGODBG(2, "Option count (%lu) >= Max option count (%lu)\n",
			ctx->opt_count, ctx->max_opts);

		ctx->max_opts *= 2;

		if (!(new_options = realloc(ctx->options,
									 ctx->max_opts * sizeof(cargo_opt_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			return -1;
		}

		ctx->options = new_options;

		// The options struct is assumed to be zeroed.
		memset(&ctx->options[ctx->opt_count],
				0, (ctx->max_opts - ctx->opt_count) * sizeof(cargo_opt_t));
	}

	// TODO: assert for argument conflicts.
	if (!(optcpy = strdup(opt)))
	{
		CARGODBG(1, "Out of memory\n");
		return -1;
	}

	o = &ctx->options[ctx->opt_count];
	ctx->opt_count++;

	// Check if the option has a prefix
	// (this means it's optional).
	o->optional = _cargo_is_prefix(ctx, opt[0]);

	o->name[o->name_count++] = optcpy;
	o->nargs = nargs;
	o->target = target;
	o->type = type;
	o->description = description;
	o->target_count = target_count;
	o->lenstr = lenstr;

	// By default "nargs" is the max number of arguments the option
	// should parse. 
	if (nargs >= 0)
	{
		o->max_target_count = nargs;
	}
	else
	{
		o->max_target_count = (size_t)-1;
	}

	o->alloc = alloc;

	if (alloc)
	{
		*(o->target) = NULL;
	}

	if (o->target_count)
	{
		*(o->target_count) = 0;
	}

	CARGODBG(2, " cargo_add %s:\n", opt);
	CARGODBG(2, "   max_target_count = %lu\n", o->max_target_count);
	CARGODBG(2, "   alloc = %d\n", o->alloc);
	CARGODBG(2, "   lenstr = %lu\n", lenstr);
	CARGODBG(2, "   nargs = %d\n", nargs);
	CARGODBG(2, "   \n"); 

	return 0;
}

static const char *_cargo_is_option_name(cargo_t ctx, 
					cargo_opt_t *opt, const char *arg)
{
	size_t i;
	const char *name;

	if (!_cargo_starts_with_prefix(ctx, arg))
		return NULL;

	for (i = 0; i < opt->name_count; i++)
	{
		name = opt->name[i];

		CARGODBG(3, "    %10s == %s ?\n", name, arg);

		if (!strcmp(name, arg))
		{
			CARGODBG(3, "          Match\n");
			return name;
		}
	}

	return NULL;
}

static void _cargo_fprint_args(cargo_t ctx, FILE *f, int highlight)
{
	// TODO: Write this to a string buffer instead!
	int i;
	size_t indentopt = 0;
	size_t indentarg = 0;
	size_t curlen = 0;
	assert(ctx);

	for (i = 0; i < ctx->argc; i++)
	{
		fprintf(f, "%s ", ctx->argv[i]);

		if (highlight)
		{
			curlen = strlen(ctx->argv[i]) + 1; // + 1 for space.
			indentopt += (i < ctx->i) ? curlen : 0;
			indentarg += (i < ctx->j) ? curlen : 0;
		}
	}

	fprintf(f, "\n");

	if (highlight > 0)
	{
		const char argh[] = "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
		const char opth[] = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
		int arglen = (int)strlen(ctx->argv[ctx->j]);
		int optlen = (int)strlen(ctx->argv[ctx->i]);

		// Highlight option.
		fprintf(f, "%*s%*.*s", (int)indentopt, "", optlen, optlen, opth);

		// Highlight arg.
		fprintf(f, "%*s%*.*s\n",
			(int)(indentarg - indentopt - optlen), "", arglen, arglen, argh);
	}
}

static int _cargo_set_target_value(cargo_t ctx, cargo_opt_t *opt,
									const char *name, char *val)
{
	void *target;
	char *end = NULL;
	assert(ctx);
	assert((opt->type >= CARGO_BOOL) && (opt->type <= CARGO_STRING));

	if ((opt->type != CARGO_BOOL) 
		&& (opt->target_idx >= opt->max_target_count))
	{
		CARGODBG(1, "Target index out of bounds (%lu > max %lu)\n",
				opt->target_idx, opt->max_target_count);
		return 1;
	}

	CARGODBG(2, "_cargo_set_target_value:\n");
	CARGODBG(2, "  alloc: %d\n", opt->alloc);

	// If number of arguments is just 1 don't allocate an array.
	if (opt->alloc && (opt->nargs != 1))
	{
		// Allocate the memory needed.
		if (!*(opt->target))
		{
			void **new_target;
			int alloc_count = opt->nargs; 

			if (opt->nargs < 0)
			{
				// In this case we don't want to preallocate everything
				// since we might have "unlimited" arguments.
				// CARGO_NARGS_ONE_OR_MORE
				// CARGO_NARGS_NONE_OR_MORE
				// TODO: Don't allocate all of these right away.
				alloc_count = ctx->argc - ctx->i;
				assert(alloc_count >= 0);

				// Don't allocate more than necessary.
				if (opt->max_target_count < (size_t)alloc_count)
					alloc_count = opt->max_target_count;
			}

			if (!(new_target = (void **)calloc(alloc_count,
						_cargo_get_type_size(opt->type))))
			{
				CARGODBG(1, "Out of memory!\n");
				return -1;
			}

			CARGODBG(3, "Allocated %dx %s!\n",
					alloc_count, _cargo_type_map[opt->type]);

			*(opt->target) = new_target;
		}

		target = *(opt->target);
	}
	else
	{
		// Just a normal pointer.
		target = (void *)opt->target;
	}

	switch (opt->type)
	{
		case CARGO_BOOL:
		{
			CARGODBG(2, "%s", "      bool\n");
			((int *)target)[opt->target_idx] = 1;
			break;
		}
		case CARGO_INT:
		{
			CARGODBG(2, "      int %s\n", val);
			((int *)target)[opt->target_idx] = strtol(val, &end, 10);
			break;
		}
		case CARGO_UINT:
		{
			CARGODBG(2, "      uint %s\n", val);
			((unsigned int *)target)[opt->target_idx] = strtoul(val, &end, 10);
			break;
		}
		case CARGO_FLOAT:
		{
			CARGODBG(2, "      float %s\n", val);
			((float *)target)[opt->target_idx] = (float)strtof(val, &end);
			break;
		}
		case CARGO_DOUBLE:
		{
			CARGODBG(2, "      double %s\n", val);
			((double *)target)[opt->target_idx] = (double)strtod(val, &end);
			break;
		}
		case CARGO_STRING:
			CARGODBG(2, "      string \"%s\"\n", val);
			if (opt->alloc)
			{
				CARGODBG(2, "       ALLOCATED STRING\n");
				if (opt->lenstr == 0)
				{
					char **t = (char **)((char *)target + opt->target_idx * sizeof(char *));
					CARGODBG(2, "          COPY FULL STRING\n");

					if (!(*t = strdup(val)))
					{
						return -1;
					}
					CARGODBG(2, "          \"%s\"\n", *t);
				}
				else
				{
					CARGODBG(2, "          MAX LENGTH: %lu\n", opt->lenstr);
					if (!(((char **)target)[opt->target_idx] 
							= cargo_strndup(val, opt->lenstr)))
					{
						return -1;
					}
				}
			}
			else
			{
				char *t = (char *)((char *)target + opt->target_idx * opt->lenstr);
				CARGODBG(2, "       STATIC STRING, bufsize = %lu\n", opt->lenstr);
				strncpy(t, val, opt->lenstr);
				CARGODBG(2, "       \"%s\"\n", t);
			}
			break;
	}

	// This indicates error for the strtox functions.
	if (end == val)
	{
		CARGODBG(1, "Cannot parse \"%s\" as %s\n",
				val, _cargo_type_map[opt->type]);

		_cargo_fprint_args(ctx, stderr, ctx->i);
		fprintf(stderr, "Cannot parse \"%s\" as %s for option %s\n",
				val, _cargo_type_map[opt->type], ctx->argv[ctx->i]);
		return -1;
	}

	// TODO: For single options that are repeated this will cause write beyond buffer!
	// --bla 1 --bla 2 <-- Second call will be written outside
	opt->target_idx++;

	if (opt->target_count)
	{
		*opt->target_count = opt->target_idx;
	}

	return 0;
}

static int _cargo_is_another_option(cargo_t ctx, char *arg)
{
	size_t j;

	for (j = 0; j < ctx->opt_count; j++)
	{
		if (_cargo_is_option_name(ctx, &ctx->options[j], arg))
		{
			return 1;
		}
	}

	return 0;
}

static int _cargo_zero_args_allowed(cargo_opt_t *opt)
{
	return (opt->nargs == CARGO_NARGS_NONE_OR_MORE)
		|| (opt->type == CARGO_BOOL);
}

static int _cargo_parse_option(cargo_t ctx, cargo_opt_t *opt, const char *name,
								int argc, char **argv)
{
	int ret;
	int args_to_look_for;
	int start = (ctx->i + 1);

	CARGODBG(2, "------ Parse option %s ------\n", opt->name[0]);
	CARGODBG(2, "argc: %d\n", argc);
	CARGODBG(2, "i: %d\n", ctx->i);
	CARGODBG(2, "start: %d\n", start);
	
	// Keep looking until the end of the argument list.
	if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) ||
		(opt->nargs == CARGO_NARGS_NONE_OR_MORE))
	{
		args_to_look_for = (argc - start);

		if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) 
			&& (args_to_look_for == 0))
		{
			args_to_look_for = 1;
		}
	}
	else
	{
		// Read (number of expected arguments) - (read so far).
		args_to_look_for = (opt->nargs - opt->target_idx);
	}

	CARGODBG(3, "Looking for %d args\n", args_to_look_for);

	// Look for arguments for this option.
	if (((start + args_to_look_for) > argc) && !_cargo_zero_args_allowed(opt))
	{
		// TODO: Don't print this to stderr (callback or similar instead)
		int expected = (opt->nargs != CARGO_NARGS_ONE_OR_MORE) ? opt->nargs : 1;
		fprintf(stderr, "Not enough arguments for %s."
						" %d expected but got only %d\n", 
						name, expected, argc - start);
		return -1;
	}

	CARGODBG(3, "Start %d, End %d (argc %d, nargs %d)\n",
			start, (start + args_to_look_for), argc, opt->nargs);

	ctx->j = start;

	if (opt->nargs == 0)
	{
		if (_cargo_zero_args_allowed(opt))
		{
			if ((ret = _cargo_set_target_value(ctx, opt, name, argv[ctx->j])) < 0)
			{
				CARGODBG(1, "Failed to set value for no argument option\n");
				return -1;
			}
		}
	}
	else
	{
		// Read until we find another option, or we've "eaten" the
		// arguments we want.
		for (ctx->j = start; ctx->j < (start + args_to_look_for); ctx->j++)
		{
			CARGODBG(3, "    argv[%i]: %s\n", ctx->j, argv[ctx->j]);

			if (_cargo_is_another_option(ctx, argv[ctx->j]))
			{
				if ((ctx->j == ctx->i) && !_cargo_zero_args_allowed(opt))
				{
					fprintf(stderr, "No argument specified for %s. "
									"%d expected.\n",
									name, 
									(opt->nargs > 0) ? opt->nargs : 1);
					return -1;
				}

				// We found another option, stop parsing arguments
				// for this option.
				CARGODBG(3, "%s", "    Found other option\n");
				break;
			}

			if ((ret = _cargo_set_target_value(ctx, opt, name, argv[ctx->j])) < 0)
			{
				CARGODBG(1, "Failed to set target value for %s: \n", name);
				return -1;
			}

			// If we have exceeded opt->max_target_count
			// for CARGO_NARGS_NONE_OR_MORE or CARGO_NARGS_ONE_OR_MORE
			// we should stop so we don't eat all the remaining arguments.
			if (ret)
				break;
		}
	}

	// Number of arguments read.
	CARGODBG(2, "_cargo_parse_option return %d\n", (ctx->j - start));
	return (ctx->j - start); 
}

static const char *_cargo_check_options(cargo_t ctx,
					cargo_opt_t **opt,
					int argc, char **argv, int i)
{
	size_t j;
	const char *name = NULL;
	assert(opt);

	if (!_cargo_starts_with_prefix(ctx, argv[i]))
		return NULL;

	for (j = 0; j < ctx->opt_count; j++)
	{
		name = NULL;
		*opt = &ctx->options[j];

		if ((name = _cargo_is_option_name(ctx, *opt, argv[i])))
		{
			return name;
		}
	}

	*opt = NULL;

	return NULL;
}

static int _cargo_compare_strlen(const void *a, const void *b)
{
	const char *as = (const char *)a;
	const char *bs = (const char *)b;
	size_t alen = strlen(as);
	size_t blen = strlen(bs);
	return alen - blen;
}

static int _cargo_generate_metavar(cargo_t ctx, cargo_opt_t *opt, char *buf, size_t bufsize)
{
	int j = 0;
	int i = 0;
	char metavarname[20];
	cargo_str_t str = { buf, bufsize, 0 };
	assert(ctx);
	assert(opt);

	while (_cargo_is_prefix(ctx, opt->name[0][i]))
	{
		i++;
	}

	while (opt->name[0][i] && (j < (sizeof(metavarname) - 1)))
	{
		metavarname[j++] = toupper(opt->name[0][i++]);
	}

	metavarname[j] = '\0';

	if (opt->nargs < 0)
	{
		// List the number of arguments.
		if (cargo_appendf(&str, "%s [%s ...]", metavarname, metavarname) < 0)
			return -1;
	}
	else if (opt->nargs > 0)
	{
		if (cargo_appendf(&str, "%s", metavarname) < 0) return -1;

		for (i = 1; (int)i < opt->nargs; i++)
		{
			if (cargo_appendf(&str, "%s", metavarname) < 0) return -1;
		}
	}

	return 0;
}

static int _cargo_get_option_name_str(cargo_t ctx, cargo_opt_t *opt,
	char *namebuf, size_t buf_size)
{
	int ret = 0;
	size_t i;
	char **sorted_names = NULL;
	cargo_str_t str = { namebuf, buf_size, 0 };

	// Sort the names by length.
	{
		if (!(sorted_names = calloc(opt->name_count, sizeof(char *))))
		{
			CARGODBG(1, "%s", "Out of memory\n");
			return -1;
		}

		for (i = 0; i < opt->name_count; i++)
		{
			if (!(sorted_names[i] = strdup(opt->name[i])))
			{
				ret = -1; goto fail;
			}
		}

		qsort(sorted_names, opt->name_count,
			sizeof(char *), _cargo_compare_strlen);
	}

	// Print the option names.
	for (i = 0; i < opt->name_count; i++)
	{
		if (cargo_appendf(&str, "%s%s", 
			sorted_names[i],
			(i + 1 != opt->name_count) ? ", " : "") < 0)
		{
			goto fail;
		}
	}

	// If the option has an argument, add a "metavar".
	if (!_cargo_zero_args_allowed(opt))
	{
		char metavarbuf[256];
		const char *metavar = NULL;

		if (opt->metavar)
		{
			metavar = opt->metavar;
		}
		else
		{
			_cargo_generate_metavar(ctx, opt, metavarbuf, sizeof(metavarbuf));
			metavar = metavarbuf;
		}

		cargo_appendf(&str, " %s", metavar);
	}

	ret = strlen(namebuf);

fail:
	for (i = 0; i < opt->name_count; i++)
	{
		if (sorted_names[i])
		{
			free(sorted_names[i]);
		}
	}

	free(sorted_names);
	return ret;
}

static void _cargo_free_str_list(char ***s, size_t *count)
{
	size_t i;

	if (!s || !*s)
		goto done;

	// Only free elements if we have a count.
	if (count)
	{
		for (i = 0; i < *count; i++)
		{
			free((*s)[i]);
			(*s)[i] = NULL;
		}
	}

	free(*s);
	*s = NULL;
done:
	if (count)
		*count = 0;
}

static char **_cargo_split(const char *s, const char *splitchars, size_t *count)
{
	char **ss;
	size_t i = 0;
	char *p = NULL;
	char *scpy = NULL;
	size_t splitlen = strlen(splitchars);
	assert(count);

	*count = 0;

	if (!s)
		return NULL;

	if (!*s)
		return NULL;

	if (!(scpy = strdup(s)))
		return NULL;

	p = scpy;

	*count = 1;

	while (*p)
	{
		for (i = 0; i < (int)splitlen; i++)
		{
			if (*p == splitchars[i])
			{
				(*count)++;
				break;
			}
		}
		p++;
	}

	if (!(ss = calloc(*count, sizeof(char *))))
		return NULL;

	p = strtok(scpy, splitchars);
	i = 0;
	while (p)
	{
		if (!(ss[i] = strdup(p)))
		{
			goto fail;
		}

		p = strtok(NULL, splitchars);
		i++;
	}

	free(scpy);

	return ss;
fail:
	free(scpy);
	_cargo_free_str_list(&ss, count);
	return NULL;
}

static char *_cargo_linebreak(cargo_t ctx, const char *str, size_t width)
{
	char *s = strdup(str);
	char *start = s;
	char *prev = s;
	char *p = s;

	if (!s)
		return NULL;

	CARGODBG(3, "_cargo_linebreak (MAX WIDTH %lu):\n%lu\"%s\"\n\n",
		width, strlen(s), s);

	// TODO: If we already have a linebreak, use that instead of adding a new one.
	p = strpbrk(p, " \n");
	while (p != NULL)
	{
		CARGODBG(4, "(p - start) = %ld: %.*s\n",
			(p - start), (int)(p - start), start);

		// Restart on already existing explicit line breaks.
		if (*p == '\n')
		{
			CARGODBG(3, "EXISTING NEW LINE len %ld:\n%.*s\n\n",
				(p - start), (int)(p - start), start);
			start = p;
		}
		else if ((size_t)(p - start) > width)
		{
			// We found a word that goes beyond the width we're
			// aiming for, so add the line break before that word.
			*prev = '\n';
			CARGODBG(3, "ADD NEW LINE len %ld:\n%.*s\n\n",
				(prev - start), (int)(prev - start), start);
			start = prev;
			continue;
		}

		prev = p;
		p = strpbrk(p + 1, " \n");

		CARGODBG(4, "\n");
	}

	return s;
}

static void _cargo_add_help_if_missing(cargo_t ctx)
{
	assert(ctx);

	if (ctx->add_help && _cargo_find_option_name(ctx, "--help", NULL, NULL))
	{
		if (_cargo_add(ctx, "--help", (void **)&ctx->help,
			NULL, 0, 0, CARGO_BOOL, "Show this help.", 0))
		{
			return;
		}

		if (_cargo_find_option_name(ctx, "-h", NULL, NULL))
		{
			cargo_add_alias(ctx, "--help", "-h");
		}
	}
}

static int _cargo_damerau_levensthein_dist(const char *s, const char *t)
{
	#define d(i, j) dd[(i) * (m + 2) + (j) ]
	#define min(x, y) ((x) < (y) ? (x) : (y))
	#define min3(a, b, c) ((a) < (b) ? min((a), (c)) : min((b), (c)))
	#define min4(a, b, c, d) ((a) < (b) ? min3((a), (c), (d)) : min3((b), (c),(d)))

	int *dd;
	int DA[256];
	int i;
	int j;
	int cost;
	int i1;
	int j1;
	int DB;
	int n = (int)strlen(s);
	int m = (int)strlen(t);
	int max_dist = n + m;

	if (!(dd = (int *)malloc((n + 2) * (m + 2) * sizeof(int))))
	{
		return -1;
	}

	memset(DA, 0, sizeof(DA));

	d(0, 0) = max_dist;

	for (i = 0; i < (n + 1); i++)
	{
		d(i + 1, 1) = i;
		d(i + 1, 0) = max_dist;
	}

	for (j = 0; j < (m + 1); j++)
	{
		d(1, j + 1) = j;
		d(0, j + 1) = max_dist;
	}

	for (i = 1; i < (n + 1); i++)
	{
		DB = 0;

		for(j = 1; j < (m + 1); j++)
		{
			i1 = DA[t[j - 1]];
			j1 = DB;
			cost = ((s[ i - 1] == t[j - 1]) ? 0 : 1);

			if (cost == 0) 
				DB = j;

			d(i + 1, j + 1) = min4(d(i, j) + cost, 
							  d(i + 1, j) + 1,
							  d(i, j + 1) + 1, 
							  d(i1, j1) + (i - i1 - 1) + 1 + (j - j1 - 1));
		}

		DA[s[i - 1]] = i;
	}

	cost = d(n + 1, m + 1);
	free(dd);
	return cost;

	#undef d
}

const char *_cargo_find_closest_opt(cargo_t ctx, const char *unknown)
{
	size_t i;
	size_t j;
	size_t maxi = 0;
	size_t maxj = 0;
	int min_dist = INT_MAX;
	int dist = 0;
	char *name = NULL;

	unknown += strspn(unknown, ctx->prefix);

	for (i = 0; i < ctx->opt_count; i++)
	{
		for (j = 0; j < ctx->options[i].name_count; j++)
		{
			name = ctx->options[i].name[j];
			name += strspn(name, ctx->prefix);

			dist = _cargo_damerau_levensthein_dist(unknown, name);

			if (dist < min_dist)
			{
				min_dist = dist;
				maxi = i;
				maxj = j;
			}
		}
	}

	return (min_dist <= 1) ? ctx->options[maxi].name[maxj] : NULL;
}

static int _cargo_fit_optnames_and_description(cargo_t ctx, cargo_str_t *str,
				size_t i, int name_padding, int option_causes_newline, int max_name_len)
{
	size_t j;
	int ret = 0;
	assert(str);
	assert(ctx);

	char **desc_lines = NULL;
	size_t line_count = 0;

	int padding = 0;
	char *opt_description = _cargo_linebreak(ctx,
		ctx->options[i].description,
		ctx->max_width - 2
		- max_name_len - (2 * name_padding));

	if (!opt_description)
	{
		ret = -1; goto fail;
	}

	CARGODBG(3, "ctx->max_width - 2 - max_name_len - (2 * NAME_PADDING) =\n");
	CARGODBG(3, "%lu - 2 - %d - (2 * %d) = %lu\n",
		ctx->max_width, max_name_len,
		name_padding,
		ctx->max_width - 2 - max_name_len - (2 * name_padding));

	if (!(desc_lines = _cargo_split(opt_description, "\n", &line_count)))
	{
		ret = -1; goto fail;
	}

	for (j = 0; j < line_count; j++)
	{
		if ((j == 0) && !option_causes_newline)
		{
			// --theoption  Description <- First line of description.
			padding = 0;
		}
		else
		{
			// --theoption  Description
			//              continues here <- Now we want pre-padding.
			// ---------------------------------------------------------
			// --reallyreallyreallyreallylongoption
			//              Description    <- First line but pad anyway.
			//              continues here 
			padding = max_name_len + name_padding;
		}

		if (cargo_appendf(str, "  %*s%s\n",
			padding, "", desc_lines[j]) < 0)
		{
			ret = -1; goto fail;
		}
	}

fail:
	if (opt_description)
	{
		free(opt_description);
	}

	_cargo_free_str_list(&desc_lines, &line_count);

	return ret;
}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

void cargo_set_max_width(cargo_t ctx, size_t max_width)
{
	int console_width;

	ctx->max_width = CARGO_DEFAULT_MAX_WIDTH;

	if (max_width == CARGO_AUTO_MAX_WIDTH)
	{
		CARGODBG(2, "User specified CARGO_AUTO_MAX_WIDTH\n");

		if ((console_width = cargo_get_console_width()) > 0)
		{
			CARGODBG(2, "Max width based on console width: %d\n", console_width);
			ctx->max_width = console_width;
		}
		else
		{
			CARGODBG(2, "Max width set to CARGO_DEFAULT_MAX_WIDTH = %d\n",
					CARGO_DEFAULT_MAX_WIDTH);
		}
	}
	else
	{
		CARGODBG(2, "User specified max width: %lu\n", max_width);
		ctx->max_width = max_width;
	}

	// Since we allocate memory based on this later on, make sure this
	// is something sane. Anything above 1024 characters is more than enough.
	// Also some systems might have a really big console width for instance.
	// (This happened on drone.io continous integration service).
	if (ctx->max_width > CARGO_MAX_MAX_WIDTH)
	{
		CARGODBG(2, "Max width too large, capping at: %d\n", CARGO_MAX_MAX_WIDTH);
		ctx->max_width = CARGO_MAX_MAX_WIDTH;
	}

	CARGODBG(2, "Usage max width: %lu\n", ctx->max_width);
}

int cargo_init(cargo_t *ctx, const char *progname)
{
	int console_width = -1;
	cargo_s *c;
	assert(ctx);

	*ctx = (cargo_s *)calloc(1, sizeof(cargo_s));
	c = *ctx;

	if (!c)
		return -1;

	c->max_opts = CARGO_DEFAULT_MAX_OPTS;

	c->progname = progname;
	c->add_help = 1;
	c->prefix = CARGO_DEFAULT_PREFIX;
	cargo_set_max_width(c, CARGO_AUTO_MAX_WIDTH);

	return 0;
}

void cargo_destroy(cargo_t *ctx)
{
	size_t i;
	size_t j;

	CARGODBG(2, "DESTROY!\n");

	if (ctx)
	{
		cargo_t c = *ctx;

		if (c->options)
		{
			CARGODBG(2, "DESTROY %lu options!\n", c->opt_count);

			for (i = 0; i < c->opt_count; i++)
			{
				CARGODBG(2, "Free opt: %s\n", c->options[i].name[0]);

				for (j = 0; j < c->options[i].name_count; j++)
				{
					free(c->options[i].name[j]);
				}

				c->options[i].name_count = 0;
			}

			free(c->options);
			c->options = NULL;
		}

		_cargo_free_str_list(&c->args, NULL);
		_cargo_free_str_list(&c->unknown_opts, NULL);

		free(*ctx);
		ctx = NULL;
	}
}

void cargo_set_option_count_hint(cargo_t ctx, size_t option_count)
{
	assert(ctx);
	if (ctx->opt_count == 0)
		ctx->max_opts = option_count;
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
	ctx->epilog = epilog;
}

void cargo_set_auto_help(cargo_t ctx, int auto_help)
{
	assert(ctx);
	ctx->add_help = auto_help;
}

void cargo_set_format(cargo_t ctx, cargo_format_t format)
{
	assert(ctx);
	ctx->format = format;
}

int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv)
{
	int i;
	int start;
	int opt_arg_count;
	char *arg;
	const char *name;
	cargo_opt_t *opt = NULL;

	CARGODBG(2, "============ Cargo Parse =============\n");

	ctx->argc = argc;
	ctx->argv = argv;

	_cargo_add_help_if_missing(ctx);

	_cargo_free_str_list(&ctx->args, NULL);
	ctx->arg_count = 0;

	_cargo_free_str_list(&ctx->unknown_opts, NULL);
	ctx->unknown_opts_count = 0;

	// Make sure we start over, if this function is
	// called more than once.
	for (i = 0; i < ctx->opt_count; i++)
	{
		ctx->options[i].target_idx = 0;
	}

	if (!(ctx->args = (char **)calloc(argc, sizeof(char *))))
	{
		CARGODBG(1, "Out of memory!\n");
		return -1;
	}

	if (!(ctx->unknown_opts = (char **)calloc(argc, sizeof(char *))))
	{
		CARGODBG(1, "Out of memory!\n");
		return -1;
	}

	CARGODBG(2, "Parse arg list of count %d start at index %d\n", argc, start_index);

	for (ctx->i = start_index; ctx->i < argc; ctx->i++)
	{
		arg = argv[ctx->i];
		start = ctx->i;

		CARGODBG(3, "\n");
		CARGODBG(3, "argv[%d] = %s\n", ctx->i, arg);
		CARGODBG(3, "  Look for opt matching %s:\n", arg);

		if ((name = _cargo_check_options(ctx, &opt, argc, argv, ctx->i)))
		{
			// We found an option, parse any arguments it might have.
			if ((opt_arg_count = _cargo_parse_option(ctx, opt, name,
													argc, argv)) < 0)
			{
				CARGODBG(1, "Failed to parse %s option: %s\n",
						_cargo_type_map[opt->type], name);
				return -1;
			}

			ctx->i += opt_arg_count;
		}
		else
		{
			// TODO: Optional to fail on unknown options!
			if (_cargo_is_prefix(ctx, argv[ctx->i][0]))
			{
				CARGODBG(2, "    Unknown option: %s\n", argv[ctx->i]);
				ctx->unknown_opts[ctx->unknown_opts_count] = argv[ctx->i];
				ctx->unknown_opts_count++;
			}
			else
			{
				// Normal argument.
				CARGODBG(2, "    Extra argument: %s\n", argv[ctx->i]);
				ctx->args[ctx->arg_count] = argv[ctx->i];
				ctx->arg_count++;
			}
		}

		#if CARGO_DEBUG
		{
			int k = 0;
			int ate = (ctx->i - start) + 1;

			CARGODBG(2, "    Ate %d args: ", ate);

			for (k = start; k < (start + ate ); k++)
			{
				CARGODBGI(2, "\"%s\" ", argv[k]);
			}

			CARGODBGI(2, "%s", "\n");
		}
		#endif // CARGO_DEBUG
	}

	if (ctx->unknown_opts_count > 0)
	{
		size_t i;
		const char *suggestion;
		// TODO: Don't print to stderr here, instead enable getting as a string.
		CARGODBG(2, "Unknown options count: %lu\n", ctx->unknown_opts_count);
		fprintf(stderr, "Unknown options:\n");

		for (i = 0; i < ctx->unknown_opts_count; i++)
		{
			suggestion = _cargo_find_closest_opt(ctx, ctx->unknown_opts[i]);
			fprintf(stderr, "%s ", ctx->unknown_opts[i]);
			if (suggestion) fprintf(stderr, " (Did you mean %s)?", suggestion);
			fprintf(stderr, "\n");
		}

		return -1;
	}

	if (ctx->help)
	{
		cargo_print_usage(ctx);
		return 1;
	}

	return 0;
}

char **cargo_get_unknown(cargo_t ctx, size_t *unknown_count)
{
	assert(ctx);

	if (unknown_count)
	{
		*unknown_count = ctx->unknown_opts_count;
	}

	return ctx->unknown_opts;
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

int cargo_add_alias(cargo_t ctx, const char *optname, const char *alias)
{
	int opt_i;
	int name_i;
	cargo_opt_t *opt;
	assert(ctx);

	if (_cargo_find_option_name(ctx, optname, &opt_i, &name_i))
	{
		CARGODBG(1, "Failed alias %s to %s, not found.\n", optname, alias);
		return -1;
	}

	CARGODBG(2, "Found option \"%s\"\n", optname);

	opt = &ctx->options[opt_i];

	if ((opt->name_count + 1) >= CARGO_NAME_COUNT)
	{
		CARGODBG(1, "Too many aliases for option: %s\n", opt->name[0]);
		return -1;
	}

	if (!(opt->name[opt->name_count] = strdup(alias)))
	{
		CARGODBG(1, "Out of memory\n");
		return -1;
	}

	opt->name_count++;

	CARGODBG(2, "  Added alias \"%s\"\n", alias);

	return 0;
}

int cargo_set_metavar(cargo_t ctx, const char *optname, const char *metavar)
{
	int opt_i;
	int name_i;
	cargo_opt_t *opt;
	assert(ctx);

	if (_cargo_find_option_name(ctx, optname, &opt_i, &name_i))
	{
		CARGODBG(1, "Failed to find option \"%s\"\n", optname);
		return -1;
	}

	opt = &ctx->options[opt_i];
	opt->metavar = metavar;

	return 0;
}

char *cargo_get_usage(cargo_t ctx, char *buf, size_t *buf_size)
{
	char *ret = NULL;
	size_t i;
	char *b = NULL;
	char **namebufs = NULL;
	int usagelen = 0;
	int namelen;
	int max_name_len = 0;
	cargo_str_t str;
	assert(ctx);
	#define MAX_OPT_NAME_LEN 40
	#define NAME_PADDING 2

	if (buf && !buf_size)
	{
		CARGODBG(1, "cargo_get_usage: buf requires buf_size\n");
		return NULL;
	}

	_cargo_add_help_if_missing(ctx);

	// First get option names and their length.
	// We get the widest one so we know the column width to use
	// for the final result.
	//   --option_a         Some description.
	//   --longer_option_b  Another description...
	// ^-------------------^
	// What should the above width be.
	if (!(namebufs = calloc(ctx->opt_count, sizeof(char *))))
	{
		CARGODBG(1, "Out of memory allocating %lu options!\n", ctx->opt_count);
		return NULL;
	}

	for (i = 0; i < ctx->opt_count; i++)
	{
		if (!(namebufs[i] = malloc(ctx->max_width)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		namelen = _cargo_get_option_name_str(ctx, &ctx->options[i], 
											namebufs[i], ctx->max_width);

		if (namelen < 0)
		{
			goto fail;
		}

		// Get the longest option name.
		// (However, if it's too long don't count it, then we'll just
		// do a line break before printing the description).
		if ((namelen > max_name_len) && (namelen <= MAX_OPT_NAME_LEN))
		{
			max_name_len = namelen;
		}

		usagelen += namelen + strlen(ctx->options[i].description);
	}

	if (ctx->description && !(ctx->format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		usagelen += strlen(ctx->description);
	}

	if (ctx->epilog && !(ctx->format & CARGO_FORMAT_HIDE_EPILOG))
	{
		usagelen += strlen(ctx->epilog);
	}

	// Add some extra to fit padding.
	// TODO: Let's be more strict here and use the exact padding instead.
	usagelen = (int)(usagelen * 2.5);

	// Make sure we where given a buffer that is large enough.
	if (buf_size)
	{
		if ((int)(*buf_size) < usagelen)
		{
			CARGODBG(1, "buf_size = %lu too small, must be %d\n",
					*buf_size, usagelen);

			*buf_size = usagelen;
			goto fail;
		}

		if (!buf)
		{
			*buf_size = usagelen;
			CARGODBG(1, "Got no buf, only returning buf_size %lu\n", *buf_size);
			goto fail;
		}

		b = buf;
	}
	else
	{
		// Allocate the final buffer.
		if (!(b = malloc(usagelen)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}
	}

	str.s = b;
	str.l = usagelen;
	str.offset = 0;

	if(ctx->description && !(ctx->format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		if (cargo_appendf(&str, "%s\nn", ctx->description) < 0) goto fail;
	}

	if (cargo_appendf(&str,  "Optional arguments:\n") < 0) goto fail;

	CARGODBG(2, "max_name_len = %d, ctx->max_width = %lu\n",
		max_name_len, ctx->max_width);

	// Option names + descriptions.
	for (i = 0; i < ctx->opt_count; i++)
	{
		// Is the option name so long we need a new line before the description?
		int option_causes_newline = (int)strlen(namebufs[i]) > max_name_len;

		// Print the option names.
		// "  --ducks [DUCKS ...]  "
		if (cargo_appendf(&str, "%*s%-*s%s",
				NAME_PADDING, " ",
				max_name_len, namebufs[i],
				(option_causes_newline ? "\n" : "")) < 0)
		{
			goto fail;
		}

		// Option description.
		if ((ctx->format & CARGO_FORMAT_RAW_OPT_DESCRIPTION)
			|| (strlen(ctx->options[i].description) < ctx->max_width))
		{
			if (cargo_appendf(&str, "%*s%s\n",
				NAME_PADDING, "", ctx->options[i].description) < 0)
			{
				goto fail;
			}
		}
		else
		{
			// Add line breaks to fit the width we want.
			if (_cargo_fit_optnames_and_description(ctx, &str, i,
					NAME_PADDING, option_causes_newline, max_name_len))
			{
				goto fail;
			}
		}
	}

	if (ctx->epilog && !(ctx->format & CARGO_FORMAT_HIDE_EPILOG))
	{
		if (cargo_appendf(&str, "%s\n", ctx->epilog) < 0) goto fail;
	}

	ret = b;

fail:
	// A real failure!
	if (!ret)
	{
		if (!buf && !buf_size && b)
		{
			free(b);
		}
	}

	{
		// We don't want to zero ctx->opt_count.
		size_t opt_count = ctx->opt_count;
		_cargo_free_str_list(&namebufs, &opt_count);
	}

	return ret;
}

int cargo_fprint_usage(FILE *f, cargo_t ctx)
{
	char *buf;
	assert(ctx);

	if (!(buf = cargo_get_usage(ctx, NULL, NULL)))
	{
		CARGODBG(1, "Out of memory!\n");
		return -1;
	}

	fprintf(f, "%s\n", buf);
	free(buf);

	return 0;
}

int cargo_print_usage(cargo_t ctx)
{
	return cargo_fprint_usage(stdout, ctx);	
}

// TODO: Add cargo_print_short_usage(cargo_t ctx)
// program [-h] [-s]  

// ARRAYS:
// -------
// 4 required
// float *flist;
// size_t fcount;
// cargo_add_option(cargo, "-s --sum", CARGO_ALLOC, "[f4]", &flist, &fcount);
//
// 0 or up to 4.
// float flist[4];
// size_t fcount;
// cargo_add_option(cargo, "-s --sum", CARGO_STATIC, "[f0-4]", flist, &fcount);
//
// 2 or more
// float *flist;
// size_t fcount;
// cargo_add_option(cargo, "-s --sum", CARGO_ALLOC, "![f2+]", &flist, &fcount);
//
// Multiple items (4 required dcount = number read)
// int d[4];
// size_t dcount;
// cargo_add_option(cargo, "-e --errors", CARGO_STATIC, "i i i i #",
//					&d[0], &d[1], &d[2], &d[3], &dcount);
//
// Flag:
// int all;
// cargo_add_option("-a --all", CARGO_STATIC, "b", &all);
// 
// String (optional argument)
// char laddr[256];
// cargo_add_option("-l --listen", CARGO_STATIC, "s?",
// 					laddr, sizeof(laddr));
//
// char *laddr;
// cargo_add_option("-l --listen", CARGO_ALLOC, "=s?:", &laddr, "example.com");
//
// =s?     Allocate optional string    const char *s;    &s
// =s      Allocate string             const char *s;    &s
// s10     Static string size 10 bytes char s[10];       &s
// s#      Static string size # bytes  char s[10];       &s, 10
// f       Required float              float f;          &f
// f?:     Required float              float f;          &f, 0.3
// f?      Optional float              float f;          &f
// f4      4 required                  float f1;         &f1, &f2, &f3, &f4
//                                     float f2;
//                                     float f3;
//                                     float f4;
// [f4+]   4 or more static            float f[20];      &f, &fc, max,
//                                     size_t fc;
//                                     size_t max = 20;
// [s10,4]    4  static strings           char strs[4][10]; &strs, &count,
//          with buflen of 10.         size_t count;
// [s10,4] 4 static strings            char strs[4][10]; &strs, &count, max
//          with buflen of 10.         size_t count;
//                                     size_t max = 4;
// [s#,4]   4 static strings           char strs[4][10]; &strs, &count, lenstr, max
//          with buflen of #.          size_t count;
//          Specify lenstr via var     size_t lenstr = 10;
//                                     size_t max = 4;
// =[s+]   Alloc 1 or more strings
// =[s4]   4 alloc strings             char *strs;       &strs, &count
//                                     size_t count;
// =[s,4]  4 alloc strings             char *strs;       &strs, &count
//                                     size_t count;
// =[f+]  4 or more allocated.        float *f;         &f, &fc      
//                                     size_t fc:
// =[f*]   None or more allocated.     float *f;         &f, &fc
//                                     size_t fc;
// [f*]    None or more static.        float f[40];      &f, &fc, max
//                                     size_t fc;
//                                     size_t max = 40;
// f_      Validate function           float f;          &f, valf
//                                     validate_f valf;
// =[s#]

typedef struct cargo_fmt_token_s
{
	int column;
	char token;
} cargo_fmt_token_t;

typedef struct cargo_fmt_scanner_s
{
	char *opt;
	const char *fmt;
	const char *start;
	cargo_fmt_token_t prev_token;
	cargo_fmt_token_t token;
	cargo_fmt_token_t next_token;

	int column;

	int array;
	int alloc;
} cargo_fmt_scanner_t;

static char _token(cargo_fmt_scanner_t *s)
{
	CARGODBG(4, "TOKEN: '%c'\n", s->token.token);
	return s->token.token;
}

static void _cargo_fmt_scanner_init(cargo_fmt_scanner_t *s,
									char *opt, const char *fmt)
{
	assert(s);
	memset(s, 0, sizeof(cargo_fmt_scanner_t));

	CARGODBG(2, "FMT scanner init: \"%s\"\n", fmt);
	s->opt = opt;
	s->fmt = fmt;
	s->start = fmt;
	s->column = 0;

	s->token.token = *fmt;
}

static void _next_token(cargo_fmt_scanner_t *s)
{
	const char *fmt;

	CARGODBG(4, "\"%s\"\n", s->start);
	CARGODBG(4, " %*s\n", s->token.column, "^");

	s->prev_token = s->token;

	s->column++;

	fmt = s->fmt;
	while ((*fmt == ' ') || (*fmt == '\t'))
	{
		s->column++;
		fmt++;
	}

	s->token.token = *fmt;
	s->token.column = s->column;

	fmt++;
	s->fmt = fmt;
}

static void _prev_token(cargo_fmt_scanner_t *s)
{
	CARGODBG(4, "PREV TOKEN\n");
	s->next_token = s->token;
	s->token = s->prev_token;
	s->fmt = &s->start[s->token.column];

	CARGODBG(4, "\"%s\"\n", s->start);
	CARGODBG(4, " %*s\n", s->token.column, "^");
}

int cargo_add_optionv_ex(cargo_t ctx, size_t flags, const char *optnames, 
					  const char *description, const char *fmt, va_list ap)
{
	size_t optcount = 0;
	char **optname_list = NULL;
	cargo_type_t type;
	void *target = NULL;
	size_t *target_count = NULL;
	int ret = 0;
	char *tmp = NULL;
	size_t lenstr = 0;
	int nargs;
	size_t i;
	cargo_fmt_scanner_t s;
	int array = 0;
	assert(ctx);

	CARGODBG(2, "ADD OPTION VARIADIC\n");

	if (!(tmp = strdup(optnames)))
	{
		return -1;
	}

	if (!(optname_list = _cargo_split(tmp, " ", &optcount))
		|| (optcount <= 0))
	{
		CARGODBG(1, "Failed to split option name list: \"%s\"\n", optnames);
		ret = -1; goto fail;
	}

	CARGODBG(3, "Got %lu option names:\n", optcount);
	#ifdef CARGODBG
	for (i = 0; i < optcount; i++)
	{
		CARGODBG(3, " %s\n", optname_list[i]);
	}
	#endif

	// Start parsing the format string.
	_cargo_fmt_scanner_init(&s, optname_list[0], fmt);
	_next_token(&s);

	// Get the first token.
	if (_token(&s) == '.')
	{
		CARGODBG(2, "Static\n");
		s.alloc = 0;
		_next_token(&s);
	}
	else
	{
		s.alloc = 1;
	}

	if (_token(&s) == '[')
	{
		CARGODBG(4, "   [ ARRAY\n");
		s.array = 1;
		_next_token(&s);
	}

	switch (_token(&s))
	{
		case 's':
		{
			type = CARGO_STRING;
			target = (void *)va_arg(ap, char *);

			CARGODBG(4, "Read string\n");
			_next_token(&s);

			if (_token(&s) == '#')
			{
				lenstr = (size_t)va_arg(ap, int);
				CARGODBG(4, "String length: %lu\n", lenstr);

				if (s.alloc)
				{
					CARGODBG(1, "%s: WARNING! Usually restricting the size of a "
						"string using # is only done on static strings.\n"
						"    Are you sure you want this?\n",
						s.opt);
					CARGODBG(1, "      \"%s\"\n", s.start);
					CARGODBG(1, "       %*s\n", s.column, "^");
				}
			}
			else
			{
				lenstr = 0;
				_prev_token(&s);
			}
			break;
		}
		case 'i': type = CARGO_INT;    target = (void *)va_arg(ap, void *); break;
		case 'd': type = CARGO_DOUBLE; target = (void *)va_arg(ap, void *); break;
		case 'b': type = CARGO_BOOL;   target = (void *)va_arg(ap, void *); break;
		case 'u': type = CARGO_UINT;   target = (void *)va_arg(ap, void *); break;
		case 'f': type = CARGO_FLOAT;  target = (void *)va_arg(ap, void *); break;
		default:
		{
			CARGODBG(1, "  %s: Unknown format character '%c' at index %d\n",
					optname_list[0], _token(&s), s.column);
			CARGODBG(1, "      \"%s\"\n", fmt);
			CARGODBG(1, "       %*s\n", s.column, "^");
			ret = -1; goto fail;
		}
	}

	if (s.array)
	{
		target_count = va_arg(ap, size_t *);
		*target_count = 0;

		_next_token(&s);

		CARGODBG(4, "Look for ']'\n");

		if (_token(&s) != ']')
		{
			CARGODBG(1, "%s: Expected ']'\n", optname_list[0]);
			CARGODBG(1, "      \"%s\"\n", fmt);
			CARGODBG(1, "        %*s\n", s.column, "^");
			ret = -1; goto fail;
		}

		_next_token(&s);

		switch (_token(&s))
		{
			case '*': nargs = CARGO_NARGS_NONE_OR_MORE; break;
			case '+': nargs = CARGO_NARGS_ONE_OR_MORE;  break;
			case '#': nargs = va_arg(ap, int); break;
			default:
			{
				CARGODBG(1, "  %s: Unknown format character '%c' at index %d\n",
						optname_list[0], _token(&s), s.column);
				CARGODBG(1, "      \"%s\"\n", fmt);
				CARGODBG(1, "       %*s\n", s.column, "^");
				ret = -1; goto fail;
			}
		}

		CARGODBG(2, "  %s: nargs = %d\n", optname_list[0], nargs);

		// TODO: Change this?
		*target_count = nargs;
	}
	else
	{
		// BOOLs never have arguments.
		nargs = (type == CARGO_BOOL) ? 0 : 1;

		// Never allocate single values (unless it's a string).
		s.alloc = (type != CARGO_STRING) ? 0 : s.alloc;
	}

	CARGODBG(2, "Add option: nargs %d\n", nargs);

	// .[s#]#    char s[5][10]; size_t c;    &s, &c, 5, 10  // 5 static str, len 10
 	// X[s#]+    char *s[10];   size_t c;    &s, 10, &c     // Alloc 1 or more strings of len 10
 	// X[s#]#    char *s[10];   size_t c;    &s, 10, &c, 5  // Alloc 5 str of len 10 
	// [s]+      char **s;      size_t c;    &s, &c         // Alloc 1 or more strings.
	// [s]#      char **s       size_t c;    &s, &c, 10     // Alloc 10 strings of any size.
	// [s]#      char **s;      size_t c;    &s, &c, 5      // Alloc 5 strings
	// .[f]#
	if (!s.alloc && s.array && (nargs < 0))
	{
		CARGODBG(1, "  %s: Static list requires a fixed size (#)\n", optname_list[0]);
		CARGODBG(1, "      \"%s\"\n", fmt);
		CARGODBG(1, "       %*s\n", s.column, "^");
		ret = -1; goto fail;
	}

	if (!(flags & CARGO_FLAG_VALIDATE_ONLY))
	{
		if ((ret = _cargo_add(ctx, optname_list[0],
						target, target_count, lenstr,
						nargs, type, description,
						s.alloc)))
		{
			goto fail;
		}

		for (i = 1; i < optcount; i++)
		{
			if (cargo_add_alias(ctx, optname_list[0], optname_list[i]))
			{
				ret = -1; goto fail;
			}
		}
	}

fail:
	if (tmp)
	{
		free(tmp);
	}

	if (optname_list)
	{
		_cargo_free_str_list(&optname_list, &optcount);
	}
	
	return ret;
}

int cargo_add_optionv(cargo_t ctx, const char *optnames, 
					  const char *description, const char *fmt, va_list ap)
{
	return cargo_add_optionv_ex(ctx, 0, optnames, description, fmt, ap);
}

int cargo_add_option(cargo_t ctx, const char *optnames,
					 const char *description, const char *fmt, ...)
{
	int ret;
	va_list ap;
	assert(ctx);

	va_start(ap, fmt);
	ret = cargo_add_optionv(ctx, optnames, description, fmt, ap);
	va_end(ap);

	return ret;
}

// -----------------------------------------------------------------------------
// Tests.
// -----------------------------------------------------------------------------
// LCOV_EXCL_START
#ifdef CARGO_TEST

#ifdef _WIN32
#define ANSI_COLOR_BLACK			""
#define ANSI_COLOR_RED				""
#define ANSI_COLOR_GREEN			""
#define ANSI_COLOR_YELLOW			""
#define ANSI_COLOR_BLUE				""
#define ANSI_COLOR_MAGENTA			""
#define ANSI_COLOR_CYAN				""
#define ANSI_COLOR_GRAY				""
#define ANSI_COLOR_DARK_GRAY		""
#define ANSI_COLOR_LIGHT_RED		""
#define ANSI_COLOR_LIGHT_GREEN		""
#define ANSI_COLOR_LIGHT_BLUE		""
#define ANSI_COLOR_LIGHT_MAGNETA	""
#define ANSI_COLOR_LIGHT_CYAN		""
#define ANSI_COLOR_WHITE			""
#define ANSI_COLOR_RESET			""
#else
#define ANSI_COLOR_BLACK			"\x1b[22;30m"
#define ANSI_COLOR_RED				"\x1b[22;31m"
#define ANSI_COLOR_GREEN			"\x1b[22;32m"
#define ANSI_COLOR_YELLOW			"\x1b[22;33m"
#define ANSI_COLOR_BLUE				"\x1b[22;34m"
#define ANSI_COLOR_MAGENTA			"\x1b[22;35m"
#define ANSI_COLOR_CYAN				"\x1b[22;36m"
#define ANSI_COLOR_GRAY				"\x1b[22;37m"
#define ANSI_COLOR_DARK_GRAY		"\x1b[01;30m"
#define ANSI_COLOR_LIGHT_RED		"\x1b[01;31m"
#define ANSI_COLOR_LIGHT_GREEN		"\x1b[01;32m"
#define ANSI_COLOR_LIGHT_BLUE		"\x1b[01;34m"
#define ANSI_COLOR_LIGHT_MAGNETA	"\x1b[01;35m"
#define ANSI_COLOR_LIGHT_CYAN		"\x1b[01;36m"
#define ANSI_COLOR_WHITE			"\x1b[01;37m"
#define ANSI_COLOR_RESET			"\x1b[0m"
#endif

#define cargo_assert(test, message) \
do 									\
{									\
	if (!(test))					\
	{								\
		msg = message;				\
		goto fail;					\
	}								\
} while (0)

#define cargo_assert_array(count, expected_count, array, array_expected)	\
do 																			\
{																			\
	size_t k;																\
	printf("Check that \""#array"\" has "#expected_count" elements\n");		\
	cargo_assert((count) == (expected_count), 								\
		#array" array count "#count" is not expected "#expected_count);		\
	for (k = 0; k < (count); k++)											\
	{																		\
		cargo_assert((array)[k] == (array_expected)[k],						\
			"Array contains unexpected value");								\
	}																		\
} while (0)

#define cargo_assert_str_array(count, expected_count, array, array_expected)	\
do 																				\
{																				\
	size_t k;																	\
	printf("Check that \""#array"\" has "#expected_count" elements\n");			\
	cargo_assert((count) == (expected_count), 									\
		#array" array count "#count" is not expected "#expected_count);			\
	for (k = 0; k < count; k++)													\
	{																			\
		printf("  %lu: \"%s\" -> \"%s\"\n",										\
			k+1, (char *)(array)[k], (char *)(array_expected)[k]);				\
		cargo_assert(!strcmp((char *)(array)[k], (char *)(array_expected)[k]),	\
			"Array contains unexpected value");									\
	}																			\
} while (0)

//
// Some helper macros for creating test functions.
//
#define _MAKE_TEST_FUNC_NAME(f) f()

// Start a test. Initializes the lib.
#define _TEST_START(testname) 							\
static char *_MAKE_TEST_FUNC_NAME(testname)				\
{														\
	char *msg = NULL;									\
	int ret = 0;										\
	cargo_t cargo;										\
														\
	if (cargo_init(&cargo, "program"))					\
	{													\
		return "Failed to init cargo";					\
	}

// Cleanup point, must be placed at the end of the test
//
// scope _TEST_START(name) { ... _TEST_CLEANUP(); free(something) } _TEST_END()
//
// cargo_assert jumps to this point on failure. Any cleanup should be
// placed after this call.
#define _TEST_CLEANUP()		\
	fail: (void)0

#define _TEST_END()			\
	cargo_destroy(&cargo); 	\
	return msg;				\
}

// =================================================================
// Test functions.
// =================================================================

_TEST_START(TEST_no_args_bool_option)
{
	int a;
	char *args[] = { "program", "--alpha" };
	int argc = sizeof(args) / sizeof(args[0]);

	ret = cargo_add_option(cargo, "--alpha", "Description", "b", &a);
	cargo_assert(ret == 0, "Failed to add valid bool option");

	if (cargo_parse(cargo, 1, argc, args))
	{
		msg = "Failed to parse bool with no argument";
		goto fail;
	}

	cargo_assert(a == 1, "Failed to parse bool with no argument to 1");

	_TEST_CLEANUP();
}
_TEST_END()

//
// Simple add option tests.
//
#define _TEST_ADD_SIMPLE_OPTION(name, type, value, fmt, ...) 				\
	_TEST_START(name)														\
	{																		\
		char *args[] = { "program", "--alpha", #value };					\
		type a;																\
		ret = cargo_add_option(cargo, "--alpha -a",							\
								"Description", 								\
								fmt,										\
								&a, ##__VA_ARGS__);							\
		cargo_assert(ret == 0, "Failed to add valid "#type" option");		\
		if (cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args))	\
		{																	\
			msg = "Failed to parse "#type" with value \""#value"\"";		\
			goto fail;														\
		}																	\
		printf("Attempt to parse value: "#value"\n");						\
		cargo_assert(a == value, "Failed to parse correct value "#value);	\
		_TEST_CLEANUP();													\
	}																		\
	_TEST_END()

_TEST_ADD_SIMPLE_OPTION(TEST_add_integer_option, int, -3, "i")
_TEST_ADD_SIMPLE_OPTION(TEST_add_uinteger_option, unsigned int, 3, "u")
_TEST_ADD_SIMPLE_OPTION(TEST_add_float_option, float, 0.3f, "f")
_TEST_ADD_SIMPLE_OPTION(TEST_add_bool_option, int, 1, "b")
_TEST_ADD_SIMPLE_OPTION(TEST_add_double_option, double, 0.4, "d")

_TEST_START(TEST_add_static_string_option)
{
	char b[10];
	char *args[] = { "program", "--beta", "abc" };
 	ret = cargo_add_option(cargo, "--beta -b",
							"Description", 
							".s#",
							&b, sizeof(b));
	cargo_assert(ret == 0, "Failed to add valid static string option");

	if (cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args))
	{
		msg = "Failed to parse static char * with value \"abc\"";
		goto fail;
	}
	printf("Attempt to parse value: abc\n");
	cargo_assert(!strcmp(b, "abc"), "Failed to parse correct value abc");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_alloc_string_option)
{
	char *b = NULL;
	char *args[] = { "program", "--beta", "abc" };
 	ret = cargo_add_option(cargo, "--beta -b",
							"Description", 
							"s",
							&b);
	cargo_assert(ret == 0, "Failed to add valid alloc string option");

	if (cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args))
	{
		msg = "Failed to parse alloc char * with value \"abc\"";
		goto fail;
	}
	printf("Attempt to parse value: abc\n");
	cargo_assert(b, "pointer is null");
	cargo_assert(!strcmp(b, "abc"), "Failed to parse correct value abc");

	_TEST_CLEANUP();
	free(b);
}
_TEST_END()

//
// =========================== Array Tests ====================================
//
#define _TEST_ARRAY_OPTION(array, array_size, args, argc, fmt, ...) 		 \
{																			 \
 	ret = cargo_add_option(cargo, "--beta -b", "Description",				 \
 							fmt, ##__VA_ARGS__);							 \
	cargo_assert(ret == 0,													 \
		"Failed to add "#array"["#array_size"] array option");				 \
	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);		 \
	cargo_assert(ret == 0, "Failed to parse array: "#array"["#array_size"]");\
}

#define ARG_SIZE (sizeof(args) / sizeof(args[0]))
#define ARRAY_SIZE (sizeof(a_expect) / sizeof(a_expect[0]))
#define _ADD_TEST_FIXED_ARRAY(fmt, printfmt)		 					\
do 																		\
{																		\
	size_t count;														\
	size_t i;															\
	_TEST_ARRAY_OPTION(a, ARRAY_SIZE, args, ARG_SIZE,					\
		fmt, &a, &count, ARRAY_SIZE);									\
	cargo_assert(a != NULL, "Array is null");							\
	cargo_assert(count == ARRAY_SIZE, "Array count is invalid");		\
	printf("Read %lu values from int array:\n", count);					\
	for (i = 0; i < ARRAY_SIZE; i++) printf("  "#printfmt"\n", a[i]);	\
	cargo_assert_array(count, ARRAY_SIZE, a, a_expect);					\
} while (0)

///
/// Simple add array tests.
///
_TEST_START(TEST_add_static_int_array_option)
{
	int a[3];
	int a_expect[3] = { 1, -2, 3 };
	char *args[] = { "program", "--beta", "1", "-2", "3" };
	_ADD_TEST_FIXED_ARRAY(".[i]#", "%d");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_static_uint_array_option)
{
	unsigned int a[3];
	unsigned int a_expect[3] = { 1, 2, 3 };
	char *args[] = { "program", "--beta", "1", "2", "3" };
	_ADD_TEST_FIXED_ARRAY(".[u]#", "%u");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_static_bool_array_option)
{
	// TODO: Hmmmm don't support this, bools should be for flags only.
	int a[3];
	int a_expect[3] = { 1, 1, 1 };
	char *args[] = { "program", "--beta", "1", "2", "3" };
	_ADD_TEST_FIXED_ARRAY(".[b]#", "%d");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_static_float_array_option)
{
	float a[3];
	float a_expect[3] = { 0.1f, 0.2f, 0.3f };
	char *args[] = { "program", "--beta", "0.1", "0.2", "0.3" };
	_ADD_TEST_FIXED_ARRAY(".[f]#", "%f");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_static_double_array_option)
{
	double a[3];
	double a_expect[3] = { 0.1, 0.2, 0.3 };
	char *args[] = { "program", "--beta", "0.1", "0.2", "0.3" };
	_ADD_TEST_FIXED_ARRAY(".[d]#", "%f");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_static_string_array_option)
{
	#define LENSTR 5
	char a[3][LENSTR];
	size_t count;
	char *args[] = { "program", "--beta", "abc", "def", "ghi" };
	#define ARRAY2_SIZE (sizeof(a) / sizeof(a[0]))
	#define ARG_SIZE (sizeof(args) / sizeof(args[0]))

	_TEST_ARRAY_OPTION(a, ARRAY2_SIZE, args, ARG_SIZE, 
						".[s#]#", &a, LENSTR, &count, ARRAY2_SIZE);

	printf("Read %lu values from int array: %s, %s, %s\n",
			count, a[0], a[1], a[2]);
	cargo_assert(count == ARRAY2_SIZE, "Array count is not 3 as expected");
	cargo_assert(!strcmp(a[0], "abc"), "Array value at index 0 is not \"abc\" as expected");
	cargo_assert(!strcmp(a[1], "def"), "Array value at index 1 is not \"def\" as expected");
	cargo_assert(!strcmp(a[2], "ghi"), "Array value at index 2 is not \"ghi\" as expected");
	_TEST_CLEANUP();
}
_TEST_END()

///
/// Alloc fixed size array tests.
///
_TEST_START(TEST_add_alloc_fixed_int_array_option)
{
	int *a = NULL;
	int a_expect[3] = { 1, -2, 3 };
	char *args[] = { "program", "--beta", "1", "-2", "3" };
	_ADD_TEST_FIXED_ARRAY("[i]#", "%d");
	_TEST_CLEANUP();
	free(a);
}
_TEST_END()

_TEST_START(TEST_add_alloc_fixed_uint_array_option)
{
	unsigned int *a = NULL;
	unsigned int a_expect[3] = { 1, 2, 3 };
	char *args[] = { "program", "--beta", "1", "2", "3" };
	_ADD_TEST_FIXED_ARRAY("[u]#", "%u");
	_TEST_CLEANUP();
	free(a);
}
_TEST_END()

_TEST_START(TEST_add_alloc_fixed_float_array_option)
{
	float *a = NULL;
	float a_expect[3] = { 1.1f, -2.2f, 3.3f };
	char *args[] = { "program", "--beta", "1.1", "-2.2", "3.3" };
	_ADD_TEST_FIXED_ARRAY("[f]#", "%f");
	_TEST_CLEANUP();
	free(a);
}
_TEST_END()

_TEST_START(TEST_add_alloc_fixed_double_array_option)
{
	double *a = NULL;
	double a_expect[3] = { 1.1, -2.2, 3.3 };
	char *args[] = { "program", "--beta", "1.1", "-2.2", "3.3" };
	_ADD_TEST_FIXED_ARRAY("[d]#", "%f");
	_TEST_CLEANUP();
	free(a);
}
_TEST_END()

_TEST_START(TEST_add_alloc_fixed_string_array_option)
{
	#define LENSTR 5
	char **a = NULL;
	size_t count;
	char *args[] = { "program", "--beta", "abc", "def", "ghi" };
	#define ARG_SIZE (sizeof(args) / sizeof(args[0]))

	_TEST_ARRAY_OPTION(a, 3, args, ARG_SIZE, 
						"[s#]#", &a, LENSTR, &count, 3);

	cargo_assert(a != NULL, "Array is null");
	cargo_assert(count == 3, "Array count is not 3");
	printf("Read %lu values from int array: %s, %s, %s\n",
			count, a[0], a[1], a[2]);
	cargo_assert(count == 3, "Array count is not 3 as expected");
	cargo_assert(!strcmp(a[0], "abc"), "Array value at index 0 is not \"abc\" as expected");
	cargo_assert(!strcmp(a[1], "def"), "Array value at index 1 is not \"def\" as expected");
	cargo_assert(!strcmp(a[2], "ghi"), "Array value at index 2 is not \"ghi\" as expected");
	_TEST_CLEANUP();
	_cargo_free_str_list(&a, &count);
}
_TEST_END()

//
// Dynamic sized alloc array tests.
//
_TEST_START(TEST_add_alloc_dynamic_int_array_option)
{
	int *a = NULL;
	int a_expect[3] = { 1, -2, 3 };
	char *args[] = { "program", "--beta", "1", "-2", "3" };
	_ADD_TEST_FIXED_ARRAY("[i]+", "%d");
	_TEST_CLEANUP();
	free(a);
}
_TEST_END()

_TEST_START(TEST_add_alloc_dynamic_int_array_option_noargs)
{
	int *a = NULL;
	char *args[] = { "program", "--beta" };
	size_t count;

	ret = cargo_add_option(cargo, "--beta -b", "Description", "[i]+",
							&a, &count);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Successfully parsed when no args");

	_TEST_CLEANUP();
}
_TEST_END()

//
// Misc output tests.
//
_TEST_START(TEST_print_usage)
{
	int a[3];
	size_t a_count = 0;
	float b;
	double c;
	char *s = NULL;
	int *vals = NULL;
	size_t val_count = 0;

 	ret |= cargo_add_option(cargo, "--alpha -a",
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
			"eiusmod tempor incididunt ut labore et dolore magna aliqua. "
			"Ut enim ad minim veniam, quis nostrud exercitation ullamco "
			"laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure "
			"dolor in reprehenderit in voluptate velit esse cillum dolore eu "
			"fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
			"proident, sunt in culpa qui officia deserunt mollit anim id est "
			"laborum", 
			".[i]#",
			&a, &a_count, sizeof(a) / sizeof(a[0]));

 	ret |= cargo_add_option(cargo, "--beta -b",
			"Shorter description", 
			"f",
			&b);
	ret |= cargo_set_metavar(cargo, "--beta", "FLOAT");

 	ret |= cargo_add_option(cargo, "--call_this_a_long_option_that_wont_fit -c",
			"Sed ut perspiciatis unde omnis iste natus error sit voluptatem "
			"accusantium doloremque laudantium, totam rem aperiam, eaque ipsa "
			"quae ab illo inventore veritatis et quasi architecto beatae vitae "
			"dicta sunt explicabo", 
			"d",
			&c);

 	ret |= cargo_add_option(cargo, "--shorter -s",
			"Sed ut perspiciatis unde omnis iste natus error sit voluptatem "
			"accusantium doloremque laudantium, totam rem aperiam, eaque ipsa "
			"quae ab illo inventore veritatis et quasi architecto beatae vitae "
			"dicta sunt explicabo", 
			"s",
			&s);

 	ret |= cargo_add_option(cargo, "--vals -v",
			"Shorter description",
			"[ i ]+",
			&vals, &val_count);
	cargo_assert(ret == 0, "Failed to add options");

	cargo_set_epilog(cargo, "That's it!");
	cargo_set_description(cargo, "Introductionary description");

	cargo_print_usage(cargo);
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_usage_settings)
{
	typedef struct _test_usage_settings_s
	{
		cargo_format_t fmt;
		char *expect[4];
		size_t expect_count;
	} _test_usage_settings_t;

	#define DESCRIPT "Brown fox"
	#define EPILOG "Lazy fox"
	#define OPT_TXT "The alpha"
	size_t j;
	size_t k;
	int i;
	char *usage = NULL;
	_test_usage_settings_t tus[] =
	{
		{ 0,								{ DESCRIPT, EPILOG, OPT_TXT }, 3},
		{ CARGO_FORMAT_HIDE_EPILOG,			{ DESCRIPT, OPT_TXT }, 2},
		{ CARGO_FORMAT_HIDE_DESCRIPTION,	{ EPILOG, OPT_TXT }, 2}
	};

	ret |= cargo_add_option(cargo, "--alpha -a", OPT_TXT, "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	cargo_set_description(cargo, "Brown fox");
	cargo_set_epilog(cargo, "Lazy fox");

	for (j = 0; j < sizeof(tus) / sizeof(tus[0]); j++)
	{
		cargo_set_format(cargo, tus[j].fmt);
		usage = cargo_get_usage(cargo, NULL, NULL);
		cargo_assert(usage != NULL, "Got null usage");
		printf("\n\n");

		for (k = 0; k < tus[j].expect_count; k++)
		{
			char *s = tus[j].expect[k];
			char *found = strstr(usage, s);
			printf("Expecting to find in usage: \"%s\"\n", s);
			cargo_assert(found != NULL,
				"Usage formatting unexpected");
		}

		printf("-------------------------------------\n");
		printf("%s\n", usage);
		printf("-------------------------------------\n");

		free(usage);
		usage = NULL;
	}

	_TEST_CLEANUP();
	if (usage) free(usage);
}
_TEST_END()

_TEST_START(TEST_autohelp_default)
{
	int i;
	char *usage = NULL;

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);

	// Default is to automatically add --help option.
	usage = cargo_get_usage(cargo, NULL, NULL);
	cargo_assert(usage != NULL, "Got NULL usage");
	printf("-------------------------------------\n");
	printf("%s", usage);
	printf("-------------------------------------\n");
	cargo_assert(strstr(usage, "help") != NULL, "No help found by default");
	free(usage);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_autohelp_off)
{
	int i;
	char *usage = NULL;

	// Turn off auto_help (--help).
	cargo_set_auto_help(cargo, 0);
	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);

	usage = cargo_get_usage(cargo, NULL, NULL);
	cargo_assert(usage != NULL, "Got NULL usage");
	printf("-------------------------------------\n");
	printf("%s", usage);
	printf("-------------------------------------\n");
	cargo_assert(strstr(usage, "help") == NULL,
				"Help found when auto_help turned off");
	free(usage);

	_TEST_CLEANUP();
}
_TEST_END()

#define _ADD_TEST_USAGE_OPTIONS() 										\
do 																		\
{																		\
	int i;																\
	float f;															\
	int b;																\
	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);	\
	ret |= cargo_add_option(cargo, "--beta", "The alpha", "f", &f);		\
	ret |= cargo_add_option(cargo, "--crash -c", "The alpha", "b", &b);	\
	cargo_assert(ret == 0, "Failed to add options");					\
} while (0)

_TEST_START(TEST_get_usage)
{
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo, NULL, NULL);
	cargo_assert(usage != NULL, "Failed to get allocated usage");
	printf("%s\n", usage);

	_TEST_CLEANUP();
	free(usage);
}
_TEST_END()

_TEST_START(TEST_get_usage_missing_arg)
{
	char buf[1024];
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo, buf, NULL);
	cargo_assert(usage == NULL, "Expected NULL usage on missing buf_size arg");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_usage_size_only)
{
	char buf[1024];
	size_t buf_size = 1024;
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo, NULL, &buf_size);
	cargo_assert(usage == NULL, "Expected NULL usage on buf_size only");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_usage_fixed)
{
	char buf[1024];
	size_t buf_size = 1024;
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo, buf, &buf_size);
	cargo_assert(usage != NULL, "Buffer size large enough but got NULL");
	cargo_assert(usage == buf, "Did not get buf pointer back from call");
	printf("%s\n", usage);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_usage_fixed_too_small)
{
	char buf[4];
	size_t buf_size = 4;
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo, buf, &buf_size);
	cargo_assert(usage == NULL, "Buffer size too small but got non-null buf");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_misspelled_argument)
{
	int i;
	float f;
	int b;
	char *args[] = { "program", "--bota", "0.1" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	ret |= cargo_add_option(cargo, "--beta", "The alpha", "f", &f);
	ret |= cargo_add_option(cargo, "--crash -c", "The alpha", "b", &b);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, 3, args);
	cargo_assert(ret == -1, "Got valid parse with invalid input");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_max_option_count)
{
	int i;
	float f;
	int b;

	cargo_set_option_count_hint(cargo, 1);

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	ret |= cargo_add_option(cargo, "--beta", "The alpha", "f", &f);
	ret |= cargo_add_option(cargo, "--crash -c", "The alpha", "b", &b);

	cargo_assert(ret == 0, "Failed to add options");
	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_add_duplicate_option)
{
	int i;

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");
	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	cargo_assert(ret != 0, "Succesfully added duplicated, not allowed");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_extra_args)
{
	size_t argc = 0;
	char **extra_args = NULL;
	char *extra_args_expect[] = { "abc", "def", "ghi" };
	int i;
	char *args[] = { "program", "-a", "1", "abc", "def", "ghi" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed to parse extra args input");

	// Get left over arguments.
	extra_args = cargo_get_args(cargo, &argc);
	cargo_assert(extra_args != NULL, "Got NULL extra args");

	printf("argc = %lu\n", argc);
	cargo_assert(argc == 3, "Expected argc == 3");
	cargo_assert_str_array(argc, 3, extra_args, extra_args_expect);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_get_unknown_opts)
{
	size_t unknown_count = 0;
	char **unknown_opts = NULL;
	char *unknown_opts_expect[] = { "-b", "-c" };
	int i;
	char *args[] = { "program", "-a", "1", "-b", "-c", "3" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succeeded parsing with unknown options");

	// Get left over arguments.
	unknown_opts = cargo_get_unknown(cargo, &unknown_count);
	cargo_assert(unknown_opts != NULL, "Got NULL unknown options");

	printf("Unknown option count = %lu\n", unknown_count);
	cargo_assert(unknown_count == 2, "Unknown option count == 2");
	cargo_assert_str_array(unknown_count, 2, unknown_opts, unknown_opts_expect);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_cargo_split)
{
	size_t i;
	size_t j;
	char **ss = NULL;
	
	char *in[] =
	{
		"abc def ghi",
		"abc",
		NULL
	};
	#define NUM (sizeof(in) / sizeof(in[0]))
	
	char *expect0[] = { "abc", "def", "ghi" };
	char *expect1[] = { "abc" };
	char *expect2[] = { NULL };
	char **expect[] = { expect0, expect1, expect2 };
	size_t expect_count[] =
	{
		3,
		1,
		0
	};

	char **out[NUM];
	size_t out_count[NUM];

	for (i = 0; i < NUM; i++)
	{
		printf("Split: \"%s\"", in[i]);
		out[i] = _cargo_split(in[i], " ", &out_count[i]);
		printf(" into %lu substrings\n", out_count[i]);

		if (in[i] != NULL)
			cargo_assert(out[i] != NULL, "Got null split result");

		for (j = 0; j < out_count[i]; j++)
		{
			printf("\"%s\"%s ", 
				out[i][j], ((j + 1) != out_count[i]) ? "," : "");
		}

		printf("\n");
	}

	for (i = 0; i < NUM; i++)
	{
		cargo_assert_str_array(out_count[i], expect_count[i], out[i], expect[i]);
	}

	_TEST_CLEANUP();
	for (i = 0; i < NUM; i++)
	{
		_cargo_free_str_list(&out[i], &out_count[i]);
	}
	#undef NUM
}
_TEST_END()

_TEST_START(TEST_parse_invalid_value)
{
	int i;
	int j;
	char *args[] = { "program", "--alpha", "1", "--beta", "a" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	ret = cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succesfully parsed invalid value");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_advanced_parse)
{
	size_t i = 0;
	int flag = 0;
	#define PORTS_COUNT 3
	int ports[PORTS_COUNT];
	size_t ports_count = 0;
	char *name = NULL;
	char **vals = NULL;
	size_t vals_count = 0;

	char *args1[] =
	{
		"program",
		"--ports", "22", "24", "26",
		"--vals", "abc", "def", "123456789101112", "ghi", "jklmnopq",
		"--name", "server"
	};
	int args1_ports_expect[] = { 22, 24, 26 };
	char *args1_vals_expect[] = { "abc", "def", "123456789101112", "ghi", "jklmnopq" };

	char *args2[] =
	{
		"program",
		"--vals", "abc", "def", "123456789101112", "ghi", "jklmnopq",
		"--ports", "33", "34", "36",
		"--name", "server"
	};
	int args2_ports_expect[] = { 33, 34, 36 };

	ret |= cargo_add_option(cargo, "--ports -p", "Ports", ".[i]#",
							&ports, &ports_count, PORTS_COUNT);
	ret |= cargo_add_option(cargo, "--name -n", "Name", "s", &name);
	ret |= cargo_add_option(cargo, "--vals -v", "Description of vals", "[s]+",
							&vals, &vals_count);
	cargo_assert(ret == 0, "Failed to add options");

	printf("\nArgs 1:\n");
	{
		ret = cargo_parse(cargo, 1, sizeof(args1) / sizeof(args1[0]), args1);

		cargo_assert(ret == 0, "Failed to parse advanced example");
		cargo_assert_array(ports_count, PORTS_COUNT, ports, args1_ports_expect);
		cargo_assert_str_array(vals_count, 5, vals, args1_vals_expect);
		cargo_assert(!strcmp(name, "server"), "Expected name = \"server\"");

		_cargo_free_str_list(&vals, &vals_count);
		if (name) free(name);
		name = NULL;
		memset(&ports, 0, sizeof(ports));
	}

	printf("\nArgs 2:\n");
	{
		ret = cargo_parse(cargo, 1, sizeof(args2) / sizeof(args2[0]), args2);

		cargo_assert(ret == 0, "Failed to parse advanced example");
		cargo_assert_str_array(vals_count, 5, vals, args1_vals_expect);
		cargo_assert_array(ports_count, PORTS_COUNT, ports, args2_ports_expect);
		cargo_assert(!strcmp(name, "server"), "Expected name = \"server\"");
	}

	_TEST_CLEANUP();
	_cargo_free_str_list(&vals, &vals_count);
	if (name) free(name);
}
_TEST_END()

//
// List of all test functions to run:
//
typedef char *(* cargo_test_f)();

typedef struct cargo_test_s
{
	const char *name;
	cargo_test_f f;
	int success;
	int ran;
	char *error;
} cargo_test_t;

#define CARGO_ADD_TEST(test) { #test, test, 0, 0, NULL }

cargo_test_t tests[] =
{
	CARGO_ADD_TEST(TEST_no_args_bool_option),
	CARGO_ADD_TEST(TEST_add_integer_option),
	CARGO_ADD_TEST(TEST_add_uinteger_option),
	CARGO_ADD_TEST(TEST_add_float_option),
	CARGO_ADD_TEST(TEST_add_bool_option),
	CARGO_ADD_TEST(TEST_add_double_option),
	CARGO_ADD_TEST(TEST_add_static_string_option),
	CARGO_ADD_TEST(TEST_add_alloc_string_option),
	CARGO_ADD_TEST(TEST_add_static_int_array_option),
	CARGO_ADD_TEST(TEST_add_static_uint_array_option),
	CARGO_ADD_TEST(TEST_add_static_bool_array_option),
	CARGO_ADD_TEST(TEST_add_static_float_array_option),
	CARGO_ADD_TEST(TEST_add_static_double_array_option),
	CARGO_ADD_TEST(TEST_add_static_string_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_fixed_int_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_fixed_uint_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_fixed_float_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_fixed_double_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_fixed_string_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_dynamic_int_array_option),
	CARGO_ADD_TEST(TEST_add_alloc_dynamic_int_array_option_noargs),
	CARGO_ADD_TEST(TEST_print_usage),
	CARGO_ADD_TEST(TEST_get_usage_settings),
	CARGO_ADD_TEST(TEST_autohelp_default),
	CARGO_ADD_TEST(TEST_autohelp_off),
	CARGO_ADD_TEST(TEST_get_usage),
	CARGO_ADD_TEST(TEST_get_usage_missing_arg),
	CARGO_ADD_TEST(TEST_get_usage_size_only),
	CARGO_ADD_TEST(TEST_get_usage_fixed),
	CARGO_ADD_TEST(TEST_get_usage_fixed_too_small),
	CARGO_ADD_TEST(TEST_misspelled_argument),
	CARGO_ADD_TEST(TEST_max_option_count),
	CARGO_ADD_TEST(TEST_add_duplicate_option),
	CARGO_ADD_TEST(TEST_get_extra_args),
	CARGO_ADD_TEST(TEST_get_unknown_opts),
	CARGO_ADD_TEST(TEST_cargo_split),
	CARGO_ADD_TEST(TEST_parse_invalid_value),
	CARGO_ADD_TEST(TEST_advanced_parse)
};

#define CARGO_NUM_TESTS (sizeof(tests) / sizeof(tests[0]))

static void _test_print_names()
{
	int i;

	for (i = 0; i < (int)CARGO_NUM_TESTS; i++)
	{
		printf("%2d: %s\n", (i + 1), tests[i].name);
	}
}

static int _test_find_test_index(const char *name)
{
	int i;

	for (i = 0; i < (int)CARGO_NUM_TESTS; i++)
	{
		if (!strcmp(name, tests[i].name))
		{
			return i + 1;
		}
	}

	return -1;
}

int main(int argc, char **argv)
{
	size_t i;
	int was_error = 0;
	int testnum = 0;
	char *res = NULL;
	int tests_to_run[CARGO_NUM_TESTS];
	size_t num_tests = 0;
	size_t success_count = 0;
	int all = 0;
	int test_index = 0;
	cargo_test_t *t;

	memset(tests_to_run, 0, sizeof(tests_to_run));

	// Get test numbers to run from command line.
	if (argc >= 2)
	{
		if (!strcmp("--list", argv[1]))
		{
			_test_print_names();
			return 0;
		}

		for (i = 1; i < (size_t)argc; i++)
		{
			// First check if we were given a function name.
			if (!strncmp(argv[i], "TEST_", 5))
			{
				test_index = _test_find_test_index(argv[i]);

				if (test_index <= 0)
				{
					fprintf(stderr, "Unknown test specified: \"%s\"\n", argv[i]);
					return -1;
				}
			}
			else
			{
				test_index = atoi(argv[i]);

				if (test_index == 0)
				{
					fprintf(stderr, "Invalid test number %s\n", argv[i]);
					return -1;
				}
				else if (test_index < 0)
				{
					printf("Run ALL tests!\n");
					all = 1;
					break;
				}
			}

			tests_to_run[num_tests] = test_index;
			num_tests++;
		}
	}
	else
	{
		_test_print_names();
		fprintf(stderr, "\nUsage: %s [test_num ...] [test_name ...]\n\n", argv[0]);
		fprintf(stderr, "  --list to get all available tests.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "  Use test_num = -1 or all tests\n");
		fprintf(stderr, "  Or you can specify the testname: TEST_...\n");
		fprintf(stderr, "  Return code for this usage message equals the number of available tests.\n");
		return CARGO_NUM_TESTS;
	}

	// Run all tests.
	if (all)
	{
		num_tests = CARGO_NUM_TESTS;

		for (i = 0; i < num_tests; i++)
		{
			tests_to_run[i] = i + 1;
		}
	}

	// Run the tests.
	for (i = 0; i < num_tests; i++)
	{
		test_index = tests_to_run[i] - 1;
		t = &tests[test_index];
		t->ran = 1;

		fprintf(stderr, "\n%sStart Test %2d:%s\n",
			ANSI_COLOR_CYAN, test_index + 1, ANSI_COLOR_RESET);

		fprintf(stderr, "%s", ANSI_COLOR_DARK_GRAY);
		t->error = t->f();
		fprintf(stderr, "%s", ANSI_COLOR_RESET);

		fprintf(stderr, "%sEnd Test %2d:%s ",
			ANSI_COLOR_CYAN, test_index + 1, ANSI_COLOR_RESET);

		if (t->error)
		{
			fprintf(stderr, "[%sFAIL%s] %s\n",
				ANSI_COLOR_RED, ANSI_COLOR_RESET, t->error);
			was_error++;
		}
		else
		{
			fprintf(stderr, "[%sSUCCESS%s]\n",
				ANSI_COLOR_GREEN, ANSI_COLOR_RESET);

			success_count++;
		}

		tests[test_index].success = (t->error == NULL);
	}

	printf("---------------------------------------------------------------\n");
	printf("Test report:\n");
	printf("---------------------------------------------------------------\n");

	for (i = 0; i < CARGO_NUM_TESTS; i++)
	{
		if (!tests[i].ran)
		{
			printf(" [%sNOT RUN%s] %2lu: %s\n",
				ANSI_COLOR_DARK_GRAY, ANSI_COLOR_RESET, (i + 1), tests[i].name);
			continue;
		}

		if (tests[i].success)
		{
			printf(" [%sSUCCESS%s] %2lu: %s\n",
				ANSI_COLOR_GREEN, ANSI_COLOR_RESET, (i + 1), tests[i].name);
		}
		else
		{
			fprintf(stderr, " [%sFAILED%s]  %2lu: %s - %s\n",
				ANSI_COLOR_RED, ANSI_COLOR_RESET,
				(i + 1), tests[i].name, tests[i].error);
		}
	}

	if (was_error)
	{
		fprintf(stderr, "\n[[%sFAIL%s]] ", ANSI_COLOR_RED, ANSI_COLOR_RESET);
	}
	else
	{
		printf("\n[[%sSUCCESS%s]] ", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
	}

	printf("%lu of %lu tests run were successful (%lu of %lu tests ran)\n",
			success_count, num_tests, num_tests, CARGO_NUM_TESTS);

	return (num_tests - success_count);
}

#elif defined(CARGO_HELPER)

int main(int argc, char **argv)
{
	int ret = 0;
	char *fmt = NULL;
	char *end = NULL;
	char arrsize[1024];
	char lenstr[1024];
	char varname[1024];
	cargo_type_t types[3];
	size_t type_count = 0;
	int array = 0;
	int alloc = 0;
	int static_str = 0;
	size_t example_count = 1;
	size_t i;
	size_t j;

	if (argc < 2)
	{
		fprintf(stderr, "%s: <variable declaration>\n", argv[0]);
		return -1;
	}

	fmt = argv[1];

	printf("%s;\n", fmt);

	while (isspace(*fmt)) fmt++;

	// Get type.
	if (!strncmp(fmt, "int", 3))
	{
		types[type_count++] = CARGO_INT;
		types[type_count++] = CARGO_BOOL;
		fmt += 3;
	}
	else if (!strncmp(fmt, "char", 4))
	{
		types[type_count++] = CARGO_STRING;
		fmt += 4;
	}
	else if (!strncmp(fmt, "float", 5))
	{
		types[type_count++] = CARGO_FLOAT;
		fmt += 5;
	}
	else if (!strncmp(fmt, "double", 6))
	{
		types[type_count++] = CARGO_DOUBLE;
		fmt += 6;
	}
	else if (!strncmp(fmt, "unsigned int", 12))
	{
		types[type_count++] = CARGO_UINT;
		fmt += 12;
	}

	while (isspace(*fmt)) fmt++;

	// Set array and alloc status.
	if (types[0] == CARGO_STRING)
	{
		if (!strncmp(fmt, "**", 2))
		{
			alloc = 1;
			array = 1;
			fmt += 2;
		}
		else if (*fmt == '*')
		{
			alloc = 1;
			fmt++;
		}
	}
	else
	{
		if (*fmt == '*')
		{
			alloc = 1;
			array = 1;
			// We can't have CARGO_BOOL arrays.
			if (types[0] == CARGO_INT)
				type_count--;

			fmt++;
		}
	}

	// Get var name.
	if ((end = strchr(fmt, '[')) != NULL)
	{
		cargo_snprintf(varname, sizeof(varname), "%.*s", (end - fmt), fmt);
	}
	else
	{
		cargo_snprintf(varname, sizeof(varname), "%s", fmt);
	}

	// Get variable name.
	if ((end = strchr(fmt, '[')) != NULL)
	{
		char *start = end + 1;
		array = 1;

		// We can't have CARGO_BOOL arrays.
		if (types[0] == CARGO_INT)
			type_count--;

		if ((end = strchr(fmt, ']')) == NULL)
		{
			fprintf(stderr, "Missing ']'\n");
			ret = -1; goto fail;
		}

		cargo_snprintf(arrsize, sizeof(arrsize), "%.*s", (end - start), start);

		if (types[0] == CARGO_STRING)
		{
			if (alloc)
			{
				fprintf(stderr, "You cannot use string arrays of this format\n");
				ret = -1; goto fail;
			}

			if ((end = strchr(start, '[')) != NULL)
			{
				// We have an array of strings.
				// char bla[COUNT][LENSTR];
				start = end + 1;

				if ((end = strchr(start, ']')) == NULL)
				{
					fprintf(stderr, "Missing ']'\n");
					ret = -1; goto fail;
				}

				static_str = 1;
				cargo_snprintf(lenstr, sizeof(lenstr), "%.*s", (end - start), start);
			}
			else
			{
				// This is not an array, simply a static string.
				// char bla[123];
				array = 0;
				static_str = 1;
				cargo_snprintf(lenstr, sizeof(lenstr), "%s", arrsize);
			}
		}

		if (strlen(arrsize) == 0)
		{
			cargo_snprintf(arrsize, sizeof(arrsize), "sizeof(%s) / sizeof(%s[0])",
							varname, varname);
		}
	}

	if ((types[0] == CARGO_STRING) && !array && !alloc && !static_str)
	{
		fprintf(stderr, "\"char\" is not a valid variable type by itself, did you mean \"char *\"?\n");
		return -1;
	}

	if (alloc) example_count++;
	if (array) printf("size_t %s_count;\n", varname);

	// TODO: Create extra examples for [s#]#
	//if ((types[0] == CARGO_STRING)) example_count += 2;

	#define IS_EXTRA_EXAMPLE(num) ((j % 2) == 0)

	for (j = 0; j < example_count; j++)
	{
		for (i = 0; i < type_count; i++)
		{
			printf("cargo_add_option(cargo, \"--%s", varname);
			if (strlen(varname) > 1) printf(" -%c", varname[0]);
			printf("\", \"Description of %s\", \"", varname);

			if (!alloc)
			{
				if (static_str || array)
					printf(".");
			}

			if (array) printf("[");

			printf("%c", _cargo_type_map[types[i]][0]);

			if (array && static_str) printf("#");
			if (array) printf("]");

			if (array && IS_EXTRA_EXAMPLE(1))
			{
				printf("#");
			}
			else
			{
				if (array && alloc) printf("+");
			}

			printf("\", &%s", varname);

			if (static_str) printf(", %s", lenstr);

			if (array)
			{
				printf(", &%s_count", varname);

				if (!alloc)
				{
					printf(", %s", arrsize);
				}
			}

			if (array && alloc && IS_EXTRA_EXAMPLE(1))
			{
				printf(", 128");
			}

			printf(");");

			if (array && alloc && IS_EXTRA_EXAMPLE(1))
			{
				printf(" // Allocated with max length 128.");
			}
			else if (array && alloc)
			{
				printf(" // Allocated unlimited length.");
			}

			printf("\n");
		}
	}

fail:
	return ret;
}

#elif defined(CARGO_EXAMPLE)

typedef struct args_s
{
	int a;
} args_t;

int main(int argc, char **argv)
{
	size_t i;
	int ret = 0;
	cargo_t cargo;
	args_t args;
	char **extra_args;
	size_t extra_count;

	cargo_init(&cargo, argv[0]);

	// TODO: Make a real example.

	cargo_print_usage(cargo);
done:
fail:
	cargo_destroy(&cargo);
	return ret;
}

#endif // CARGO_EXAMPLE

// LCOV_EXCL_END
