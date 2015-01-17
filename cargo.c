#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include "cargo.h"
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif // _WIN32

int cargo_get_console_width()
{
	#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		return -1;
	}

	CARGODBG(3, "Console width: %d\n", (int)(csbi.srWindow.Right - csbi.srWindow.Left));

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

int cargo_snprintf(char *buf, size_t buflen, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = cargo_vsnprintf(buf, buflen, format, ap);
	va_end(ap);
	return r;
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
	const char *metavar;
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
	const char *epilog;
	cargo_usage_settings_t usage;
	cargo_format_t format;

	int i;
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
	// should parse. 
	if (nargs >= 0)
	{
		o->max_target_count = nargs;
	}
	else if (target_count)
	{
		// But when allocating the space internally
		// and nargs is set to CARGO_NARGS_ONE_OR_MORE the max allowed
		// value is specified by the value in "target_count", or if that 
		// is 0 the size_t max value is used.
		if (*target_count == 0)
			o->max_target_count = (size_t)-1;
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
		*(o->target_count) = 0;
	}

	CARGODBG(1, " cargo_add %s, max_target_count = %lu\n",
				opt, o->max_target_count);

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
				assert(alloc_count >= 0);

				// Don't allocate more than necessary.
				if (opt->max_target_count < (size_t)alloc_count)
					alloc_count = opt->max_target_count;
			}

			if (!(new_target = (void **)calloc(alloc_count,
						_cargo_get_type_size(opt->type))))
			{
				fprintf(stderr, "Out of memory!\n");
				return -1;
			}

			CARGODBG(1, "Allocated %dx %s!\n",
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
			((float *)target)[opt->target_idx] = (float)atof(val);
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

static int _cargo_parse_option(cargo_t ctx, cargo_opt_t *opt, const char *name,
								int argc, char **argv)
{
	int start = ctx->i + 1;
	int opt_arg_count = 0;

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

		CARGODBG(1, "  Looking for %d args\n", args_to_look_for);

		// Look for arguments for this option.
		if ((start + args_to_look_for) > argc)
		{
			int expected = (opt->nargs!=CARGO_NARGS_ONE_OR_MORE) ? opt->nargs:1;
			fprintf(stderr, "Not enough arguments for %s."
							" %d expected but got only %d\n", 
							name, expected, argc - start);
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
	size_t i;
	size_t j;
	cargo_opt_t *opt;

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

static int _cargo_compare_strlen(const void *a, const void *b)
{
	return strlen(*((const char **)a)) - strlen(*((const char **)b));
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
			fprintf(stderr, "Out of memory\n");
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
	if ((opt->nargs != 0) && (opt->nargs != CARGO_NARGS_NONE_OR_MORE))
	{
		char metavarbuf[20];
		const char *metavar = NULL;

		if (opt->metavar)
		{
			metavar = opt->metavar;
		}
		else
		{
			// Got no user supplied metavar, simply use the
			// option name in upper case instead.
			int j = 0;
			i = 0;

			while (_cargo_is_prefix(ctx, opt->name[0][i]))
			{
				i++;
			}

			while (opt->name[0][i] && (j < (sizeof(metavarbuf)-1)))
			{
				metavarbuf[j++] = toupper(opt->name[0][i++]);
			}

			metavarbuf[j] = '\0';
			metavar = metavarbuf;
		}

		if (opt->nargs < 0)
		{
			// List the number of arguments.
			if (cargo_appendf(&str, " [%s ...]", metavar) < 0) goto fail;
		}
		else if (opt->nargs > 0)
		{
			if (cargo_appendf(&str, " [%s", metavar) < 0) goto fail;

			for (i = 1; i < opt->nargs; i++)
			{
				if (cargo_appendf(&str, " %s", metavar) < 0) goto fail;
			}

			if (cargo_appendf(&str, "]") < 0) goto fail;
		}
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

static char **_cargo_split(cargo_t ctx, char *s, char *splitchars, size_t *count)
{
	char **ss;
	int i = 0;
	char *p = s;
	size_t splitlen = strlen(splitchars);

	*count = 1;

	CARGODBG(3, "_cargo_split:\n\n");

	while (*p)
	{
		for (i = 0; i < splitlen; i++)
		{
			if (*p == splitchars[i])
				(*count)++;
		}
		p++;
	}

	if (!(ss = calloc(*count, sizeof(char *))))
		return NULL;

	p = strtok(s, splitchars);
	while (p)
	{
		ss[i] = p;
		CARGODBG(3, "%d: %s\n", i, ss[i]);

		p = strtok(NULL, "\n");
		i++;
	}

	return ss;
}

static char **_cargo_linebreak_to_list(cargo_t ctx, char *s, size_t *count)
{
	return _cargo_split(ctx, s, "\n", count);
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
		CARGODBG(4, "(p - start) = %d: %.*s\n",
			(p - start), (p - start), start);

		// Restart on already existing explicit line breaks.
		if (*p == '\n')
		{
			CARGODBG(3, "EXISTING NEW LINE len %d:\n%.*s\n\n",
				(p - start), (p - start), start);
			start = p;
		}
		else if ((size_t)(p - start) > width)
		{
			// We found a word that goes beyond the width we're
			// aiming for, so add the line break before that word.
			*prev = '\n';
			CARGODBG(3, "ADD NEW LINE len %d:\n%.*s\n\n",
				(prev - start), (prev - start), start);
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
		if (cargo_add(ctx, "--help", &ctx->help, CARGO_BOOL, "Show this help."))
		{
			return;
		}

		if (_cargo_find_option_name(ctx, "-h", NULL, NULL))
		{
			cargo_add_alias(ctx, "--help", "-h");
		}
	}
}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

int cargo_init(cargo_t *ctx, size_t max_opts,
				const char *progname, const char *description)
{
	int console_width = -1;
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
	c->usage.max_width = CARGO_DEFAULT_MAX_WIDTH;

	if ((console_width = cargo_get_console_width()) > 0)
	{
		c->usage.max_width = console_width;
	}

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

		if ((*ctx)->unknown_opts)
		{
			free((*ctx)->unknown_opts);
			(*ctx)->unknown_opts = NULL;
			(*ctx)->unknown_opts_count = 0;
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
	ctx->epilog = epilog;
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
	return _cargo_add(ctx, opt, (void **)target, NULL, 0, (type != CARGO_BOOL),
						type, description, 0);
}

int cargo_add_str(cargo_t ctx,
				const char *opt,
				void *target,
				size_t lenstr,
				const char *description)
{
	return _cargo_add(ctx, opt, target, NULL, 0, 1,
						CARGO_STRING, description, 0);
}

int cargo_add_alloc(cargo_t ctx,
				const char *opt,
				void **target,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, target, NULL, 0, (type != CARGO_BOOL),
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
	return _cargo_add(ctx, opt, (void **)target, target_count, 0,
						nargs, type, description, 0);
}

int cargo_addv_str(cargo_t ctx, 
				const char *opt,
				void *target,
				size_t *target_count,
				size_t lenstr,
				int nargs,
				cargo_type_t type,
				const char *description)
{
	assert(ctx);
	return _cargo_add(ctx, opt, (void **)target, target_count, lenstr,
						nargs, CARGO_STRING, description, 0);
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
	return _cargo_add(ctx, opt, target, target_count, 0,
						nargs, type, description, 1);
}

int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv)
{
	int start;
	int opt_arg_count;
	char *arg;
	const char *name;
	cargo_opt_t *opt = NULL;

	ctx->argc = argc;
	ctx->argv = argv;

	_cargo_add_help_if_missing(ctx);

	if (ctx->args)
	{
		free(ctx->args);
		ctx->args = NULL;
	}

	ctx->arg_count = 0;

	if (ctx->unknown_opts)
	{
		free(ctx->unknown_opts);
		ctx->unknown_opts = NULL;
	}

	ctx->unknown_opts_count = 0;

	if (!(ctx->args = (char **)calloc(argc, sizeof(char *))))
	{
		return -1;
	}

	if (!(ctx->unknown_opts = (char **)calloc(argc, sizeof(char *))))
	{
		return -1;
	}

	for (ctx->i = start_index; ctx->i < argc; ctx->i++)
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
				CARGODBG(2, "\"%s\" ", argv[k]);
			}

			CARGODBG(2, "%s", "\n");
		}
		#endif // CARGO_DEBUG
	}

	if (ctx->unknown_opts_count > 0)
	{
		size_t i;
		// TODO: Don't print to stderr here, instead enable getting as a string.
		CARGODBG(2, "Unknown options count: %lu\n", ctx->unknown_opts_count);
		fprintf(stderr, "Unknown options:\n");

		for (i = 0; i < ctx->unknown_opts_count; i++)
		{
			fprintf(stderr, "%s\n", ctx->unknown_opts[i]);
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

int cargo_add_alias(cargo_t ctx, const char *name, const char *alias)
{
	int opt_i;
	int name_i;
	cargo_opt_t *opt;
	assert(ctx);

	if (_cargo_find_option_name(ctx, name, &opt_i, &name_i))
	{
		CARGODBG(1, "Failed alias %s to %s, not found.\n", name, alias);
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

int cargo_get_usage(cargo_t ctx, char **buf, size_t *buf_size)
{
	int ret = 0;
	size_t i;
	size_t j;
	char *b;
	char **namebufs = NULL;
	int usagelen = 0;
	int namelen;
	int max_name_len = 0;
	cargo_str_t str;
	assert(ctx);
	assert(buf);
	#define MAX_OPT_NAME_LEN 40
	#define NAME_PADDING 2

	// First get option names and their length.
	// We get the widest one so we know the column width to use
	// for the final result.
	//   --option_a         Some description.
	//   --longer_option_b  Another description...
	// ^-------------------^
	// What should the above width be.
	if (!(namebufs = calloc(ctx->opt_count, sizeof(char *))))
	{
		fprintf(stderr, "Out of memory!\n");
		return -1;
	}

	for (i = 0; i < ctx->opt_count; i++)
	{
		if (!(namebufs[i] = malloc(ctx->usage.max_width)))
		{
			ret = -1; goto fail;
		}

		namelen = _cargo_get_option_name_str(ctx, &ctx->options[i], 
											namebufs[i], ctx->usage.max_width);

		if (namelen < 0)
		{
			ret = -1; goto fail;
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

	if (ctx->description && !(ctx->usage.format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		usagelen += strlen(ctx->description);
	}

	if (ctx->epilog && !(ctx->usage.format & CARGO_FORMAT_HIDE_EPILOG))
	{
		usagelen += strlen(ctx->epilog);
	}

	// Add some extra to fit padding.
	// TODO: Let's be more strict here and use the exact padding instead.
	usagelen = (int)(usagelen * 2.5);

	// Make sure we where given a buffer that is large enough.
	if (buf_size)
	{
		if (*buf_size < usagelen)
		{
			ret = -1; goto fail;
		}

		*buf_size = usagelen;
	}
	else
	{
		// Allocate the final buffer.
		if (!(b = malloc(usagelen)))
		{
			fprintf(stderr, "Out of memory!\n");
			*buf = NULL;
			ret = -1; goto fail;
		}

		// Output to the buffer.
		*buf = b;
	}

	str.s = b;
	str.l = usagelen;
	str.offset = 0;

	if(ctx->description && !(ctx->usage.format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		if (cargo_appendf(&str, "%s\nn", ctx->description) < 0) goto fail;
	}

	// TODO: Replace cargo_snprintf(&b[pos], (usagelen - pos) .... 
	// with a cargo_appendf function instead.
	if (cargo_appendf(&str,  "Optional arguments:\n") < 0) goto fail;

	CARGODBG(2, "max_name_len = %d, ctx->usage.max_width = %d\n",
		max_name_len, ctx->usage.max_width);

	// Option names + descriptions.
	for (i = 0; i < ctx->opt_count; i++)
	{
		// Is the option name so long we need a new line before the description?
		int option_causes_newline = strlen(namebufs[i]) > max_name_len;

		// Print the option names.
		// "  --ducks [DUCKS ...]  "
		if (cargo_appendf(&str, "%*s%-*s%s",
				NAME_PADDING, " ",
				max_name_len, namebufs[i],
				(option_causes_newline ? "\n" : "")) < 0)
		{
			ret = -1; goto fail;
		}

		// Option description.
		if ((ctx->usage.format & CARGO_FORMAT_RAW_OPT_DESCRIPTION)
			|| (strlen(ctx->options[i].description) < ctx->usage.max_width))
		{
			if (cargo_appendf(&str, "%*s%s\n",
				NAME_PADDING, "", ctx->options[i].description) < 0)
			{
				ret = -1; goto fail;
			}
		}
		else
		{
			// TODO: Break out into separate function.
			// Add line breaks to fit the width we want.
			char **desc_lines = NULL;
			size_t line_count = 0;

			int padding = 0;
			char *opt_description = _cargo_linebreak(ctx,
										ctx->options[i].description,
										ctx->usage.max_width - 2
										- max_name_len - (2 * NAME_PADDING));

			CARGODBG(1, "ctx->usage.max_width - 2 - max_name_len - (2 * NAME_PADDING) =\n%d - 2 - %d - (2 * %d) = %d\n",
					ctx->usage.max_width, max_name_len, NAME_PADDING, ctx->usage.max_width - 2 - max_name_len - (2 * NAME_PADDING));

			if (!opt_description)
			{
				ret = -1; goto fail;
			}

			if (!(desc_lines = _cargo_linebreak_to_list(ctx,
									opt_description, &line_count)))
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
					padding = max_name_len + NAME_PADDING;
				}

				CARGODBG(1, "line len: %d\n", strlen(desc_lines[j]));

				if (cargo_appendf(&str, "  %*s%s\n",
					padding, "", desc_lines[j]) < 0)
				{
					ret = -1; goto fail;
				}
			}

			free(opt_description);
			free(desc_lines);
		}
	}

	if (ctx->epilog && !(ctx->usage.format & CARGO_FORMAT_HIDE_EPILOG))
	{
		if (cargo_appendf(&str, "%s\n", ctx->epilog) < 0) goto fail;
	}

fail:
	if (namebufs)
	{
		for (i = 0; i < ctx->opt_count; i++)
		{
			if (namebufs[i])
			{
				free(namebufs[i]);
			}
		}

		free(namebufs);
	}

	return ret;
}

int cargo_print_usage(cargo_t ctx)
{
	char *buf;
	assert(ctx);

	cargo_get_usage(ctx, &buf, NULL);
	printf("%s\n", buf);
	free(buf);

	return 0;
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
// cargo_add_option("-l --listen", CARGO_ALLOC, "=s?", &laddr);
//
static int _cargo_parse_opt_format(cargo_t ctx)
{

}

int cargo_add_optionv(cargo_t ctx, const char *optnames, 
					  const char *description, const char *fmt, va_list ap)
{
	const char *s = fmt;
	cargo_type_t type;
	int ret;
	int array = 0;
	size_t optcount = 0;
	char **optname_list  = NULL;
	int alloc = 0;
	void *target = NULL;
	size_t *target_count = NULL;
	void **alloc_target = NULL;
	assert(ctx);

	// TODO: Split the optnames list.
	if (!(optname_list = _cargo_split(ctx, optnames, " ", &optcount))
		|| (optcount <= 0))
	{
		fprintf(stderr, "Failed to split option name list: \"%s\"\n", optnames);
		return -1;
	}

	// Parse format.
	s += strspn(s, " \t");

	if (*s == '=')
	{
		alloc = 1;
		s++;
	}

	if (*s == '[')
	{
		if (!strchr(s, ']'))
		{
			fprintf(stderr, "Format parse error: Expected ']'\n");
			return -1;
		}

		array = 1;
		s++;
	}

	while (*s)
	{
		s += strspn(s, " \t");

		if (array)
		{

		}
		else
		{
			if (alloc)
			{
				
			}
			else
			{
				if (*s == 'f')
				{
					float *f;
					type = CARGO_FLOAT;
					target = (void *)va_arg(ap, float *);
					target_count = NULL;
				}
				else if (*s == 's')
				{
					type = CARGO_STRING;
					target = (void *)va_arg(ap, const char *);
					target_count = va_arg(ap, size_t *);
				}
			}

			// TODO: static allocation of string, how?
			ret = cargo_add(ctx, optname_list[0], (void *)target, 
							type, description);
			/*ret = cargo_add(cargo, "--ducks",
						(void **)&args.ducks, &args.duck_count,
						2, CARGO_INT, "How man ducks live on the farm");*/
		}
	}

fail:
	free(optname_list);

	return ret;
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
				 2, CARGO_INT, "How man ducks live on the farm");

	args.arne = 4.4f;
	ret |= cargo_add(cargo, "--arne", &args.arne, CARGO_FLOAT,
				"Arne");
	cargo_add_alias(cargo, "--arne", "-a");

	args.poem_count = 0;
	ret |= cargo_addv(cargo, "--poemspoemspoemspoemspoemspoemspoemspoemspoemspoemspoemspoemspoems", args.poems, &args.poem_count, 3,
				CARGO_STRING,
				"The poems. A very very long\ndescription for an option, "
				"this couldn't possibly fit just one line, let's see if "
				"we get any more? Let's try even more crap here! Awesomely "
				"long sentences going on forever, so much crap in this text "
				"that it's hard to read");

	ret |= cargo_addv_alloc(cargo, "--tut", (void **)&args.tut, &args.tut_count, 
							5, CARGO_STRING, "Tutiness");

	args.blurp_count = 5;
	ret |= cargo_addv_alloc(cargo, "--blurp", (void **)&args.blurp, &args.blurp_count, 
							CARGO_NARGS_NONE_OR_MORE, CARGO_STRING, "Blurp");

	if (ret != 0)
	{
		fprintf(stderr, "Failed to add argument\n");
		ret = -1; goto fail;
	}

	if ((ret = cargo_parse(cargo, 1, argc, argv)))
	{
		if (ret < 0)
		{
			fprintf(stderr, "Error parsing!\n");
			ret = -1; goto fail;
		}

		ret = 0; goto done;
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

	if (extra_count > 0)
	{
		for (i = 0; i < extra_count; i++)
		{
			printf("%s\n", extra_args[i]);
		}
	}
	else
	{
		printf("  [None]\n");
	}

	cargo_print_usage(cargo);
done:
fail:
	cargo_destroy(&cargo);
	if (args.tut)
		free(args.tut);

	return ret;
}

#endif

