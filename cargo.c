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
#include <io.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <wordexp.h>
#endif // _WIN32

#ifdef CARGO_DEBUG
#define CARGODBG(level, fmt, ...)											\
do 																			\
{																			\
	if (level == 1)															\
	{																		\
		fprintf(stderr, "[cargo.c:%4d]: [ERROR] " fmt, __LINE__, ##__VA_ARGS__); \
	}																		\
	else if (level <= CARGO_DEBUG)											\
	{																		\
		fprintf(stderr, "[cargo.c:%4d]: " fmt, __LINE__, ##__VA_ARGS__);	\
	}																		\
} while (0)

#define CARGODBGI(level, fmt, ...)											\
do 																			\
{																			\
	if (level <= CARGO_DEBUG)												\
	{																		\
		fprintf(stderr, fmt, ##__VA_ARGS__);								\
	}																		\
} while (0)

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

#define CARGO_MIN(a, b) ((a) < (b) ? a : b)
#define CARGO_MAX(a, b) ((a) > (b) ? a : b)

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

int cargo_snprintf(char *buf, size_t buflen, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = cargo_vsnprintf(buf, buflen, format, ap);
	va_end(ap);
	return r;
}

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
	size_t diff;
} cargo_str_t;

int cargo_vappendf(cargo_str_t *str, const char *format, va_list ap)
{
	int ret;
	assert(str);

	str->diff = (str->l - str->offset);

	if ((ret = cargo_vsnprintf(&str->s[str->offset],
			str->diff, format, ap)) < 0)
	{
		return -1;
	}

	str->offset += ret;

	return ret;
}

#define CARGO_ASTR_DEFAULT_SIZE 256
typedef struct cargo_astr_s
{
	char **s;
	size_t l;
	size_t offset;
	size_t diff;
} cargo_astr_t;

int cargo_avappendf(cargo_astr_t *str, const char *format, va_list ap)
{
	int ret = 0;
	va_list apc;
	assert(str);
	assert(str->s);

	if (!(*str->s))
	{
		if (str->l == 0)
		{
			str->l = CARGO_ASTR_DEFAULT_SIZE;
		}

		str->offset = 0;

		if (!(*str->s = calloc(1, str->l)))
		{
			CARGODBG(1, "Out of memory!\n");
			return -1;
		}
	}

	while (1)
	{
		// We must copy the va_list otherwise it will be
		// out of sync on a realloc.
		va_copy(apc, ap);

		str->diff = (str->l - str->offset);

		if ((ret = cargo_vsnprintf(&(*str->s)[str->offset],
				str->diff, format, apc)) < 0)
		{
			CARGODBG(1, "Formatting error\n");
			return -1;
		}

		va_end(apc);

		if ((size_t)ret >= str->diff)
		{
			str->l *= 2;
			CARGODBG(4, "Realloc %lu\n", str->l);

			if (!(*str->s = realloc(*str->s, str->l)))
			{
				CARGODBG(1, "Out of memory!\n");
				return -1;
			}

			continue;
		}

		break;
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

int cargo_aappendf(cargo_astr_t *str, const char *fmt, ...)
{
	int ret;
	va_list ap;
	assert(str);
	va_start(ap, fmt);
	ret = cargo_avappendf(str, fmt, ap);
	va_end(ap);
	return ret;
}

#ifndef _WIN32
int _vscprintf(const char *format, va_list argptr)
{
    return(vsnprintf(NULL, 0, format, argptr));
}
#endif

//
// Below is code for translating ANSI color escape codes to
// windows equivalent API calls.
//
int cargo_vasprintf(char **strp, const char *format, va_list ap)
{
	int count;
	va_list apc;
	assert(strp);

	// Find out how long the resulting string is
	va_copy(apc, ap);
	count = _vscprintf(format, apc);
	va_end(apc);

	if (count == 0)
	{
		*strp = strdup("");
		return 0;
	}
	else if (count < 0)
	{
		// Something went wrong, so return the error code (probably still requires checking of "errno" though)
		return count;
	}

	// Allocate memory for our string
	if (!(*strp = malloc(count + 1)))
	{
		return -1;
	}

	// Do the actual printing into our newly created string
	return vsprintf(*strp, format, ap);
}

int cargo_asprintf(char** strp, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
	count = cargo_vasprintf(strp, format, ap);
	va_end(ap);

	return count;
}

#ifdef _WIN32
//
// Conversion tables and structs below are from
// https://github.com/adoxa/ansicon
//
#define CARGO_ANSI_MAX_ARG 16
#define CARGO_ANSI_ESC '\x1b'

#define FOREGROUND_BLACK 0
#define FOREGROUND_WHITE FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE

#define BACKGROUND_BLACK 0
#define BACKGROUND_WHITE BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE

const BYTE foregroundcolor[8] =
{
	FOREGROUND_BLACK,					// black foreground
	FOREGROUND_RED,						// red foreground
	FOREGROUND_GREEN,					// green foreground
	FOREGROUND_RED | FOREGROUND_GREEN,	// yellow foreground
	FOREGROUND_BLUE,					// blue foreground
	FOREGROUND_BLUE | FOREGROUND_RED,	// magenta foreground
	FOREGROUND_BLUE | FOREGROUND_GREEN,	// cyan foreground
	FOREGROUND_WHITE					// white foreground
};

const BYTE backgroundcolor[8] =
{
	BACKGROUND_BLACK,					// black background
	BACKGROUND_RED,						// red background
	BACKGROUND_GREEN,					// green background
	BACKGROUND_RED | BACKGROUND_GREEN,	// yellow background
	BACKGROUND_BLUE,					// blue background
	BACKGROUND_BLUE | BACKGROUND_RED,	// magenta background
	BACKGROUND_BLUE | BACKGROUND_GREEN,	// cyan background
	BACKGROUND_WHITE,					// white background
};

// Map windows console attribute to ANSI number.
const BYTE attr2ansi[8] =
{
	0,					// black
	4,					// blue
	2,					// green
	6,					// cyan
	1,					// red
	5,					// magenta
	3,					// yellow
	7					// white
};

// Used to save the state while parsing the render attributes.
typedef struct cargo_ansi_state_s
{
	WORD 	def;		// The default attribute.
	BYTE	foreground;	// ANSI base color (0 to 7; add 30)
	BYTE	background;	// ANSI base color (0 to 7; add 40)
	BYTE	bold;		// console FOREGROUND_INTENSITY bit
	BYTE	underline;	// console BACKGROUND_INTENSITY bit
	BYTE	rvideo; 	// swap foreground/bold & background/underline
	BYTE	concealed;	// set foreground/bold to background/underline
	BYTE	reverse;	// swap console foreground & background attributes
} cargo_ansi_state_t;

// ANSI render modifiers used by \x1b[#;#;...;#m
//
// 00 for normal display (or just 0)
// 01 for bold on (or just 1)
// 02 faint (or just 2)
// 03 standout (or just 3)
// 04 underline (or just 4)
// 05 blink on (or just 5)
// 07 reverse video on (or just 7)
// 08 nondisplayed (invisible) (or just 8)
// 22 normal
// 23 no-standout
// 24 no-underline
// 25 no-blink
// 27 no-reverse
// 30 black foreground
// 31 red foreground
// 32 green foreground
// 33 yellow foreground
// 34 blue foreground
// 35 magenta foreground
// 36 cyan foreground
// 37 white foreground
// 39 default foreground
// 40 black background
// 41 red background
// 42 green background
// 43 yellow background
// 44 blue background
// 45 magenta background
// 46 cyan background
// 47 white background
// 49 default background

void cargo_ansi_to_win32(cargo_ansi_state_t *state, int ansi)
{
	assert(state);

	if ((ansi >= 30) && (ansi <= 37))
	{
		// Foreground colors.
		// We remove 30 so we can use our translation table later.
		state->foreground = (ansi - 30);
	}
	else if ((ansi >= 40) && (ansi <= 47))
	{
		// Background colors.
		state->background = (ansi - 40);
	}
	else switch (ansi)
	{
		// Reset to defaults.
		case 0:  // 00 for normal display (or just 0)
		case 39: // 39 default foreground
		case 49: // 49 default background
		{
			WORD a = 7;

			if (a < 0)
			{
				state->reverse = 1;
				a = -a;
			}

			if (ansi != 49)
				state->foreground = attr2ansi[a & 7];

			if (ansi != 39)
				state->background = attr2ansi[(a >> 4) & 7];

			// Reset all to default
			// (or rather what the console was set to before we started parsing)
			if (ansi == 0)
			{
				state->bold = 0;
				state->underline = 0;
				state->rvideo = 0;
				state->concealed = 0;
			}
			break;
		}
		case 1: state->bold = FOREGROUND_INTENSITY; break;
		case 5: // blink.
		case 4: state->underline = BACKGROUND_INTENSITY; break;
		case 7: state->rvideo = 1; break;
		case 8: state->concealed = 1; break;
		case 21: // Double underline says ansicon
		case 22: state->bold = 0; break;
		case 25: // no blink.
		case 24: state->underline = 0; break;
		case 27: state->rvideo = 0; break;
		case 28: state->concealed = 0; break;
	}
}

WORD cargo_ansi_state_to_attr(cargo_ansi_state_t *state)
{
	WORD attr = 0;
	assert(state);

	if (state->concealed)
	{
		// Reversed video.
		if (state->rvideo)
		{
			attr = foregroundcolor[state->foreground]
				 | backgroundcolor[state->foreground];

			if (state->bold)
			{
				attr |= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
			}
		}
		else // Normal.
		{
			attr = foregroundcolor[state->background]
				 | backgroundcolor[state->background];

			if (state->underline)
			{
				attr |= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
			}
		}
	}
	else if (state->rvideo)
	{
		attr = foregroundcolor[state->background]
			 | backgroundcolor[state->foreground];

		if (state->bold) attr |= BACKGROUND_INTENSITY;
		if (state->underline) attr |= FOREGROUND_INTENSITY;
	}
	else
	{
		attr = foregroundcolor[state->foreground] | state->bold
			 | backgroundcolor[state->background] | state->underline;
	}

	if (state->reverse)
	{
		attr = ((attr >> 4) & 15) | ((attr & 15) << 4);
	}

	return attr;
}

void cargo_print_ansicolor(FILE *fd, const char *buf)
{
	const char *start = NULL;
	const char *end = NULL;
	char *numend = NULL;
	const char *s = buf;
	int i = 0;
	cargo_ansi_state_t state;
	HANDLE handle = INVALID_HANDLE_VALUE;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	memset(&state, 0, sizeof(cargo_ansi_state_t));

	handle = (HANDLE)_get_osfhandle(fileno(fd));
	GetConsoleScreenBufferInfo(handle, &csbi);

	// We reset to these attributes if we get a reset code.
	state.def = csbi.wAttributes;

	while (*s)
	{
		if (*s == CARGO_ANSI_ESC)
		{
			s++;

			// ANSI colors are in the following format:
			// \x1b[#;#;...;#m
			//
			// Where # is a set of colors and modifiers, see above for table.
			if (*s == '[')
			{
				// TODO: Add support for other Escape codes than render ones.
				s++;
				end = strchr(s, 'm');

				if (!end || ((end - s) > CARGO_ANSI_MAX_ARG))
				{
					// Output other escaped sequences
					// like normal, we just support colors!
					continue;
				}

				// Parses #;#;#;# until there is no more ';'
				// at the end.
				do
				{
					i = strtol(s, &numend, 0);
					cargo_ansi_to_win32(&state, i);
					s = numend + 1;
				} while (*numend == ';');

				SetConsoleTextAttribute(handle,
					cargo_ansi_state_to_attr(&state));
			}
		}
		else
		{
			putchar(*s);
			s++;
		}
	}
}

void cargo_fprintf(FILE *fd, const char *fmt, ...)
{
	int ret;
	va_list ap;
	char *s = NULL;

	va_start(ap, fmt);
	ret = cargo_vasprintf(&s, fmt, ap);
	va_end(ap);

	if (ret != -1)
	{
		cargo_print_ansicolor(fd, s);
	}

	if (s) free(s);
}

#define fprintf cargo_fprintf
#define printf cargo_printf

#else // Unix below (already has ANSI support).

void cargo_print_ansicolor(FILE *fd, const char *buf)
{
	fprintf(fd, "%s", buf);
}

#define cargo_fprintf fprintf

#endif // End Unix

#define cargo_printf(fmt, ...) cargo_fprintf(stdout, fmt, ##__VA_ARGS__)

#define CARGO_NARGS_ONE_OR_MORE 	-1
#define CARGO_NARGS_ZERO_OR_MORE	-2
#define CARGO_NARGS_ZERO_OR_ONE		-3

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

typedef struct cargo_group_s cargo_group_t;

typedef struct cargo_opt_s
{
	char *name[CARGO_NAME_COUNT];
	size_t name_count;
	const char *description; // TODO: Copy these from user.
	const char *metavar;
	int positional;
	cargo_type_t type;
	int nargs;
	int alloc;

	int group_index;

	cargo_custom_cb_t custom;
	void *custom_user;
	size_t *custom_user_count;

	char **custom_target;		// Internal storage for args passed to callback
	size_t custom_target_count;

	void **target;
	size_t target_idx;
	size_t *target_count;
	size_t lenstr;
	size_t max_target_count;

	int array;
	int parsed;
	size_t flags; // TODO: replace with cargo_option_flags_t
	int num_eaten;
} cargo_opt_t;

#define CARGO_DEFAULT_MAX_GROUPS 4
#define CARGO_DEFAULT_MAX_GROUP_OPTS 8

struct cargo_group_s
{
	char *name;
	char *title;
	char *description;
	size_t flags;

	size_t *option_indices;
	size_t opt_count;
	size_t max_opt_count;
};

typedef struct cargo_s
{
	const char *progname;
	const char *description;
	const char *epilog;
	size_t max_width;
	cargo_format_t format;
	cargo_flags_t flags;

	int i;	// argv index.
	int j;	// sub-argv index (when getting arguments for options)
	int argc;
	char **argv;
	int start;

	int add_help;
	int help;

	cargo_group_t *groups;
	size_t group_count;
	size_t max_groups;

	cargo_group_t *mutex_groups;
	size_t mutex_group_count;
	size_t mutex_max_groups;

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

	return 0;
}

static int _cargo_nargs_is_valid(int nargs)
{
	return (nargs >= 0) 
		|| (nargs == CARGO_NARGS_ZERO_OR_MORE)
		|| (nargs == CARGO_NARGS_ONE_OR_MORE)
		|| (nargs == CARGO_NARGS_ZERO_OR_ONE);
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
									size_t *opt_i, size_t *name_i)
{
	size_t i = 0;
	size_t j = 0;
	cargo_opt_t *opt = NULL;

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

static int _cargo_validate_option_args(cargo_t ctx, cargo_opt_t *o)
{
	size_t opt_len;
	char *name = o->name[0];
	assert(ctx);
	assert(o);

	if ((opt_len = strlen(name)) == 0)
	{
		CARGODBG(1, "%s", "Option name has length 0\n");
		return -1;
	}

	if (!_cargo_nargs_is_valid(o->nargs))
	{
		CARGODBG(1, "%s: nargs is invalid %d\n", name, o->nargs);
		return -1;
	}

	if (o->custom)
	{
		if (o->type != CARGO_STRING)
		{
			CARGODBG(1, "%s: Custom callback must be of type string\n", name);
			return -1;
		}

		if (!o->alloc)
		{
			CARGODBG(1, "%s: Custom callback cannot use static variable", name);
			return -1;
		}
	}

	if ((o->type != CARGO_STRING) && (o->nargs == 1) && o->alloc)
	{
		CARGODBG(1, "%s: Cannot allocate for single nonstring value\n", name);
		return -1;
	}

	if (!o->custom && !o->target)
	{
		CARGODBG(1, "%s: target NULL\n", name);
		return -1;
	}

	if (!o->custom && !o->target_count && (o->nargs > 1))
	{
		CARGODBG(1, "%s: target_count NULL, when nargs > 1\n", name);
		return -1;
	}

	return 0;
}

static int _cargo_grow_options(cargo_opt_t **options,
								size_t *opt_count, size_t *max_opts)
{
	assert(options);
	assert(opt_count);
	assert(max_opts);
	assert(*max_opts > 0);

	if (!*options)
	{
		if (!((*options) = calloc(*max_opts, sizeof(cargo_opt_t))))
		{
			CARGODBG(1, "Out of memory\n");
			return -1;
		}
	}

	if (*opt_count >= *max_opts)
	{
		cargo_opt_t *new_options = NULL;
		CARGODBG(2, "Option count (%lu) >= Max option count (%lu)\n",
			*opt_count, *max_opts);

		(*max_opts) *= 2;

		if (!(new_options = realloc(*options,
									(*max_opts) * sizeof(cargo_opt_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			return -1;
		}

		*options = new_options;
	}

	return 0;
}

static int _cargo_get_positional(cargo_t ctx, size_t *opt_i)
{
	size_t i;
	cargo_opt_t *opt = NULL;
	assert(ctx);

	*opt_i = 0;

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		if (opt->positional && (opt->num_eaten != opt->nargs))
		{
			*opt_i = i;
			return 0;
		}
	}

	return -1;
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

static void _cargo_cleanup_option_value(cargo_opt_t *opt)
{
	assert(opt);

	opt->target_idx = 0;
	opt->parsed = 0;
	opt->num_eaten = 0;

	CARGODBG(3, "Cleanup option (%s) target: %s\n", _cargo_type_map[opt->type], opt->name[0]);

	if (opt->custom)
	{
		_cargo_free_str_list(&opt->custom_target, &opt->custom_target_count);
	}

	if (opt->alloc)
	{
		CARGODBG(4, "    Allocated value\n");

		if (opt->target && *opt->target)
		{
			if (opt->array)
			{
				CARGODBG(4, "    Array\n");

				if (opt->type == CARGO_STRING)
				{
					_cargo_free_str_list((char ***)&opt->target, opt->target_count);
				}
				else
				{
					free(*opt->target);
					*opt->target = NULL;
				}
			}
			else
			{
				CARGODBG(4, "    Not array\n");

				if (opt->type == CARGO_STRING)
				{
					CARGODBG(4, "    String\n");
					free(*opt->target);
					*opt->target = NULL;
				}
			}
		}
		else
		{
			CARGODBG(4, "    Target value NULL\n");
		}
	}
	else
	{
		if (opt->target && opt->target_count)
			memset(opt->target, 0, _cargo_get_type_size(opt->type) * (*opt->target_count));
	}

	if (opt->target_count)
		*opt->target_count = 0;
}

static void _cargo_cleanup_option_values(cargo_t ctx)
{
	size_t i;
	assert(ctx);

	for (i = 0; i < ctx->opt_count; i++)
	{
		_cargo_cleanup_option_value(&ctx->options[i]);
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
	CARGODBG(2, "  nargs: %d\n", opt->nargs);

	// If number of arguments is just 1 don't allocate an array
	// (Except for custom callback, then we always use an array).
	if (opt->custom || (opt->alloc && (opt->nargs != 1)))
	{
		// Allocate the memory needed.
		if (!*(opt->target))
		{
			// TODO: Break out into function.
			void **new_target;
			int alloc_count = opt->nargs;

			if (opt->nargs < 0)
			{
				// In this case we don't want to preallocate everything
				// since we might have "unlimited" arguments.
				// CARGO_NARGS_ONE_OR_MORE
				// CARGO_NARGS_ZERO_OR_MORE
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
		CARGODBG(3, "Not allocating array or single %s\n",
				_cargo_type_map[opt->type]);

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
	// (Don't include bool here, since val will be NULL in that case).
	if ((opt->type != CARGO_BOOL) && (end == val))
	{
		// TODO: Make helper function instead.
		size_t fprint_flags =
			(ctx->flags & CARGO_NOCOLOR) ? CARGO_FPRINT_NOCOLOR : 0;

		CARGODBG(1, "Cannot parse \"%s\" as %s\n",
				val, _cargo_type_map[opt->type]);

		// TODO: Dont use fprintf here.
		cargo_fprint_args(stderr, ctx->argc, ctx->argv, ctx->start, fprint_flags,
						1, ctx->i, "~"CARGO_COLOR_RED);
		fprintf(stderr, "Cannot parse \"%s\" as %s for option %s\n",
				val, _cargo_type_map[opt->type], ctx->argv[ctx->i]);
		return -1;
	}

	// TODO: For single options that are repeated this will cause write beyond buffer!
	// --bla 1 --bla 2 <-- Second call will be written outside
	opt->target_idx++;
	CARGODBG(3, "UPDATED TARGET INDEX: %lu\n", opt->target_idx);

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
	return (opt->nargs == CARGO_NARGS_ZERO_OR_MORE)
		|| (opt->nargs == CARGO_NARGS_ZERO_OR_ONE)
		|| (opt->type == CARGO_BOOL);
}

static int _cargo_check_if_already_parsed(cargo_t ctx, cargo_opt_t *opt, const char *name)
{
	if (opt->parsed)
	{
		size_t fprint_flags =
			(ctx->flags & CARGO_NOCOLOR) ? CARGO_FPRINT_NOCOLOR : 0;
		// TODO: Don't printf directly here.

		if (opt->flags & CARGO_OPT_UNIQUE)
		{
			cargo_fprint_args(stderr, ctx->argc, ctx->argv, ctx->start,
							fprint_flags,
							2, // Number of highlights.
							opt->parsed, "^"CARGO_COLOR_GREEN,
							ctx->i, "~"CARGO_COLOR_RED);
			fprintf(stderr, " Error: %s was already specified before.\n", name);
			return -1;
		}
		else
		{
			cargo_fprint_args(stderr, ctx->argc, ctx->argv, ctx->start,
							fprint_flags,
							2,
							opt->parsed, "^"CARGO_COLOR_DARK_GRAY,
							ctx->i, "~"CARGO_COLOR_YELLOW);
			fprintf(stderr, " Warning: %s was already specified before, "
							"the latter value will be used.\n", name);

			// TODO: Should we always do this? Say --abc takes a list of integers.
			// --abc 1 2 3 ... or why not --abc 1 --def 5 --abc 2 3
			// (probably a bad idea :D)
			_cargo_cleanup_option_value(opt);
		}
	}

	return 0;
}

static int _cargo_parse_option(cargo_t ctx, cargo_opt_t *opt, const char *name,
								int argc, char **argv)
{
	int ret;
	int args_to_look_for;
	int start = opt->positional ? ctx->i : (ctx->i + 1);

	CARGODBG(2, "------ Parse option %s ------\n", opt->name[0]);
	CARGODBG(2, "argc: %d\n", argc);
	CARGODBG(2, "i: %d\n", ctx->i);
	CARGODBG(2, "start: %d\n", start);
	CARGODBG(2, "parsed: %d\n", opt->parsed);
	CARGODBG(2, "positional: %d\n", opt->positional);

	if (!opt->positional
		&& _cargo_check_if_already_parsed(ctx, opt, name))
	{
		return -1;
	}

	// Keep looking until the end of the argument list.
	if ((opt->nargs == CARGO_NARGS_ONE_OR_MORE) ||
		(opt->nargs == CARGO_NARGS_ZERO_OR_MORE) ||
		(opt->nargs == CARGO_NARGS_ZERO_OR_ONE))
	{
		args_to_look_for = (argc - start);

		switch (opt->nargs)
		{
			case CARGO_NARGS_ONE_OR_MORE:
				args_to_look_for = CARGO_MAX(args_to_look_for, 1);
				break;
			case CARGO_NARGS_ZERO_OR_ONE:
				args_to_look_for = CARGO_MIN(args_to_look_for, 1);
				break;
		}
	}
	else
	{
		// Read (number of expected arguments) - (read so far).
		args_to_look_for = (opt->nargs - opt->target_idx);
	}

	assert(args_to_look_for >= 0);
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
					// TODO: Don't print this to stderr.
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
			// for CARGO_NARGS_ZERO_OR_MORE or CARGO_NARGS_ONE_OR_MORE
			// we should stop so we don't eat all the remaining arguments.
			if (ret)
				break;
		}
	}

	opt->parsed = ctx->i;

	// Number of arguments eaten.
	opt->num_eaten = (ctx->j - start);

	if (!opt->positional)
	{
		opt->num_eaten++;
	}

	CARGODBG(2, "_cargo_parse_option ate %d\n", opt->num_eaten);

	// If we're parsing using a custom callback, pass it onto that.
	if (opt->custom)
	{
		int custom_eaten = 0;

		// Set the value of the return count for the caller as well:
		// ... "[c]#", callback_func, &data, &data_count, DATA_COUNT);
		//                                   ^^^^^^^^^^^
		if (opt->custom_user_count)
		{
			CARGODBG(3, "Set custom user count: %lu\n", opt->custom_target_count);
			*opt->custom_user_count = opt->custom_target_count;
		}

		custom_eaten = opt->custom(ctx, opt->custom_user, opt->name[0],
									opt->custom_target_count, opt->custom_target);

		if (custom_eaten < 0)
		{
			CARGODBG(1, "Custom callback indicated error\n");
			return -1;
		}

		CARGODBG(2, "Custom call back ate: %d\n", custom_eaten);
	}

	return opt->num_eaten;
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
	const char *as = *((const char **)a);
	const char *bs = *((const char **)b);
	size_t alen = strlen(as);
	size_t blen = strlen(bs);
	CARGODBG(4, "Compare length: %s (%lu) < %s (%lu) (%lu)\n",
			as, alen, bs, blen, (alen - blen));
	return (alen - blen);
}

static int _cargo_generate_metavar(cargo_t ctx, cargo_opt_t *opt, char *buf, size_t bufsize)
{
	int j = 0;
	int i = 0;
	char metavarname[20];
	cargo_str_t str;
	assert(ctx);
	assert(opt);

	memset(&str, 0, sizeof(str));
	memset(buf, 0, bufsize);
	str.s = buf;
	str.l = bufsize;

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
	cargo_str_t str;
	assert(ctx);
	assert(opt);

	memset(&str, 0, sizeof(str));
	str.s = namebuf;
	str.l = buf_size;

	CARGODBG(3, "Sorting %lu option names:\n", opt->name_count);

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
		if (opt->positional)
			continue;

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

static char **_cargo_split(const char *s, const char *splitchars, size_t *count)
{
	char **ss;
	size_t i = 0;
	char *p = NULL;
	char *scpy = NULL;
	char *end = NULL;
	size_t splitlen = strlen(splitchars);
	size_t split_count = 0;
	assert(count);

	*count = 0;

	if (!s)
		return NULL;

	if (!*s)
		return NULL;

	if (!(scpy = strdup(s)))
		return NULL;

	p = scpy;
	end = scpy + strlen(scpy);
	p += strspn(p, splitchars);

	*count = 1;

	while (*p)
	{
		// If the string ends in just splitchars
		// don't count the last empty string.
		if ((p + strspn(p, splitchars)) >= end)
		{
			break;
		}

		// Look for a split character.
		for (i = 0; i < splitlen; i++)
		{
			if (*p == splitchars[i])
			{
				(*count)++;
				break;
			}
		}

		// Consume all duplicates so we don't get
		// a bunch of empty strings.
		p += strspn(p, splitchars) + 1;
	}

	if (!(ss = calloc(*count, sizeof(char *))))
		goto fail;

	p = strtok(scpy, splitchars);
	i = 0;

	while (p && (i < (*count)))
	{
		p += strspn(p, splitchars);

		if (!(ss[i] = strdup(p)))
		{
			goto fail;
		}

		p = strtok(NULL, splitchars);
		i++;
	}

	// The count and number of strings must match.
	assert(i == *count);

	free(scpy);

	return ss;
fail:
	if (scpy) free(scpy);
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

	p = strpbrk(p, " \n");
	while (p != NULL)
	{
		CARGODBG(6, "(p - start) = %ld: %.*s\n",
			(p - start), (int)(p - start), start);

		// Restart on already existing explicit line breaks.
		if (*p == '\n')
		{
			CARGODBG(5, "EXISTING NEW LINE len %ld:\n%.*s\n\n",
				(p - start), (int)(p - start), start);
			start = p;
		}
		else if ((size_t)(p - start) > width)
		{
			// We found a word that goes beyond the width we're
			// aiming for, so add the line break before that word.
			*prev = '\n';
			CARGODBG(5, "ADD NEW LINE len %ld:\n%.*s\n\n",
				(prev - start), (int)(prev - start), start);
			start = prev;
			continue;
		}

		prev = p;
		p = strpbrk(p + 1, " \n");

		CARGODBG(5, "\n");
	}

	return s;
}

static void _cargo_add_help_if_missing(cargo_t ctx)
{
	assert(ctx);

	if (ctx->add_help && _cargo_find_option_name(ctx, "--help", NULL, NULL))
	{
		if (cargo_add_option(ctx, "--help", "Show this help", "b", &ctx->help))
		{
			CARGODBG(1, "Failed to add --help\n");
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
			i1 = DA[(int)t[j - 1]];
			j1 = DB;
			cost = ((s[ i - 1] == t[j - 1]) ? 0 : 1);

			if (cost == 0) 
				DB = j;

			d(i + 1, j + 1) = min4(d(i, j) + cost, 
							  d(i + 1, j) + 1,
							  d(i, j + 1) + 1, 
							  d(i1, j1) + (i - i1 - 1) + 1 + (j - j1 - 1));
		}

		DA[(int)s[i - 1]] = i;
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

	// We want to fit the opt names + description within max_width
	// We already know the width of the opt names (max_name_len)
	// so calculate how wide the description is allowed to be
	// including any spaces and padding between them.
	size_t max_desc_len =
		ctx->max_width 	// Total width of names + description.
		- 2 			// 2 for spaces before/after opt " --opt ".
		- max_name_len 	// The longest of the opt names.
		- name_padding;	// Padding.

	char *opt_description = NULL;

	CARGODBG(2, "max_desc_len = %lu\n", max_desc_len);

	opt_description = _cargo_linebreak(ctx, ctx->options[i].description,
											max_desc_len);

	if (!opt_description)
	{
		ret = -1; goto fail;
	}

	CARGODBG(5, "ctx->max_width - 2 - max_name_len - (2 * NAME_PADDING) =\n");
	CARGODBG(5, "%lu - 2 - %d - (2 * %d) = %lu\n",
		ctx->max_width, max_name_len,
		name_padding,
		max_desc_len);

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

static int _cargo_print_options(cargo_t ctx, 
								size_t *opt_indices, size_t opt_count,
								int show_positional, cargo_str_t *str,
								int max_name_len, int indent)
{
	#define NAME_PADDING 2
	size_t i = 0;
	cargo_opt_t *opt = NULL;
	int option_causes_newline = 0;
	size_t namelen = 0;
	char *name = NULL;
	int ret = -1;
	assert(str);
	assert(ctx);

	if (!(name = malloc(ctx->max_width)))
	{
		CARGODBG(1, "Out of memory!\n");
		return -1;
	}

	// Option names + descriptions.
	for (i = 0; i < opt_count; i++)
	{
		opt = &ctx->options[opt_indices[i]];

		namelen = _cargo_get_option_name_str(ctx, opt,
											name, ctx->max_width);

		// Is the option name so long we need a new line before the description?
		option_causes_newline = (int)strlen(name) > max_name_len;

		if ((show_positional && !opt->positional)
		 || (!show_positional && opt->positional))
		{
			continue;
		}

		// Print the option names.
		// "  --ducks [DUCKS ...]  "
		if (cargo_appendf(str, "%*s%-*s%s",
				indent + NAME_PADDING, " ",
				max_name_len, name,
				(option_causes_newline ? "\n" : "")) < 0)
		{
			goto fail;
		}

		// Option description.
		if ((ctx->format & CARGO_FORMAT_RAW_OPT_DESCRIPTION)
			|| (strlen(opt->description) < ctx->max_width))
		{
			if (cargo_appendf(str, "%*s%s\n",
				NAME_PADDING, "", opt->description) < 0)
			{
				goto fail;
			}
		}
		else
		{
			// Add line breaks to fit the width we want.
			if (_cargo_fit_optnames_and_description(ctx, str, i,
					indent + NAME_PADDING, option_causes_newline, max_name_len))
			{
				goto fail;
			}
		}
	}

	ret = 0;

fail:
	if (name)
	{
		free(name);
	}

	return ret;
}

static int _cargo_get_short_option_usage(cargo_t ctx,
										cargo_opt_t *opt,
										cargo_astr_t *str,
										int is_positional)
{
	int is_req;
	char metavarbuf[256];
	const char *metavar = NULL;
	assert(ctx);
	assert(opt);
	assert(str);

	// Positional arguments.
	if (is_positional && !opt->positional)
	{
		return 1;
	}

	// Options.
	if (!is_positional && opt->positional)
	{
		return 1;
	}

	if (opt->metavar)
	{
		metavar = opt->metavar;
	}
	else
	{
		if (_cargo_generate_metavar(ctx, opt, metavarbuf, sizeof(metavarbuf)))
		{
			CARGODBG(1, "Failed to generate metavar\n");
			return -1;
		}
		metavar = metavarbuf;
	}

	is_req = (opt->flags & CARGO_OPT_REQUIRED);

	cargo_aappendf(str, " ");

	if (!is_req)
	{
		cargo_aappendf(str, "[");
	}

	if (!opt->positional)
	{
		cargo_aappendf(str, "%s ", opt->name[0]);
	}

	cargo_aappendf(str, "%s", metavar);

	if (!is_req)
	{
		cargo_aappendf(str, "]");
	}

	return 0;
}

static int _cargo_get_short_option_usages(cargo_t ctx,
										cargo_astr_t *str,
										size_t indent,
										int is_positional)
{
	size_t i;
	int ret;
	cargo_astr_t opt_str;
	char *opt_s = NULL;
	assert(ctx);
	assert(str);

	for (i = 0; i < ctx->opt_count; i++)
	{
		memset(&opt_str, 0, sizeof(opt_str));
		opt_str.s = &opt_s;

		if ((ret = _cargo_get_short_option_usage(ctx,
								&ctx->options[i],
								&opt_str, is_positional)) < 0)
		{
			CARGODBG(1, "Failed to get option usage\n");
			return -1;
		}

		if (ret == 1)
		{
			continue;
		}

		// Does the option fit on this line?
		if ((str->offset + opt_str.offset) >= ctx->max_width)
		{
			cargo_aappendf(str, "\n%*s", indent, " ");
		}

		if (opt_s)
		{
			cargo_aappendf(str, "%s", opt_s);
			free(opt_s);
			opt_s = NULL;
		}
	}

	return 0;
}

static char **_cargo_split_and_verify_option_names(cargo_t ctx,
													const char *optnames,
													size_t *optcount)
{
	char **optname_list = NULL;
	char *tmp = NULL;
	size_t i;
	assert(ctx);
	assert(optcount);

	if (!(tmp = strdup(optnames)))
	{
		return NULL;
	}

	if (!(optname_list = _cargo_split(tmp, " ", optcount))
		|| (*optcount <= 0))
	{
		CARGODBG(1, "Failed to split option name list: \"%s\"\n", optnames);
		goto fail;
	}

	#ifdef CARGO_DEBUG
	CARGODBG(3, "Got %lu option names:\n", *optcount);
	for (i = 0; i < *optcount; i++)
	{
		CARGODBG(3, " %s\n", optname_list[i]);
	}
	#endif // CARGO_DEBUG

	if (!_cargo_find_option_name(ctx, optname_list[0], NULL, NULL))
	{
		CARGODBG(1, "%s already exists\n", optname_list[0]);
		goto fail;
	}

	// If this is a positional argument it has to start with
	// [a-zA-Z]
	if (!_cargo_starts_with_prefix(ctx, optname_list[0]))
	{
		if (strpbrk(optname_list[0],
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
			!= optname_list[0])
		{
			CARGODBG(1, "A positional argument must start with [a-zA-Z]\n");
			goto fail;
		}
	}

	free(tmp);
	return optname_list;

fail:
	if (tmp) free(tmp);
	if (optname_list)
	{
		_cargo_free_str_list(&optname_list, optcount);
	}

	return NULL;
}

static cargo_opt_t *_cargo_option_init(cargo_t ctx,
										const char *name,
										const char *description)
{
	char *optname = NULL;
	cargo_opt_t *o = NULL;
	assert(ctx);

	if (_cargo_grow_options(&ctx->options, &ctx->opt_count, &ctx->max_opts))
	{
		return NULL;
	}

	o = &ctx->options[ctx->opt_count];
	memset(o, 0, sizeof(cargo_opt_t));
	ctx->opt_count++;

	if (o->name_count >= CARGO_NAME_COUNT)
	{
		CARGODBG(1, "Max %d names allowed\n", CARGO_NAME_COUNT);
		return NULL;
	}

	if (!(optname = strdup(name)))
	{
		CARGODBG(1, "Out of memory\n");
		return NULL;
	}

	o->name[o->name_count] = optname;
	o->name_count++;
	o->description = description;

	return o;
}

static void _cargo_option_destroy(cargo_opt_t *o)
{
	size_t j;

	if (!o)
	{
		return;
	}

	for (j = 0; j < o->name_count; j++)
	{
		CARGODBG(2, "###### FREE OPTION NAME: %s\n", o->name[j]);
		free(o->name[j]);
		o->name[j] = NULL;
	}

	o->name_count = 0;

	// Special case for custom callback target, it is allocated
	// internally so we should always auto clean it.
	if (o->custom)
	{
		_cargo_free_str_list(&o->custom_target, &o->custom_target_count);
	}
}

typedef struct cargo_fmt_token_s
{
	int column;
	char token;
} cargo_fmt_token_t;

typedef struct cargo_fmt_scanner_s
{
	const char *fmt;
	const char *start;
	cargo_fmt_token_t prev_token;
	cargo_fmt_token_t token;
	cargo_fmt_token_t next_token;

	int column;
} cargo_fmt_scanner_t;

static char _cargo_fmt_token(cargo_fmt_scanner_t *s)
{
	return s->token.token;
}

static void _cargo_fmt_scanner_init(cargo_fmt_scanner_t *s,
									const char *fmt)
{
	assert(s);
	memset(s, 0, sizeof(cargo_fmt_scanner_t));

	CARGODBG(2, "FMT scanner init: \"%s\"\n", fmt);
	s->fmt = fmt;
	s->start = fmt;
	s->column = 0;

	s->token.token = *fmt;
}

static void _cargo_fmt_next_token(cargo_fmt_scanner_t *s)
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

static void _cargo_fmt_prev_token(cargo_fmt_scanner_t *s)
{
	s->next_token = s->token;
	s->token = s->prev_token;
	s->fmt = &s->start[s->token.column];

	CARGODBG(4, "\"%s\"\n", s->start);
	CARGODBG(4, " %*s\n", s->token.column, "^");
}

static const char *_cargo_get_option_group_name(cargo_t ctx,
										const char *optnames,
										char **grpname)
{
	int len = 0;
	char *end = NULL;
	assert(ctx);
	assert(grpname);

	*grpname = NULL;

	// Skip any whitespace.
	optnames += strspn(optnames, " \t");

	if (*optnames != '<')
	{
		return optnames;
	}

	if (!(end = strchr(optnames, '>')))
	{
		CARGODBG(1, "Missing '>' to terminate group name\n");
		return NULL;
	}

	end--;

	len = (end - optnames);

	if (!(*grpname = cargo_strndup(&optnames[1], len)))
	{
		CARGODBG(1, "Out of memory!\n");
		return NULL;
	}

	optnames += len + 2;

	return optnames;
}

static void _cargo_group_destroy(cargo_group_t *g)
{
	if (!g) return;
	if (g->option_indices) free(g->option_indices);
	if (g->name) free(g->name);
	if (g->title) free(g->title);
	if (g->description) free(g->description);

	g->option_indices = NULL;
	g->opt_count = 0;
	g->name = NULL;
	g->title = NULL;
	g->description = NULL;
}

static void _cargo_groups_destroy(cargo_t ctx)
{
	size_t i;
	assert(ctx);

	if (ctx->groups)
	{
		for (i = 0; i < ctx->group_count; i++)
		{
			_cargo_group_destroy(&ctx->groups[i]);
		}

		free(ctx->groups);
		ctx->groups = NULL;
	}

	if (ctx->mutex_groups)
	{
		for (i = 0; i < ctx->mutex_group_count; i++)
		{
			_cargo_group_destroy(&ctx->mutex_groups[i]);
		}

		free(ctx->mutex_groups);
		ctx->mutex_groups = NULL;
	}
}

// TODO: Make this return the group_index of the found group also.
static cargo_group_t *_cargo_find_group(cargo_t ctx,
					cargo_group_t *groups, size_t group_count,
					const char *name, size_t *grp_i)
{
	size_t i;
	cargo_group_t *g;
	assert(ctx);
	assert(name);
	assert(groups);

	for (i = 0; i < group_count; i++)
	{
		g = &groups[i];

		if (!strcmp(g->name, name))
		{
			if (grp_i) *grp_i = i;
			return g;
		}
	}

	return NULL;
}

static int _cargo_add_group(cargo_t ctx,
					cargo_group_t **groups, size_t *group_count,
					size_t *max_groups,
					size_t flags, const char *name,
					const char *title, const char *description)
{
	int ret = -1;
	cargo_group_t *grp = NULL;
	assert(ctx);
	assert(name);
	assert(groups);
	assert(max_groups);
	assert(group_count);

	CARGODBG(2, "Add group %s (%lu)\n", name, *group_count);

	// Initial allocation.
	if (!*groups)
	{
		(*max_groups) = CARGO_DEFAULT_MAX_GROUPS;

		if (!((*groups) = calloc(*max_groups, sizeof(cargo_group_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			return -1;
		}
	}

	if (_cargo_find_group(ctx, *groups, *group_count, name, NULL))
	{
		CARGODBG(1, "Group \"%s\" already exists\n", name);
		return -1;
	}

	// Realloc if needed.
	if (*group_count >= *max_groups)
	{
		(*max_groups) *= 2;

		if (!((*groups) = realloc(*groups,
			sizeof(cargo_group_t) * (*max_groups))))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}
	}

	grp = &(*groups)[*group_count];
	memset(grp, 0, sizeof(cargo_group_t));

	if (!(grp->name = strdup(name)))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	if (title)
	{
		if (!(grp->title = strdup(title)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}
	}
	else
	{
		if (!(grp->title = strdup(name)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}
	}

	if (grp->description)
	{
		if (!(grp->description = strdup(description)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}
	}

	grp->flags = flags;
	grp->max_opt_count = CARGO_DEFAULT_MAX_GROUP_OPTS;
	grp->opt_count = 0;

	if (!(grp->option_indices = calloc(grp->max_opt_count, sizeof(size_t))))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	(*group_count)++;
	CARGODBG(3, "  group_count after: %lu\n", *group_count);

	ret = 0;

fail:
	if (ret < 0)
	{
		_cargo_group_destroy(grp);
	}

	return ret;
}

static int _cargo_group_add_option_ex(cargo_t ctx,
										cargo_group_t *groups,
										size_t group_count,
										const char *group,
										const char *opt)
{
	size_t opt_i;
	cargo_group_t *g = NULL;
	size_t grp_i;
	cargo_opt_t *o = NULL;
	assert(ctx);
	assert(groups);
	assert(group);
	assert(opt);

	CARGODBG(2, "+++++++ Add %s to group \"%s\" +++++++\n", opt, group);

	if (!(g = _cargo_find_group(ctx, groups, group_count, group, &grp_i)))
	{
		CARGODBG(1, "No such group \"%s\"\n", group);
		return -1;
	}

	if (_cargo_find_option_name(ctx, opt, &opt_i, NULL))
	{
		CARGODBG(1, "No such option \"%s\"\n", opt);
		return -1;
	}

	assert(opt_i < ctx->opt_count);
	o = &ctx->options[opt_i];

	if (o->group_index > 0)
	{
		CARGODBG(1, "\"%s\" is already in another group \"%s\"\n",
				o->name[0], ctx->groups[o->group_index].name);
		return -1;
	}

	if (g->opt_count >= g->max_opt_count)
	{
		CARGODBG(2, "Realloc group max option count from %lu to %lu\n",
				g->max_opt_count, 2 * g->max_opt_count);

		g->max_opt_count *= 2;

		if (!(g->option_indices = realloc(g->option_indices,
				g->max_opt_count * sizeof(size_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			return -1;
		}
	}

	g->option_indices[g->opt_count] = opt_i;
	o->group_index = grp_i;
	g->opt_count++;

	CARGODBG(2, "   Group \"%s\" option count: %lu\n", g->name, g->opt_count);

	return 0;
}

static void _cargo_print_mutex_group(cargo_t ctx, cargo_group_t *g)
{
	size_t i;
	cargo_opt_t *opt = NULL;
	assert(g);

	for (i = 0; i < g->opt_count; i++)
	{
		opt = &ctx->options[g->option_indices[i]];
		fprintf(stderr, "%s%s", opt->name[0],
				(i < (g->opt_count - 1)) ? ", " : "\n");
	}
}

static int _cargo_check_mutex_groups(cargo_t ctx)
{
	int ret = -1;
	size_t i = 0;
	size_t j = 0;
	cargo_highlight_t *parse_highlights = NULL;
	size_t parsed_count = 0;
	cargo_group_t *g = NULL;
	cargo_opt_t *opt = NULL;
	assert(ctx);

	CARGODBG(2, "Check mutex %lu groups\n", ctx->mutex_group_count);

	for (i = 0; i < ctx->mutex_group_count; i++)
	{
		g = &ctx->mutex_groups[i];
		parsed_count = 0;

		if (!(parse_highlights = calloc(g->opt_count,
									sizeof(cargo_highlight_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		for (j = 0; j < g->opt_count; j++)
		{
			opt = &ctx->options[g->option_indices[j]];

			if (opt->parsed)
			{
				parse_highlights[parsed_count].i = j + ctx->start;
				parse_highlights[parsed_count].c = "~"CARGO_COLOR_RED;
				parsed_count++;
			}
		}

		if (parsed_count > 1)
		{
			char *s;
			size_t fprint_flags =
				(ctx->flags & CARGO_NOCOLOR) ? CARGO_FPRINT_NOCOLOR : 0;

			if (!(s = cargo_get_fprintl_args(ctx->argc, ctx->argv, ctx->start,
				fprint_flags, parsed_count, parse_highlights)))
			{
				CARGODBG(1, "Out of memory\n");
				goto fail;
			}

			fprintf(stderr, "%s\n", s);
			free(s);

			fprintf(stderr, "Only one of these variables allowed at the same time:\n");
			_cargo_print_mutex_group(ctx, g);
			goto fail;
		}
		else if ((parsed_count == 0)
				&& (g->flags & CARGO_MUTEXGRP_ONE_REQUIRED))
		{
			fprintf(stderr, "One of these variables is required:\n");
			_cargo_print_mutex_group(ctx, g);
			goto fail;
		}

		free(parse_highlights);
		parse_highlights = NULL;
	}

	ret = 0;
fail:
	if (parse_highlights)
	{
		free(parse_highlights);
	}

	return ret;
}

static int _cargo_add_orphans_to_default_group(cargo_t ctx)
{
	// Instead of being able to delete options from groups
	// we add any orphans to the default group at first use.
	size_t i;
	cargo_opt_t *opt = NULL;
	assert(ctx);

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		// Default group.
		if (opt->group_index < 0)
		{
			if (cargo_group_add_option(ctx, "", opt->name[0]))
			{
				CARGODBG(1, "Failed to add \"%s\" to default group\n", opt->name[0]);
				return -1;
			}
		}
	}

	return 0;
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

int cargo_init_ex(cargo_t *ctx, const char *progname, cargo_flags_t flags)
{
	cargo_s *c;
	assert(ctx);

	*ctx = (cargo_s *)calloc(1, sizeof(cargo_s));
	c = *ctx;

	if (!c)
		return -1;

	c->max_opts = CARGO_DEFAULT_MAX_OPTS;
	c->flags = flags;
	c->progname = progname;
	c->add_help = 1;
	c->prefix = CARGO_DEFAULT_PREFIX;
	cargo_set_max_width(c, CARGO_AUTO_MAX_WIDTH);

	// Add the default group.
	cargo_add_group(c, 0, "", "", "");

	return 0;
}

int cargo_init(cargo_t *ctx, const char *progname)
{
	return cargo_init_ex(ctx, progname, 0);
}

void cargo_destroy(cargo_t *ctx)
{
	size_t i;

	CARGODBG(2, "cargo_destroy: DESTROY!\n");

	if (ctx)
	{
		cargo_opt_t *opt;
		cargo_t c = *ctx;

		if (c->flags & CARGO_AUTOCLEAN)
		{
			CARGODBG(2, "Auto clean target values\n");
			_cargo_cleanup_option_values(c);
		}

		if (c->options)
		{
			CARGODBG(2, "DESTROY %lu options!\n", c->opt_count);

			for (i = 0; i < c->opt_count; i++)
			{
				opt = &c->options[i];
				CARGODBG(2, "Free opt: %s\n", opt->name[0]);
				_cargo_option_destroy(opt);
			}

			free(c->options);
			c->options = NULL;
		}

		_cargo_groups_destroy(c);

		_cargo_free_str_list(&c->args, NULL);
		_cargo_free_str_list(&c->unknown_opts, NULL);

		free(*ctx);
		ctx = NULL;
	}
}

void cargo_set_flags(cargo_t ctx, cargo_flags_t flags)
{
	assert(ctx);
	// TODO: Hmm, are there flags we should only be allowed to set in init?
	ctx->flags = flags;
}

cargo_flags_t cargo_get_flags(cargo_t ctx)
{
	assert(ctx);
	return ctx->flags;
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

typedef struct cargo_phighlight_s
{
	int i;				// Index of highlight in argv.
	char *c;			// Highlight character (followed by color).
	int indent; 		// Indent position for highlight in relation
						// to previous highlight.
	int total_indent;	// Total indentation since start of string.
	int highlight_len;	// Length of the highlight.
	int show;
} cargo_phighlight_t;

char *cargo_get_fprintl_args(int argc, char **argv, int start, size_t flags,
							size_t highlight_count,
							const cargo_highlight_t *highlights_in)
{
	char *ret = NULL;
	int i;
	int j;
	int global_indent = 0;
	size_t arglen = 0;
	cargo_str_t str;
	char *out = NULL;
	size_t out_size = 0;
	cargo_phighlight_t *highlights = NULL;
	assert(highlights_in);

	// TODO: If the buffer we return is bigger than the console width, 
	// try to show only the relevant parts. If we get a line break
	// the indentation and highlight gets completely screwed.

	if (!(highlights = calloc(highlight_count, sizeof(cargo_phighlight_t))))
	{
		CARGODBG(1, "Out of memory!\n");
		return NULL;
	}

	for (i = 0; i < (int)highlight_count; i++)
	{
		highlights[i].i = highlights_in[i].i;
		highlights[i].c = highlights_in[i].c;
	}

	// Get buffer size and highlight data.
	for (i = 0, j = 0; i < argc; i++)
	{
		arglen = (i >= start) ? strlen(argv[i]) : 0;

		if (j < (int)highlight_count)
		{
			cargo_phighlight_t *h = &highlights[j];
			if (h->i == i)
			{
				// We only keep this flag so that we can make sure we don't
				// output the color codes when an index is out of range.
				if (i >= start)
				{
					h->show = 1;
				}

				h->highlight_len = (int)arglen;
				h->total_indent = global_indent;

				// We want to indent in relation to the previous indentation.
				if (j > 0)
				{
					cargo_phighlight_t *hprev = &highlights[j - 1];
					h->indent = h->total_indent
							  - (hprev->total_indent + hprev->highlight_len);
				}
				else
				{
					h->indent = h->total_indent;
				}

				// If we use color, we must include the ANSI color code length
				// in the buffer length as well.
				out_size += + strlen(h->c);

				j++;
			}
		}

		global_indent += arglen;
		if (i >= start) global_indent++; // + 1 for space.
		out_size += global_indent;
	}

	// Allocate and fill buffer.
	out_size += 2; // New lines.
	out_size *= 2; // Two rows, one for args and one for highlighting.

	if (!(out = malloc(out_size)))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	str.s = out;
	str.l = out_size;
	str.offset = 0;

	if (!(flags & CARGO_FPRINT_NOARGS))
	{
		for (i = start; i < argc; i++)
		{
			cargo_appendf(&str, "%s ", argv[i]);
		}

		cargo_appendf(&str, "\n");
	}

	if (!(flags & CARGO_FPRINT_NOHIGHLIGHT))
	{
		for (i = 0; i < (int)highlight_count; i++)
		{
			cargo_phighlight_t *h = &highlights[i];
			char *highlvec;
			int has_color = strlen(h->c) > 1;

			if (!(highlvec = malloc(h->highlight_len)))
			{
				CARGODBG(1, "Out of memory!\n");
				goto fail;
			}

			// Use the first character as the highlight character.
			//                                ~~~~~~~~~
			memset(highlvec, *h->c, h->highlight_len);

			// If we have more characters, we append that as a string.
			// (This can be used for color ansi color codes).
			if (!(flags & CARGO_FPRINT_NOCOLOR) && has_color && h->show)
			{
				cargo_appendf(&str, "%s", &h->c[1]);
			}

			cargo_appendf(&str, "%*s%*.*s",
				h->indent, "",
				h->highlight_len,
				h->highlight_len,
				highlvec);

			if (!(flags & CARGO_FPRINT_NOCOLOR) && has_color && h->show)
			{
				cargo_appendf(&str, "%s", CARGO_COLOR_RESET);
			}

			free(highlvec);
		}
	}

	ret = out;

fail:
	if (!ret) free(out);
	free(highlights);

	return ret;
}

char *cargo_get_vfprint_args(int argc, char **argv, int start, size_t flags,
							size_t highlight_count, va_list ap)
{
	char *ret = NULL;
	int i;
	cargo_highlight_t *highlights = NULL;

	// Create a list of indices to highlight from the va_args.
	if (!(highlights = calloc(highlight_count, sizeof(cargo_highlight_t))))
	{
		CARGODBG(1, "Out of memory trying to allocate %lu highlights!\n",
				highlight_count);
		goto fail;
	}

	for (i = 0; i < (int)highlight_count; i++)
	{
		highlights[i].i = va_arg(ap, int);
		highlights[i].c = va_arg(ap, char *);
	}

	ret = cargo_get_fprintl_args(argc, argv, start, flags,
								highlight_count, highlights);

fail:
	if (highlights) free(highlights);

	return ret;
}

char *cargo_get_fprint_args(int argc, char **argv, int start, size_t flags,
							size_t highlight_count, ...)
{
	char *ret;
	va_list ap;
	va_start(ap, highlight_count);
	ret = cargo_get_vfprint_args(argc, argv, start, flags, highlight_count, ap);
	va_end(ap);
	return ret;
}

int cargo_fprint_args(FILE *f, int argc, char **argv, int start,
							size_t flags, size_t highlight_count, ...)
{
	char *ret;
	va_list ap;
	va_start(ap, highlight_count);
	ret = cargo_get_vfprint_args(argc, argv, start, flags, highlight_count, ap);
	va_end(ap);

	if (!ret)
	{
		return -1;
	}
	
	fprintf(f, "%s\n", ret);
	free(ret);
	return 0;
}

int cargo_fprintl_args(FILE *f, int argc, char **argv, int start,
							size_t flags, size_t highlight_count,
							const cargo_highlight_t *highlights)
{
	char *ret;
	if (!(ret = cargo_get_fprintl_args(argc, argv, start, flags,
								highlight_count, highlights)))
	{
		return -1;
	}

	fprintf(f, "%s\n", ret);
	free(ret);
	return 0;
}

// TODO: Maybe have an cargo_parse_ex that takes flags as well...
// Then we can for instance, either give error on unknown opts/args
// or just ignore them.
int cargo_parse(cargo_t ctx, int start_index, int argc, char **argv)
{
	size_t i;
	int start = 0;
	int opt_arg_count = 0;
	char *arg = NULL;
	const char *name = NULL;
	cargo_opt_t *opt = NULL;

	CARGODBG(2, "============ Cargo Parse =============\n");

	ctx->argc = argc;
	ctx->argv = argv;
	ctx->start = start_index;

	_cargo_add_help_if_missing(ctx);
	_cargo_add_orphans_to_default_group(ctx);

	_cargo_free_str_list(&ctx->args, NULL);
	ctx->arg_count = 0;

	_cargo_free_str_list(&ctx->unknown_opts, NULL);
	ctx->unknown_opts_count = 0;

	// Make sure we start over, if this function is
	// called more than once.
	_cargo_cleanup_option_values(ctx);

	if (!(ctx->args = (char **)calloc(argc, sizeof(char *))))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	if (!(ctx->unknown_opts = (char **)calloc(argc, sizeof(char *))))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	CARGODBG(2, "Parse arg list of count %d start at index %d\n", argc, start_index);

	for (ctx->i = start_index; ctx->i < argc; )
	{
		arg = argv[ctx->i];
		start = ctx->i;
		opt_arg_count = 0;

		CARGODBG(3, "\n");
		CARGODBG(3, "argv[%d] = %s\n", ctx->i, arg);
		CARGODBG(3, "  Look for opt matching %s:\n", arg);

		// TODO: Add support for a "--" option which forces all
		// options specified after it to be parsed as positional arguments
		// and not options. Say there's a file named "-thefile".

		// TODO: Add support for abbreviated prefix matching so that
		// --ar will match --arne unless it's ambigous with some other option.

		if ((name = _cargo_check_options(ctx, &opt, argc, argv, ctx->i)))
		{
			// We found an option, parse any arguments it might have.
			if ((opt_arg_count = _cargo_parse_option(ctx, opt, name,
													argc, argv)) < 0)
			{
				CARGODBG(1, "Failed to parse %s option: %s\n",
						_cargo_type_map[opt->type], name);
				goto fail;
			}
		}
		else
		{
			// TODO: Optional to fail on unknown options and arguments!
			if (_cargo_is_prefix(ctx, argv[ctx->i][0]))
			{
				CARGODBG(2, "    Unknown option: %s\n", argv[ctx->i]);
				ctx->unknown_opts[ctx->unknown_opts_count] = argv[ctx->i];
				ctx->unknown_opts_count++;
				opt_arg_count = 1;
			}
			else
			{
				size_t opt_i = 0;
				CARGODBG(2, "    Positional argument: %s\n", argv[ctx->i]);

				// Positional argument.
				if (_cargo_get_positional(ctx, &opt_i) == 0)
				{
					opt = &ctx->options[opt_i];
					if ((opt_arg_count = _cargo_parse_option(ctx, opt,
														opt->name[0],
														argc, argv)) < 0)
					{
						CARGODBG(1, "    Failed to parse %s option: %s\n",
								_cargo_type_map[opt->type], name);
						goto fail;
					}
				}
				else
				{
					CARGODBG(2, "    Extra argument: %s\n", argv[ctx->i]);
					ctx->args[ctx->arg_count] = argv[ctx->i];
					ctx->arg_count++;
					opt_arg_count = 1;
				}
			}
		}

		ctx->i += opt_arg_count;

		#if CARGO_DEBUG
		{
			int k = 0;
			int ate = opt_arg_count;

			CARGODBG(2, "    Ate %d args: ", opt_arg_count);

			for (k = start; k < (start + opt_arg_count); k++)
			{
				CARGODBGI(2, "\"%s\" ", argv[k]);
			}

			CARGODBGI(2, "%s", "\n");
		}
		#endif // CARGO_DEBUG
	}

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		if ((opt->flags & CARGO_OPT_REQUIRED) && !opt->parsed)
		{
			CARGODBG(1, "Missing required option %s\n", opt->name[0]);
			fprintf(stderr, "Missing required option %s\n", opt->name[0]);
			goto fail;
		}
	}

	if (_cargo_check_mutex_groups(ctx))
	{
		goto fail;
	}

	if (ctx->unknown_opts_count > 0)
	{
		const char *suggestion = NULL;
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

		goto fail;
	}

	if (ctx->help)
	{
		cargo_print_usage(ctx);
		return 1;
	}

	return 0;
fail:
	_cargo_cleanup_option_values(ctx);
	return -1;
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
	size_t opt_i;
	size_t name_i;
	cargo_opt_t *opt;
	assert(ctx);

	if (_cargo_find_option_name(ctx, optname, &opt_i, &name_i))
	{
		CARGODBG(1, "Failed alias %s to %s, not found.\n", optname, alias);
		return -1;
	}

	CARGODBG(2, "Found option \"%s\"\n", optname);

	opt = &ctx->options[opt_i];

	if (opt->positional)
	{
		CARGODBG(1, "Cannot add alias for positional argument\n");
		return -1;
	}

	if (opt->name_count >= CARGO_NAME_COUNT)
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
	size_t opt_i;
	size_t name_i;
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

char *cargo_get_short_usage(cargo_t ctx)
{
	char *b = NULL;
	size_t indent = 0;
	cargo_astr_t str;
	assert(ctx);

	_cargo_add_help_if_missing(ctx);
	_cargo_add_orphans_to_default_group(ctx);

	memset(&str, 0, sizeof(str));
	str.s = &b;

	cargo_aappendf(&str, "Usage: %s", ctx->progname);
	indent = str.offset;

	// Options.
	_cargo_get_short_option_usages(ctx, &str, indent, 0);

	// Positional arguments at the end.
	_cargo_get_short_option_usages(ctx, &str, indent, 1);

	// Reallocate the memory used for the string so it's too big.
	if (!(b = realloc(b, str.offset + 1)))
	{
		CARGODBG(1, "Out of memory!\n");
		return NULL;
	}

	return b;
}

char *cargo_get_usage(cargo_t ctx)
{
	char *ret = NULL;
	size_t i;
	char *b = NULL;
	char *name = NULL;
	int usagelen = 0;
	int namelen;
	int max_name_len = 0;
	size_t positional_count = 0;
	size_t option_count = 0;
	char *short_usage = NULL;
	cargo_opt_t *opt = NULL;
	cargo_group_t *grp = NULL;
	cargo_str_t str;
	int is_default_group = 1;
	assert(ctx);
	#define MAX_OPT_NAME_LEN 40

	if (!(short_usage = cargo_get_short_usage(ctx)))
	{
		CARGODBG(1, "Failed to get short usage\n");
		return NULL;
	}

	// First get option names and their length.
	// We get the widest one so we know the column width to use
	// for the final result.
	//   --option_a         Some description.
	//   --longer_option_b  Another description...
	// ^-------------------^
	// What should the above width be.
	if (!(name = malloc(ctx->max_width)))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	for (i = 0; i < ctx->opt_count; i++)
	{
		opt = &ctx->options[i];

		if (opt->positional)
		{
			positional_count++;
		}
		else
		{
			option_count++;
		}

		namelen = _cargo_get_option_name_str(ctx, opt,
											name, ctx->max_width);

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

		usagelen += namelen + strlen(opt->description);
	}

	// TODO: Don't "guess" the length like this, use cargo_aapendf instead!
	if (ctx->description && !(ctx->format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		usagelen += strlen(ctx->description);
	}

	if (ctx->epilog && !(ctx->format & CARGO_FORMAT_HIDE_EPILOG))
	{
		usagelen += strlen(ctx->epilog);
	}

	usagelen += strlen(short_usage);

	// Add some extra to fit padding.
	// TODO: Let's be more strict here and use the exact padding instead.
	usagelen = (int)(usagelen * 2.5);

	// Allocate the final buffer.
	if (!(b = malloc(usagelen)))
	{
		CARGODBG(1, "Out of memory!\n");
		goto fail;
	}

	str.s = b;
	str.l = usagelen;
	str.offset = 0;

	cargo_appendf(&str, "%s\n", short_usage);

	if(ctx->description && !(ctx->format & CARGO_FORMAT_HIDE_DESCRIPTION))
	{
		if (cargo_appendf(&str, "%s\nn", ctx->description) < 0) goto fail;
	}

	CARGODBG(2, "max_name_len = %d, ctx->max_width = %lu\n",
			max_name_len, ctx->max_width);

	#ifdef CARGO_DEBUG
	for (i = 0; i < ctx->group_count; i++)
	{
		CARGODBG(2, "%s: %lu\n",
			ctx->groups[i].name, ctx->groups[i].opt_count);
	}
	#endif

	for (i = 0; i < ctx->group_count; i++)
	{
		int indent = 0;
		grp = &ctx->groups[i];

		// The group name is always set for normal groups.
		// The default group is simply "".
		is_default_group = (strlen(grp->name) == 0);

		if (!is_default_group)
		{
			indent = 2;
		}

		if (!is_default_group) cargo_appendf(&str, "\n%s:\n", grp->title);
		if (grp->description && strlen(grp->description))
			cargo_appendf(&str, "%*s%s\n", indent, " ", grp->description);

		if (cargo_appendf(&str,  "\n") < 0) goto fail;

		// Note, we only show the "Positional arguments" and "Options"
		// titles for the default group. It becomes quite spammy otherwise.
		if (positional_count > 0)
		{
			if (is_default_group)
				if (cargo_appendf(&str,  "Positional arguments:\n") < 0) goto fail;

			if (_cargo_print_options(ctx, grp->option_indices, grp->opt_count,
									1, &str, max_name_len, indent))
			{
				goto fail;
			}
		}

		if (option_count > 0)
		{
			if (is_default_group)
				if (cargo_appendf(&str,  "Options:\n") < 0) goto fail;

			if (_cargo_print_options(ctx, grp->option_indices, grp->opt_count,
									0, &str, max_name_len, indent))
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
		if (b)
		{
			free(b);
		}
	}

	if (name)
	{
		free(name);
	}

	if (short_usage)
	{
		free(short_usage);
	}

	return ret;
}

int cargo_fprint_usage(FILE *f, cargo_t ctx)
{
	char *buf;
	assert(ctx);

	if (!(buf = cargo_get_usage(ctx)))
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
	return cargo_fprint_usage(stderr, ctx);
}

int cargo_add_group(cargo_t ctx, size_t flags, const char *name,
					const char *title, const char *description)
{
	return _cargo_add_group(ctx, &ctx->groups, &ctx->group_count,
							&ctx->max_groups,
							flags, name, title, description);
}

int cargo_group_add_option(cargo_t ctx, const char *group, const char *opt)
{
	return _cargo_group_add_option_ex(ctx,
				ctx->groups, ctx->group_count, group, opt);
}

int cargo_add_mutex_group(cargo_t ctx, size_t flags, const char *name)
{
	return _cargo_add_group(ctx, &ctx->mutex_groups, &ctx->mutex_group_count,
							&ctx->mutex_max_groups,
							flags, name, NULL, NULL);
}

int cargo_mutex_group_add_option(cargo_t ctx, const char *group, const char *opt)
{
	return _cargo_group_add_option_ex(ctx,
				ctx->mutex_groups, ctx->mutex_group_count, group, opt);
}

int cargo_add_optionv_ex(cargo_t ctx, size_t flags, const char *optnames,
					  const char *description, const char *fmt, va_list ap)
{
	size_t optcount = 0;
	char **optname_list = NULL;
	int ret = -1;
	size_t i = 0;
	cargo_fmt_scanner_t s;
	cargo_opt_t *o = NULL;
	char *optname = NULL;
	char *grpname = NULL;
	assert(ctx);

	CARGODBG(2, "-------- Add option \"%s\", \"%s\" --------\n", optnames, fmt);

	if (!(optnames = _cargo_get_option_group_name(ctx, optnames, &grpname)))
	{
		CARGODBG(1, "Failed to parse group name\n");
		return -1;
	}

	CARGODBG(2, " Group: \"%s\", Optnames: \"%s\"\n", grpname, optnames);

	if (!(optname_list = _cargo_split_and_verify_option_names(ctx, optnames, &optcount)))
	{
		CARGODBG(1, "Failed to split option names \"%s\"\n", optnames);
		goto fail;
	}

	if (!(o = _cargo_option_init(ctx, optname_list[0], description)))
	{
		CARGODBG(1, "Failed to init option\n");
		goto fail;
	}

	if (grpname)
	{
		if (cargo_group_add_option(ctx, grpname, o->name[0]))
		{
			CARGODBG(1, "Failed to add option to group \"%s\"\n", grpname);
			goto fail;
		}
	}

	o->group_index = -1;

	// Start parsing the format string.
	_cargo_fmt_scanner_init(&s, fmt);
	_cargo_fmt_next_token(&s);

	// Get the first token.
	if (_cargo_fmt_token(&s) == '.')
	{
		CARGODBG(2, "Static\n");
		o->alloc = 0;
		_cargo_fmt_next_token(&s);
	}
	else
	{
		o->alloc = 1;
	}

	if (_cargo_fmt_token(&s) == '[')
	{
		CARGODBG(4, "   [\n");
		o->array = 1;
		_cargo_fmt_next_token(&s);
	}

	switch (_cargo_fmt_token(&s))
	{
		//
		// !!!WARNING!!!
		// Do not attempt to refactor and put these longer cases in
		// separate functions. Passing a va_list around is dangerous.
		// This works fine on Unix, but fails randomly on Windows!
		//

		// Same as string, but the target is internal
		// and will be passed to the user specified callback.
		case 'c':
		{
			CARGODBG(4, "Custom callback\n");

			if (!o->alloc)
			{
				CARGODBG(1, "WARNING! Static '.' is ignored for a custom "
							"callback the memory for the arguments is "
							"allocated internally.");
				o->alloc = 1;
			}

			o->type = CARGO_STRING;
			o->custom = va_arg(ap, cargo_custom_cb_t);
			o->custom_user = va_arg(ap, void *);

			// Internal target.
			o->target = (void **)&o->custom_target;
			o->target_count = &o->custom_target_count;

			o->lenstr = 0;
			o->nargs = 1;

			if (!o->custom)
			{
				CARGODBG(1, "Got NULL custom callback pointer\n");
				goto fail;
			}

			break;
		}
		case 's':
		{
			o->type = CARGO_STRING;
			o->target = (void *)va_arg(ap, char *);

			CARGODBG(4, "Read string\n");
			_cargo_fmt_next_token(&s);

			if (_cargo_fmt_token(&s) == '#')
			{
				o->lenstr = (size_t)va_arg(ap, int);
				CARGODBG(4, "String length: %lu\n", o->lenstr);

				if (o->alloc)
				{
					CARGODBG(1, "%s: WARNING! Usually restricting the size of a "
						"string using # is only done on static strings.\n"
						"    Are you sure you want this?\n",
						o->name[0]);
					CARGODBG(1, "      \"%s\"\n", s.start);
					CARGODBG(1, "       %*s\n", s.column, "^");
				}
			}
			else
			{
				// String size not fixed.
				o->lenstr = 0;
				_cargo_fmt_prev_token(&s);
			}

			break;
		}
		case 'i': o->type = CARGO_INT;    o->target = va_arg(ap, void *); break;
		case 'd': o->type = CARGO_DOUBLE; o->target = va_arg(ap, void *); break;
		case 'b': o->type = CARGO_BOOL;   o->target = va_arg(ap, void *); break;
		case 'u': o->type = CARGO_UINT;   o->target = va_arg(ap, void *); break;
		case 'f': o->type = CARGO_FLOAT;  o->target = va_arg(ap, void *); break;
		default:
		{
			CARGODBG(1, "  %s: Unknown format character '%c' at index %d\n",
					o->name[0], _cargo_fmt_token(&s), s.column);
			CARGODBG(1, "      \"%s\"\n", fmt);
			CARGODBG(1, "       %*s\n", s.column, "^");
			goto fail;
		}
	}

	if (o->array)
	{
		// Custom callbacks uses an internal target and count.
		// However we still return the count in a separate
		// user specified value for arrays.
		if (o->custom)
		{
			o->custom_user_count = va_arg(ap, size_t *);
		}
		else
		{
			o->target_count = va_arg(ap, size_t *);
			*o->target_count = 0;
		}

		_cargo_fmt_next_token(&s);

		if (_cargo_fmt_token(&s) != ']')
		{
			CARGODBG(1, "%s: Expected ']'\n", o->name[0]);
			CARGODBG(1, "      \"%s\"\n", fmt);
			CARGODBG(1, "        %*s\n", s.column, "^");
			goto fail;
		}

		_cargo_fmt_next_token(&s);

		switch (_cargo_fmt_token(&s))
		{
			case '*': o->nargs = CARGO_NARGS_ZERO_OR_MORE; break;
			case '+': o->nargs = CARGO_NARGS_ONE_OR_MORE;  break;
			case 'N': // Fall through. Python uses N so lets allow that...
			case '#': o->nargs = va_arg(ap, int); break;
			default:
			{
				CARGODBG(1, "  %s: Unknown format character '%c' at index %d\n",
						optname_list[0], _cargo_fmt_token(&s), s.column);
				CARGODBG(1, "      \"%s\"\n", fmt);
				CARGODBG(1, "       %*s\n", s.column, "^");
				goto fail;
			}
		}
	}
	else
	{
		// Non-array.

		if (o->type == CARGO_BOOL)
		{
			// BOOLs never have arguments.
			o->nargs = 0;
		}
		else
		{
			_cargo_fmt_next_token(&s);

			if (_cargo_fmt_token(&s) == '?')
			{
				o->nargs = CARGO_NARGS_ZERO_OR_ONE;
			}
			else
			{
				_cargo_fmt_prev_token(&s);
				o->nargs = 1;
			}
		}

		// Never allocate single values (unless it's a string).
		o->alloc = (o->type != CARGO_STRING) ? 0 : o->alloc;
	}

	if (!o->alloc && o->array && (o->nargs < 0))
	{
		CARGODBG(1, "  %s: Static list requires a fixed size (#)\n", o->name[0]);
		CARGODBG(1, "      \"%s\"\n", fmt);
		CARGODBG(1, "       %*s\n", s.column, "^");
		goto fail;
	}

	o->flags = flags;

	// Check if the option has a prefix
	// (if not it's positional).
	o->positional = !_cargo_is_prefix(ctx, o->name[0][0]);

	if (o->positional
		&& (o->nargs != CARGO_NARGS_ZERO_OR_MORE)
		&& (o->nargs != CARGO_NARGS_ZERO_OR_ONE))
	{
		CARGODBG(2, "Positional argument %s required by default\n", o->name[0]);
		o->flags |= CARGO_OPT_REQUIRED;
	}

	// By default "nargs" is the max number of arguments the option should parse.
	o->max_target_count = (o->nargs >= 0) ? o->nargs : (size_t)(-1);

	if (o->alloc)
	{
		*(o->target) = NULL;
	}

	if (o->target_count)
	{
		*(o->target_count) = 0;
	}

	if (_cargo_validate_option_args(ctx, o))
	{
		goto fail;
	}

	for (i = 1; i < optcount; i++)
	{
		if (cargo_add_alias(ctx, optname_list[0], optname_list[i]))
		{
			goto fail;
		}
	}

	CARGODBG(2, " Option %s:\n", optname);
	CARGODBG(2, "   max_target_count = %lu\n", o->max_target_count);
	CARGODBG(2, "   alloc = %d\n", o->alloc);
	CARGODBG(2, "   lenstr = %lu\n", o->lenstr);
	CARGODBG(2, "   nargs = %d\n", o->nargs);
	CARGODBG(2, "   positional = %d\n", o->positional);
	CARGODBG(2, "   array = %d\n", o->array);
	CARGODBG(2, "   \n");

	ret = 0;

fail:
	if (ret < 0)
	{
		if (o)
		{
			_cargo_option_destroy(o);
			ctx->opt_count--;
		}
	}

	if (grpname)
	{
		free(grpname);
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

int cargo_add_option_ex(cargo_t ctx, size_t flags, const char *optnames,
						const char *description, const char *fmt, ...)
{
	int ret;
	va_list ap;
	assert(ctx);

	va_start(ap, fmt);
	ret = cargo_add_optionv_ex(ctx, flags, optnames, description, fmt, ap);
	va_end(ap);

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

void cargo_free_commandline(char ***argv, int argc)
{
	size_t i;
	assert(argv);

	if (*argv)
	{
		for (i = 0; i < (size_t)argc; i++)
		{
			if ((*argv)[i]) free((*argv)[i]);
			(*argv)[i] = NULL;
		}

		free(*argv);
		*argv = NULL;
	}
}

char **cargo_split_commandline(const char *cmdline, int *argc)
{
	int i;
	char **argv = NULL;
	assert(argc);

	if (!cmdline)
	{
		return NULL;
	}

	// Posix.
	#ifndef _WIN32
	{
		wordexp_t p;

		// Note! This expands shell variables.
		if (wordexp(cmdline, &p, 0))
		{
			return NULL;
		}

		*argc = p.we_wordc;

		if (!(argv = calloc(*argc, sizeof(char *))))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		for (i = 0; i < p.we_wordc; i++)
		{
			if (!(argv[i] = strdup(p.we_wordv[i])))
			{
				CARGODBG(1, "Out of memory!\n");
				goto fail;
			}
		}

		wordfree(&p);

		return argv;
	fail:
		wordfree(&p);
	}
	#else // WIN32
	{
		wchar_t **wargs = NULL;
		size_t needed = 0;
		wchar_t *cmdlinew = NULL;
		size_t len = strlen(cmdline) + 1;

		if (!(cmdlinew = calloc(len, sizeof(wchar_t))))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		if (!MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlinew, len))
		{
			CARGODBG(1, "Failed to convert to unicode!\n");
			goto fail;
		}

		if (!(wargs = CommandLineToArgvW(cmdlinew, argc)))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		if (!(argv = calloc(*argc, sizeof(char *))))
		{
			CARGODBG(1, "Out of memory!\n");
			goto fail;
		}

		// Convert from wchar_t * to ANSI char *
		for (i = 0; i < *argc; i++)
		{
			// Get the size needed for the target buffer.
			// CP_ACP = Ansi Codepage.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
										NULL, 0, NULL, NULL);

			if (!(argv[i] = malloc(needed)))
			{
				CARGODBG(1, "Out of memory!\n");
				goto fail;
			}

			// Do the conversion.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
										argv[i], needed, NULL, NULL);
		}

		if (wargs) LocalFree(wargs);
		if (cmdlinew) free(cmdlinew);
		return argv;

	fail:
		if (wargs) LocalFree(wargs);
		if (cmdlinew) free(cmdlinew);
	}
	#endif // WIN32

	if (argv)
	{
		for (i = 0; i < *argc; i++)
		{
			if (argv[i])
			{
				free(argv[i]);
			}
		}

		free(argv);
	}

	return NULL;
}

const char *cargo_get_version()
{
	return CARGO_VERSION_STR;
}

// -----------------------------------------------------------------------------
// Tests.
// -----------------------------------------------------------------------------
// LCOV_EXCL_START
#ifdef CARGO_TEST

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
#define _TEST_START_EX(testname, flags) 				\
static const char *_MAKE_TEST_FUNC_NAME(testname)		\
{														\
	const char *msg = NULL;								\
	int ret = 0;										\
	cargo_t cargo;										\
														\
	if (cargo_init_ex(&cargo, "program", flags))		\
	{													\
		return "Failed to init cargo";					\
	}

#define _TEST_START(testname) _TEST_START_EX(testname, 0)

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

// For tests where we want to assert after
// cargo_destroy has been called.
#define _TEST_END_NODESTROY()	\
	return msg;					\
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
		usage = cargo_get_usage(cargo);
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
	usage = cargo_get_usage(cargo);
	cargo_assert(usage != NULL, "Got NULL usage");
	printf("-------------------------------------\n");
	printf("%s", usage);
	printf("-------------------------------------\n");
	cargo_assert(strstr(usage, "help") != NULL, "No help found by default");

	_TEST_CLEANUP();
	free(usage);
}
_TEST_END()

_TEST_START(TEST_autohelp_off)
{
	int i;
	char *usage = NULL;

	// Turn off auto_help (--help).
	cargo_set_auto_help(cargo, 0);
	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);

	usage = cargo_get_usage(cargo);
	cargo_assert(usage != NULL, "Got NULL usage");
	printf("-------------------------------------\n");
	printf("%s", usage);
	printf("-------------------------------------\n");
	cargo_assert(strstr(usage, "help") == NULL,
				"Help found when auto_help turned off");

	_TEST_CLEANUP();
	free(usage);
}
_TEST_END()

#define _ADD_TEST_USAGE_OPTIONS() 										\
do 																		\
{																		\
	int *k;																\
	size_t k_count;														\
	int i;																\
	float f;															\
	int b;																\
	ret |= cargo_add_option(cargo, "pos", "Positional arg", "[i]+",		\
							&k, &k_count);								\
	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);	\
	ret |= cargo_add_option(cargo, "--beta", "The alpha", "f", &f);		\
	ret |= cargo_add_option(cargo, "--crash -c", "The alpha", "b", &b);	\
	cargo_assert(ret == 0, "Failed to add options");					\
} while (0)

_TEST_START(TEST_get_usage)
{
	char *usage = NULL;

	_ADD_TEST_USAGE_OPTIONS();
	usage = cargo_get_usage(cargo);
	cargo_assert(usage != NULL, "Failed to get allocated usage");
	printf("%s\n", usage);
	printf("Cargo v%s\n", cargo_get_version());

	_TEST_CLEANUP();
	free(usage);
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
	
	char *in[] =
	{
		" abc def   ghi   ",
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
    ret = 0;

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

_TEST_START(TEST_parse_twice)
{
	// This test make sure the parser is reset between
	// different parse sessions.
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
		cargo_assert(name && !strcmp(name, "server"), "Expected name = \"server\"");

		// TODO: Remove this and make sure these are freed at cargo_parse instead
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

_TEST_START(TEST_parse_missing_value)
{
	int i;
	int j;
	char *args[] = { "program", "--alpha", "1", "--beta" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	ret = cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succesfully parsed missing value");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_missing_array_value)
{
	int i[3];
	size_t i_count;
	int j;
	char *args[] = { "program", "--beta", "2", "--alpha", "1", "2" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", ".[i]#", &i, &i_count, 3);
	ret = cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succesfully parsed missing value");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_missing_array_value_ensure_free)
{
	// Here we make sure allocated values get freed on a failed parse.
	int i[3];
	size_t i_count;
	int *j;
	size_t j_count;
	char *args[] = { "program", "--beta", "2", "3", "--alpha", "1", "2" };

	ret = cargo_add_option(cargo, "--alpha -a", "The alpha", ".[i]#", &i, &i_count, 3);
	ret = cargo_add_option(cargo, "--beta -b", "The beta", "[i]#", &j, &j_count, 2);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succesfully parsed missing value");
	cargo_assert(j == NULL, "Array non-NULL after failed parse");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_same_option_twice)
{
	int i;
	int j;
	char *args[] = { "program", "--alpha", "1", "--beta", "4", "--alpha", "2" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "i", &i);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	printf("--alpha == %d\n", i);
	cargo_assert(ret == 0, "Failed to parsed duplicate option without unique");
	cargo_assert(i == 2, "Expected --alpha to have value 2");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_same_option_twice_string)
{
	char *s = NULL;
	int i;
	char *args[] = { "program", "--alpha", "abc", "--beta", "4", "--alpha", "def" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "s", &s);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed to parsed duplicate option without unique");
	cargo_assert(s && !strcmp(s, "def"), "Expected --alpha to have value \"def\"");

	_TEST_CLEANUP();
	if (s) free(s);
}
_TEST_END()

_TEST_START(TEST_parse_same_option_twice_with_unique)
{
	int i;
	int j;
	char *args[] = { "program", "--alpha", "1", "--beta", "4", "--alpha", "2" };

	ret |= cargo_add_option_ex(cargo, CARGO_OPT_UNIQUE,
								"--alpha -a", "The alpha", "i", &i);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	printf("--alpha == %d\n", i);
	cargo_assert(ret != 0, "Succesfully parsed duplicate option");
	cargo_assert(i == 1, "Expected --alpha to have value 1");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_same_option_twice_string_with_unique)
{
	char *s = NULL;
	int i;
	char *args[] = { "program", "--alpha", "abc", "--beta", "4", "--alpha", "def" };

	ret |= cargo_add_option_ex(cargo, CARGO_OPT_UNIQUE,
							"--alpha -a", "The alpha", "s", &s);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	printf("--alpha = %s\n", s);
	cargo_assert(ret != 0, "Succesfully parsed duplicate option");
	cargo_assert(s == NULL, "Expected --alpha to have value NULL");

	_TEST_CLEANUP();
	if (s) free(s);
}
_TEST_END()

_TEST_START(TEST_highlight_args)
{
	char *args[] =
	{
		"program",
		"--alpha", "abc",
		"--beta", "def", "ghi",
		"--crazy", "banans"
	};
	int argc = sizeof(args) / sizeof(args[0]);

	printf("With color highlight & args:\n");
	printf("----------------------------\n");
	ret = cargo_fprint_args(stdout,
						argc,
						args,
						1, // Start index.
						0, // flags
						3, // Highlight count.
						1, "^"CARGO_COLOR_RED,
						3, "~"CARGO_COLOR_GREEN,
						4, "-");
	cargo_assert(ret == 0, "Failed call cargo_fprint_args");

	printf("With highlight & args:\n");
	printf("----------------------\n");
	ret = cargo_fprint_args(stdout,
						argc,
						args,
						1, // Start index.
						CARGO_FPRINT_NOCOLOR, // flags
						3, // Highlight count.
						1, "^"CARGO_COLOR_RED,
						3, "~"CARGO_COLOR_GREEN,
						4, "-");
	cargo_assert(ret == 0, "Failed call cargo_fprint_args");

	printf("With highlight & no args:\n");
	printf("-------------------------\n");
	ret = cargo_fprint_args(stdout,
						argc,
						args,
						1, // Start index.
						CARGO_FPRINT_NOARGS, // flags
						3, // Highlight count.
						1, "^"CARGO_COLOR_RED, 3, "~"CARGO_COLOR_GREEN, 4, "-");
	cargo_assert(ret == 0, "Failed call cargo_fprint_args");

	printf("With no highlight & args:\n");
	printf("-------------------------\n");
	ret = cargo_fprint_args(stdout,
						argc,
						args,
						1, // Start index.
						CARGO_FPRINT_NOHIGHLIGHT, // flags
						3, // Highlight count.
						1, "^"CARGO_COLOR_RED, 3, "~"CARGO_COLOR_GREEN, 4, "-");
	cargo_assert(ret == 0, "Failed call cargo_fprint_args");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_positional_argument)
{
	// Here we make sure allocated values get freed on a failed parse.
	int i;
	int j;
	char *args[] = { "program", "--beta", "123", "456" };

	ret |= cargo_add_option(cargo, "alpha", "The alpha", "i", &j);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	printf("alpha = %d\n", j);
	cargo_assert(ret == 0, "Failed to parse positional argument");
	cargo_assert(j == 456, "Expected \"alpha\" to have value 456");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_positional_array_argument)
{
	// Here we make sure allocated values get freed on a failed parse.
	size_t k;
	int i;
	int j[3];
	int j_expect[] = { 456, 789, 101112 };
	size_t j_count = 0;
	char *args[] = { "program", "--beta", "123", "456", "789", "101112" };

	ret |= cargo_add_option(cargo, "alpha", "The alpha", ".[i]#", &j, &j_count, 3);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	for (k = 0; k < j_count; k++)
	{
		printf("alpha = %d\n", j[k]);
	}
	cargo_assert(ret == 0, "Failed to parse positional argument");
	cargo_assert_array(j_count, 3, j, j_expect);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_multiple_positional_array_argument)
{
	size_t k;
	int i;
	int j[3];
	int j_expect[] = { 456, 789, 101112 };
	size_t j_count = 0;
	float m[3];
	float m_expect[] = { 4.3f, 2.3f, 50.34f };
	size_t m_count = 0;
	char *args[] =
	{
		"program",
		"--beta", "123",
		"456", "789", "101112",
		"4.3", "2.3", "50.34"
	};

	ret |= cargo_add_option(cargo, "alpha", "The alpha", ".[i]#",
							&j, &j_count, 3);
	ret |= cargo_add_option(cargo, "mad", "Mutual Assured Destruction", ".[f]#",
							&m, &m_count, 3);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);

	for (k = 0; k < j_count; k++)
	{
		printf("alpha = %d\n", j[k]);
	}

	for (k = 0; k < m_count; k++)
	{
		printf("mad = %f\n", m[k]);
	}

	cargo_assert(ret == 0, "Failed to parse positional argument");
	cargo_assert_array(j_count, 3, j, j_expect);
	cargo_assert_array(m_count, 3, m, m_expect);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_multiple_positional_array_argument2)
{
	// Shuffle the order of arguments
	size_t k;
	int i;
	int j[3];
	int j_expect[] = { 456, 789, 101112 };
	size_t j_count = 0;
	float m[3];
	float m_expect[] = { 4.3f, 2.3f, 50.34f };
	size_t m_count = 0;
	char *args[] =
	{
		"program",
		"456", "789", "101112",
		"--beta", "123",
		"4.3", "2.3", "50.34"
	};

	ret |= cargo_add_option(cargo, "alpha", "The alpha", ".[i]#",
							&j, &j_count, 3);
	ret |= cargo_add_option(cargo, "mad", "Mutual Assured Destruction", ".[f]#",
							&m, &m_count, 3);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);

	for (k = 0; k < j_count; k++)
	{
		printf("alpha = %d\n", j[k]);
	}

	for (k = 0; k < m_count; k++)
	{
		printf("mad = %f\n", m[k]);
	}

	cargo_assert(ret == 0, "Failed to parse positional argument");
	cargo_assert_array(j_count, 3, j, j_expect);
	cargo_assert(m, "m is NULL");
	cargo_assert_array(m_count, 3, m, m_expect);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_multiple_positional_array_argument3)
{
	// Test with an allocated array of 1 or more positional arguments.
	size_t k;
	int i;
	int j[3];
	int j_expect[] = { 456, 789, 101112 };
	size_t j_count = 0;
	float *m = NULL;
	float m_expect[] = { 4.3f, 2.3f, 50.34f, 0.99f };
	size_t m_count = 0;
	char *args[] =
	{
		"program",
		"456", "789", "101112",
		"--beta", "123",
		"4.3", "2.3", "50.34", "0.99"
	};

	ret |= cargo_add_option(cargo, "alpha", "The alpha", ".[i]#",
							&j, &j_count, 3);
	ret |= cargo_add_option(cargo, "mad", "Mutual Assured Destruction", "[f]+",
							&m, &m_count);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);

	printf("j_count = %lu\n", j_count);
	for (k = 0; k < j_count; k++)
	{
		printf("alpha = %d\n", j[k]);
	}

	printf("m_count = %lu\n", m_count);
	for (k = 0; k < m_count; k++)
	{
		printf("mad = %f\n", m[k]);
	}

	cargo_assert(ret == 0, "Failed to parse positional argument");
	cargo_assert_array(j_count, 3, j, j_expect);
	cargo_assert_array(m_count, 4, m, m_expect);

	_TEST_CLEANUP();
	if (m) free(m);
}
_TEST_END()

_TEST_START_EX(TEST_autoclean_flag, CARGO_AUTOCLEAN)
{
	char *s = NULL;
	char *args[] = { "program", "--alpha", "abc" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "s", &s);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed to parse valid option");
	cargo_assert(s && !strcmp(s, "abc"), "Expected --alpha to have value \"abc\"");

	_TEST_CLEANUP();
	// Note! We're not using the normal _TEST_END here so we can assert after
	// the cargo_destroy call. Important not to use cargo_assert after
	// _TEST_CLEANUP. Since it will result in an infinite loop!
	cargo_destroy(&cargo);
	if (s != NULL) return "Expected \"s\" to be freed and NULL";
}
_TEST_END_NODESTROY()

_TEST_START_EX(TEST_autoclean_flag_off, 0)
{
	char *s = NULL;
	char *args[] = { "program", "--alpha", "abc" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "s", &s);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed to parse valid option");
	cargo_assert(s && !strcmp(s, "abc"), "Expected --alpha to have value \"abc\"");

	_TEST_CLEANUP();
	cargo_destroy(&cargo);
	if (s == NULL) return "Expected \"s\" to be non-NULL";
	free(s);
}
_TEST_END_NODESTROY()

_TEST_START(TEST_parse_zero_or_more_with_args)
{
	int *i;
	int i_expect[] = { 1, 2 };
	size_t i_count;
	int j;
	char *args[] = { "program", "--beta", "2", "--alpha", "1", "2" };

	cargo_get_flags(cargo);
	cargo_set_flags(cargo, CARGO_AUTOCLEAN);

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "[i]*", &i, &i_count);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Parse failed");
	cargo_assert_array(i_count, 2, i, i_expect);

	_TEST_CLEANUP();
	// Auto cleanup enabled.
}
_TEST_END()

_TEST_START(TEST_parse_zero_or_more_without_args)
{
	int *i;
	size_t i_count;
	int j;
	char *args[] = { "program", "--beta", "2", "--alpha" };

	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "[i]*", &i, &i_count);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &j);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Parse failed");
	cargo_assert(i == NULL, "Expected i to be NULL");
	cargo_assert(i_count == 0, "Expected i count to be 0");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_parse_zero_or_more_with_positional)
{
	int *pos;
	int pos_expect[] = { 1, 2 };
	size_t pos_count;
	int *i;
	int i_expect[] = { 3, 4 };
	size_t i_count;
	int *j;
	int j_expect[] = { 5, 6 };
	size_t j_count;
	char *args[] = { "program", "1", "2", "--alpha", "3", "4", "--beta", "5", "6" };

	cargo_get_flags(cargo);
	cargo_set_flags(cargo, CARGO_AUTOCLEAN);

	ret |= cargo_add_option(cargo, "pos", "The positional", "[i]*", &pos, &pos_count);
	ret |= cargo_add_option(cargo, "--alpha -a", "The alpha", "[i]*", &i, &i_count);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "[i]*", &j, &j_count);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Parse failed");
	cargo_assert_array(pos_count, 2, pos, pos_expect);
	cargo_assert_array(i_count, 2, i, i_expect);
	cargo_assert_array(j_count, 2, j, j_expect);

	_TEST_CLEANUP();
	// Auto cleanup enabled.
}
_TEST_END()

_TEST_START(TEST_required_option_missing)
{
	int i;
	int j;
	char *args[] = { "program", "--beta", "123", "456" };

	ret |= cargo_add_option_ex(cargo, CARGO_OPT_REQUIRED,
							"--alpha", "The alpha", "i", &j);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succeeded with missing required option");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_required_option)
{
	int i;
	int j;
	char *args[] = { "program", "--beta", "123", "456", "--alpha", "789" };

	ret |= cargo_add_option_ex(cargo, CARGO_OPT_REQUIRED,
							"--alpha", "The alpha", "i", &j);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed with existing required option");
	cargo_assert(j == 789, "Expected j == 789");

	_TEST_CLEANUP();
}
_TEST_END()

typedef struct _test_data_s
{
	int width;
	int height;
} _test_data_t;

static int _test_cb(cargo_t ctx, void *user, const char *optname,
					int argc, char **argv)
{
	assert(ctx);
	assert(user);

	_test_data_t *u = (_test_data_t *)user;
	memset(u, 0, sizeof(_test_data_t));

	if (argc > 0)
	{
		if (sscanf(argv[0], "%dx%d", &u->width, &u->height) != 2)
		{
			return -1;
		}

		return 1; // We ate 1 argument.
	}

	return -1;
}

_TEST_START(TEST_custom_callback)
{
	_test_data_t data;
	char *args[] = { "program", "--alpha", "128x64" };

	cargo_set_flags(cargo, CARGO_AUTOCLEAN);

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "c", _test_cb, &data);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	printf("%dx%d\n", data.width, data.height);
	cargo_assert(data.width == 128, "Width expected to be 128");
	cargo_assert(data.height == 64, "Height expected to be 128");

	_TEST_CLEANUP();
}
_TEST_END()

static int _test_cb_fixed_array(cargo_t ctx, void *user, const char *optname,
								int argc, char **argv)
{
	int i;
	assert(ctx);
	assert(user);

	_test_data_t *u = (_test_data_t *)user;
	memset(u, 0, sizeof(_test_data_t));

	for (i = 0; i < argc; i++)
	{
		if (sscanf(argv[i], "%dx%d", &u[i].width, &u[i].height) != 2)
		{
			return -1;
		}
	}

	return i;
}

_TEST_START(TEST_custom_callback_fixed_array)
{
	size_t i;
	#define DATA_COUNT 5
	_test_data_t data[DATA_COUNT];
	size_t data_count = 0;
	_test_data_t data_expect[] =
	{
		{ 128, 64 },
		{ 32, 16 },
		{ 24, 32 },
		{ 32, 32 },
		{ 24, 24 }
	};
	char *args[] = { "program", "--alpha", "128x64", "32x16", "24x32", "32x32", "24x24" };

	cargo_set_flags(cargo, CARGO_AUTOCLEAN);

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "[c]#",
							_test_cb_fixed_array, &data, &data_count, DATA_COUNT);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);

	cargo_assert(data_count == DATA_COUNT, "Expected data_count == DATA_COUNT");

	for (i = 0; i < data_count; i++)
	{
		printf("%dx%d == %dx%d\n",
			data[i].width, data[i].height,
			data_expect[i].width, data_expect[i].height);
		cargo_assert(data[i].width == data_expect[i].width, "Unexpected width");
		cargo_assert(data[i].height == data_expect[i].height, "Unexpected height");
	}

	_TEST_CLEANUP();
}
_TEST_END()

static int _test_cb_array(cargo_t ctx, void *user, const char *optname,
								int argc, char **argv)
{
	int i;
	assert(ctx);
	assert(user);
	_test_data_t **u = (_test_data_t **)user;
	_test_data_t *data;

	if (!(*u = calloc(argc, sizeof(_test_data_t))))
	{
		return -1;
	}

	data = *u;

	for (i = 0; i < argc; i++)
	{
		if (sscanf(argv[i], "%dx%d", &data[i].width, &data[i].height) != 2)
		{
			return -1;
		}
	}

	return i; // How many arguments we ate.
}

_TEST_START(TEST_custom_callback_array)
{
	size_t i;
	_test_data_t *data = NULL;
	size_t data_count = 0;
	_test_data_t data_expect[] =
	{
		{ 128, 64 },
		{ 32, 16 },
		{ 24, 32 },
		{ 32, 32 },
		{ 24, 24 }
	};
	char *args[] = { "program", "--alpha", "128x64", "32x16", "24x32", "32x32", "24x24" };

	cargo_set_flags(cargo, CARGO_AUTOCLEAN);

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "[c]*",
							_test_cb_array, &data, &data_count, DATA_COUNT);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);

	cargo_assert(data_count == 5, "Expected data_count == 5");

	for (i = 0; i < data_count; i++)
	{
		printf("%dx%d == %dx%d\n",
			data[i].width, data[i].height,
			data_expect[i].width, data_expect[i].height);
		cargo_assert(data[i].width == data_expect[i].width, "Unexpected width");
		cargo_assert(data[i].height == data_expect[i].height, "Unexpected height");
	}

	_TEST_CLEANUP();
	if (data) free(data);
}
_TEST_END()

_TEST_START(TEST_zero_or_more_with_arg)
{
	int i = 0;
	int j = 0;
	char *args[] = { "program", "--alpha", "789", "--beta", "123", "456" };

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "i?", &j);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed zero or more args parse");
	cargo_assert(j == 789, "Expected j == 789");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_zero_or_more_without_arg)
{
	int i = 0;
	int j = 5;
	char *args[] = { "program", "--alpha", "--beta", "123", "456" };

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "i?", &j);
	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed zero or more args parse");
	cargo_assert(j == 5, "Expected j == 5");

	_TEST_CLEANUP();
}
_TEST_END()

#define LOREM_IPSUM															\
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do"		\
	" eiusmod tempor incididunt ut labore et dolore magna aliqua. "			\
	"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "	\
	"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "	\
	"reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "	\
	"pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "	\
	"culpa qui officia deserunt mollit anim id est laborum."

_TEST_START(TEST_group)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char *args[] = { "program", "--alpha", "--beta", "123", "456" };

	ret = cargo_add_group(cargo, 0, "group1", "The Group 1", "This group is 1st");
	cargo_assert(ret == 0, "Failed to add group");

	// Add options to the group using inline method.
	ret |= cargo_add_option(cargo, "   <group1> --alpha", "The alpha", "i?", &j);
	ret |= cargo_add_option(cargo, "<group1> --beta -b", LOREM_IPSUM, "i", &i);

	// Try using the API to add the option to the group.
	ret |= cargo_add_option(cargo, "--centauri", "The alpha centauri", "i?", &k);
	ret |= cargo_group_add_option(cargo, "group1", "--centauri");
	cargo_assert(ret == 0, "Failed to add options");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed zero or more args parse");

	cargo_print_usage(cargo);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_many_groups)
{
	size_t i = 0;
	size_t j = 0;
	#define GROUP_COUNT 20
	#define GROUP_OPT_COUNT 20
	int vals[GROUP_COUNT][GROUP_OPT_COUNT];
	char grpname[256];
	char title[256];
	char optname[256];
	char *args[] = { "program", "--optg01o01", "5", "--optg02o02", "123" };

	for (i = 0; i < GROUP_COUNT; i++)
	{
		cargo_snprintf(grpname, sizeof(grpname), "group%d", i+1);
		cargo_snprintf(title, sizeof(title), "The Group%d", i+1);
		ret |= cargo_add_group(cargo, 0, grpname, title, "Group");
		cargo_assert(ret == 0, "Failed to add group");

		for (j = 0; j < GROUP_OPT_COUNT; j++)
		{
			cargo_snprintf(optname, sizeof(optname), "--optg%02do%02d", i+1, j+1);
			ret |= cargo_add_option(cargo, optname, LOREM_IPSUM, "i", &vals[i][j]);
			ret |= cargo_group_add_option(cargo, grpname, optname);
			cargo_assert(ret == 0, "Failed to add option");
		}	
	}

	cargo_assert(ret == 0, "Failed to add groups");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed zero or more args parse");
	cargo_assert(vals[0][0] == 5, "Expected --optg01o01 to be 5");
	cargo_assert(vals[1][1] == 123, "Expected --optg02o02 to be 123");

	cargo_print_usage(cargo);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_mutex_group_guard)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char *args[] = { "program", "--alpha", "--beta", "123", "456" };

	ret = cargo_add_mutex_group(cargo, 0, "mutex_group1");
	cargo_assert(ret == 0, "Failed to add mutex group");

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "i?", &j);
	ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--alpha");

	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--beta");

	ret |= cargo_add_option(cargo, "--centauri", "The centauri", "i?", &k);
	ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--centauri");
	cargo_assert(ret == 0, "Failed to add options");

	// We parse args with 2 members of the mutex group.
	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0, "Succesfully parsed mutex group with 2 group members");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_mutex_group_require_one)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char *args[] = { "program", "--centauri", "5" };

	ret = cargo_add_mutex_group(cargo,
								CARGO_MUTEXGRP_ONE_REQUIRED,
								"mutex_group1");
	cargo_assert(ret == 0, "Failed to add mutex group");

	ret |= cargo_add_option(cargo, "--alpha", "The alpha", "i", &j);
	ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--alpha");

	ret |= cargo_add_option(cargo, "--beta -b", "The beta", "i", &i);
	ret |= cargo_mutex_group_add_option(cargo, "mutex_group1", "--beta");

	// Don't add this to te mutex group.
	ret |= cargo_add_option(cargo, "--centauri", "The centauri", "i?", &k);
	cargo_assert(ret == 0, "Failed to add options");

	// We parse args with no members of the mutex group.
	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret != 0,
		"Succesfully parsed mutex group with no mutex member when 1 required");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_cargo_split_commandline)
{
	size_t i;
	const char *cmd = "abc def \"ghi jkl\"";
	char *argv_expect[] =
	{
		"abc", "def", "ghi jkl"
	};
	char **argv = NULL;
	int argc = 0;
	ret = 0;

	argv = cargo_split_commandline(cmd, &argc);
	cargo_assert(argv != NULL, "Got NULL argv");
	cargo_assert_str_array((size_t)argc, 3, argv, argv_expect);

	_TEST_CLEANUP();
	cargo_free_commandline(&argv, argc);
}
_TEST_END()

static const char *_cargo_test_verify_usage_length(char *usage, int width)
{
	char *ret = NULL;
	char **lines = NULL;
	size_t line_count = 0;
	size_t len = 0;
	size_t i = 0;
	assert(usage);
	assert(width);

	printf("%s\n", usage);

	if (!(lines = _cargo_split(usage, "\n", &line_count)))
	{
		return "Failed to split usage";
	}

	printf("Line count: %lu\n", line_count);

	for (i = 0; i < line_count; i++)
	{
		len = strlen(lines[i]);
		printf("%02lu Len: %02lu:%s\n", i, len, lines[i]);

		if ((int)len > width)
		{
			ret = "Got usage line longer than specified";
			break;
		}
	}

	_cargo_free_str_list(&lines, &line_count);
	return ret;
}

_TEST_START(TEST_cargo_set_max_width)
{
	int i;
	int j;
	const char *err = NULL;
	char *usage = NULL;
	char *args[] = { "program", "--alpha", "789", "--beta", "123" };

	ret |= cargo_add_option(cargo, "--alpha", LOREM_IPSUM, "i", &j);
	ret |= cargo_add_option(cargo, "--beta -b", LOREM_IPSUM, "i", &i);
	cargo_assert(ret == 0, "Failed to add options");

	cargo_set_max_width(cargo, 40);
	usage = cargo_get_usage(cargo);
	cargo_assert(usage != NULL, "Got NULL usage on width 40");
	err = _cargo_test_verify_usage_length(usage, 40);
	cargo_assert(err == NULL, err);
	free(usage);
	usage = NULL;

	// Set a size bigger than CARGO_MAX_MAX_WIDTH.
	cargo_set_max_width(cargo, CARGO_MAX_MAX_WIDTH * 2);
	usage = cargo_get_usage(cargo);
	cargo_assert(usage != NULL, "Got NULL usage on width CARGO_MAX_MAX_WIDTH * 2");
	err = _cargo_test_verify_usage_length(usage, CARGO_MAX_MAX_WIDTH);
	cargo_assert(err == NULL, err);
	free(usage);
	usage = NULL;

	_TEST_CLEANUP();
	if (usage) free(usage);
}
_TEST_END()

_TEST_START(TEST_cargo_snprintf)
{
	char buf[32];
	ret = cargo_snprintf(buf, 0, "%d", 123);
	printf("%d\n", ret);
	cargo_assert(ret == 0, "Expected ret == 0 when given 0 buflen");

	// Let memcheckers make sure this doesn't write past end of buffer
	ret = cargo_snprintf(buf, sizeof(buf), "%s", LOREM_IPSUM);

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_cargo_set_prefix)
{
	int i;
	int j;
	int k;
	char *args[] = { "program", "--alpha", "789", "++beta", "123" };

	cargo_set_prefix(cargo, "+");

	ret = cargo_add_option(cargo, "--alpha", LOREM_IPSUM, "i", &j);
	cargo_assert(ret != 0, "Succesfully added argument with invalid prefix");

	ret = cargo_add_option(cargo, "centauri c", LOREM_IPSUM, "i", &k);
	cargo_assert(ret != 0, "Expected positional argument to reject alias");

	ret = cargo_add_option(cargo, "++beta +b", LOREM_IPSUM, "i", &i);
	cargo_assert(ret == 0, "Failed to add option with valid prefix '+'");

	ret = cargo_parse(cargo, 1, sizeof(args) / sizeof(args[0]), args);
	cargo_assert(ret == 0, "Failed to parse with custom prefix");
	cargo_assert(i == 123, "Expected ++beta to have value 123");

	_TEST_CLEANUP();
}
_TEST_END()

_TEST_START(TEST_cargo_aapendf)
{
	char *s = NULL;
	size_t lbefore = 0;
	size_t lorem_lens = (strlen(LOREM_IPSUM) * 3);
	cargo_astr_t astr;

	// This test is only valid if the test strings causes a realloc.
	assert(lorem_lens > CARGO_ASTR_DEFAULT_SIZE);

	memset(&astr, 0, sizeof(cargo_astr_t));
	astr.s = &s;

	ret = cargo_aappendf(&astr, "%s", "Some short string");
	cargo_assert(ret > 0, "Expected ret > 0");
	lbefore = astr.l;

	// Trigger realloc.
	cargo_aappendf(&astr, "%s%s%s", LOREM_IPSUM, LOREM_IPSUM, LOREM_IPSUM);
	cargo_assert(ret > 0, "Expected ret > 0");
	cargo_assert(astr.offset > lorem_lens, "Expected longer string");
	cargo_assert(astr.l > lbefore, "Expected realloc");

	_TEST_CLEANUP();
	if (s) free(s);
}
_TEST_END()

_TEST_START(TEST_cargo_get_fprint_args)
{
	int i = 0;
	int j = 0;
	char *argv[] = { "program", "first", "second", "--third", "123" };
	int argc = sizeof(argv) / sizeof(argv[0]);
	char *s = NULL;
	int start;
	cargo_highlight_t highlights[] =
	{
		{ 1, "#"CARGO_COLOR_YELLOW },
		{ 4, "="CARGO_COLOR_GREEN }
	};
	ret = 0;

	start = 0;
	printf("\nStart %d:\n", start);
	printf("----------\n");
	s = cargo_get_fprint_args(argc, argv,
							start,	// start.
							0,		// flags.
							3,		// highlight_count (how many follows).
							0, "^"CARGO_COLOR_RED,
							2 ,"~",
							4, "*"CARGO_COLOR_CYAN);
	cargo_assert(s != NULL, "Got NULL fprint string");
	printf("%s\n", s);
	for (i = start; i < argc; i++)
	{
		cargo_assert(strstr(s, argv[i]), "Expected to find argv params");
	}
	cargo_assert(strstr(s, "^"), "Expected ^ highlight");
	cargo_assert(strstr(s, CARGO_COLOR_RED), "Expected red color for ^");
	cargo_assert(strstr(s, "~"), "Expected ~ highlight");
	cargo_assert(strstr(s, "*"), "Expected * highlight");
	cargo_assert(strstr(s, CARGO_COLOR_CYAN), "Expected red color for *");
	free(s);
	s = NULL;

	// Other start index.
	start = 1;
	printf("\nStart %d:\n", start);
	printf("----------\n");
	s = cargo_get_fprint_args(argc, argv,
							start,	// start.
							0,		// flags.
							3,		// highlight_count (how many follows).
							0, "^"CARGO_COLOR_RED, // Should not be shown.
							2 ,"~",
							4, "*"CARGO_COLOR_CYAN);
	cargo_assert(s != NULL, "Got NULL fprint string");
	printf("%s\n", s);
	for (i = start; i < argc; i++)
	{
		cargo_assert(strstr(s, argv[i]), "Expected to find argv params");
	}
	cargo_assert(!strstr(s, "^"), "Expected NO ^ highlight");
	cargo_assert(!strstr(s, CARGO_COLOR_RED), "Expected NO red color for ^");
	cargo_assert(strstr(s, "~"), "Expected ~ highlight");
	cargo_assert(strstr(s, "*"), "Expected red color for *");
	cargo_assert(strstr(s, CARGO_COLOR_CYAN), "Expected red color for *");
	free(s);
	s = NULL;

	// Pass a list instead of var args.
	for (i = 0; i < argc+1; i++)
	{
		start = i;
		printf("\nList start %d:\n", start);
		printf("-------------\n");
		s = cargo_get_fprintl_args(argc, argv,
								start,	// start.
								0,		// flags
								sizeof(highlights) / sizeof(highlights[0]),
								highlights);
		printf("%s\n", s);
		for (j = start; j < argc; j++)
		{
			cargo_assert(strstr(s, argv[i]), "Expected to find argv params");
		}

		if (start <= highlights[0].i)
		{
			cargo_assert(strstr(s, "#"), "Expected # highlight");
			cargo_assert(strstr(s, CARGO_COLOR_YELLOW), "Expected yellow color for #");
		}
		else
		{
			cargo_assert(!strstr(s, "#"), "Expected NO # highlight");
			cargo_assert(!strstr(s, CARGO_COLOR_YELLOW), "Expected NO yellow color for #");
		}

		if (start <= highlights[1].i)
		{
			cargo_assert(strstr(s, "="), "Expected = highlight");
			cargo_assert(strstr(s, CARGO_COLOR_GREEN), "Expected red color for =");
		}
		else
		{
			cargo_assert(!strstr(s, "="), "Expected NO = highlight");
			cargo_assert(!strstr(s, CARGO_COLOR_GREEN), "Expected NO red color for =");
		}

		free(s);
		s = NULL;
	}

	ret = cargo_fprintl_args(stdout, argc, argv, 0, 0,
							sizeof(highlights) / sizeof(highlights[0]),
							highlights);
	cargo_assert(ret == 0, "Expected success");

	_TEST_CLEANUP();
	if (s) free(s);
}
_TEST_END()

_TEST_START(TEST_cargo_fprintf)
{
	char *s = NULL;
	cargo_fprintf(stdout, "%shej%s poop\n",
		CARGO_COLOR_YELLOW, CARGO_COLOR_RESET);

	cargo_asprintf(&s, "%shej%s\n",
		CARGO_COLOR_YELLOW, CARGO_COLOR_RESET);

	cargo_assert(s, "Got NULL string");

	_TEST_CLEANUP();
	free(s);
}
_TEST_END()

// TODO: Test cargo_get_fprint_args, cargo_get_fprintl_args
// TODO: Refactor cargo_get_usage
// TODO: Test giving add_option an invalid alias
// TODO: Test cargo_split_commandline with invalid command line
// TODO: Test autoclean with string of arrays
// TODO: Test parsing option --alpha --beta, where --alpha should have an argument.
// TODO: Add a help function to free argv.


//
// List of all test functions to run:
//
typedef const char *(* cargo_test_f)();

typedef struct cargo_test_s
{
	const char *name;
	cargo_test_f f;
	int success;
	int ran;
	const char *error;
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
	CARGO_ADD_TEST(TEST_misspelled_argument),
	CARGO_ADD_TEST(TEST_max_option_count),
	CARGO_ADD_TEST(TEST_add_duplicate_option),
	CARGO_ADD_TEST(TEST_get_extra_args),
	CARGO_ADD_TEST(TEST_get_unknown_opts),
	CARGO_ADD_TEST(TEST_cargo_split),
	CARGO_ADD_TEST(TEST_parse_invalid_value),
	CARGO_ADD_TEST(TEST_parse_twice),
	CARGO_ADD_TEST(TEST_parse_missing_value),
	CARGO_ADD_TEST(TEST_parse_missing_array_value),
	CARGO_ADD_TEST(TEST_parse_missing_array_value_ensure_free),
	CARGO_ADD_TEST(TEST_parse_same_option_twice),
	CARGO_ADD_TEST(TEST_parse_same_option_twice_string),
	CARGO_ADD_TEST(TEST_parse_same_option_twice_with_unique),
	CARGO_ADD_TEST(TEST_parse_same_option_twice_string_with_unique),
	CARGO_ADD_TEST(TEST_highlight_args),
	CARGO_ADD_TEST(TEST_positional_argument),
	CARGO_ADD_TEST(TEST_positional_array_argument),
	CARGO_ADD_TEST(TEST_multiple_positional_array_argument),
	CARGO_ADD_TEST(TEST_multiple_positional_array_argument2),
	CARGO_ADD_TEST(TEST_multiple_positional_array_argument3),
	CARGO_ADD_TEST(TEST_autoclean_flag),
	CARGO_ADD_TEST(TEST_autoclean_flag_off),
	CARGO_ADD_TEST(TEST_parse_zero_or_more_with_args),
	CARGO_ADD_TEST(TEST_parse_zero_or_more_without_args),
	CARGO_ADD_TEST(TEST_parse_zero_or_more_with_positional),
	CARGO_ADD_TEST(TEST_required_option_missing),
	CARGO_ADD_TEST(TEST_required_option),
	CARGO_ADD_TEST(TEST_custom_callback),
	CARGO_ADD_TEST(TEST_custom_callback_fixed_array),
	CARGO_ADD_TEST(TEST_custom_callback_array),
	CARGO_ADD_TEST(TEST_zero_or_more_with_arg),
	CARGO_ADD_TEST(TEST_zero_or_more_without_arg),
	CARGO_ADD_TEST(TEST_group),
	CARGO_ADD_TEST(TEST_many_groups),
	CARGO_ADD_TEST(TEST_mutex_group_guard),
	CARGO_ADD_TEST(TEST_mutex_group_require_one),
	CARGO_ADD_TEST(TEST_cargo_split_commandline),
	CARGO_ADD_TEST(TEST_cargo_set_max_width),
	CARGO_ADD_TEST(TEST_cargo_snprintf),
	CARGO_ADD_TEST(TEST_cargo_set_prefix),
	CARGO_ADD_TEST(TEST_cargo_aapendf),
	CARGO_ADD_TEST(TEST_cargo_get_fprint_args),
	CARGO_ADD_TEST(TEST_cargo_fprintf)
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

static int _tests_print_usage(const char *progname)
{
	_test_print_names();
	fprintf(stderr, "\nCargo v%s\n", cargo_get_version());
	fprintf(stderr, "Usage: %s [--shortlist] [test_num ...] [test_name ...]\n\n", progname);
	fprintf(stderr, "  --shortlist  Don't list test that were not run (must be first argument).\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  Use test_num = -1 or all tests\n");
	fprintf(stderr, "  Or you can specify the testname: TEST_...\n");
	fprintf(stderr, "  Return code for this usage message equals the number of available tests.\n");
	return CARGO_NUM_TESTS;
}

int main(int argc, char **argv)
{
	size_t i;
	int was_error = 0;
	int tests_to_run[CARGO_NUM_TESTS];
	size_t num_tests = 0;
	size_t success_count = 0;
	int all = 0;
	int test_index = 0;
	int shortlist = 0;
	int start = 1;
	cargo_test_t *t;

	memset(tests_to_run, 0, sizeof(tests_to_run));

	// Get test numbers to run from command line.
	if (argc >= 2)
	{
		if (!strcmp("--shortlist", argv[1]))
		{
			shortlist = 1;
			start++;
		}

		for (i = start; i < (size_t)argc; i++)
		{
			// First check if we were given a function name.
			if (!strncmp(argv[i], "TEST_", 5))
			{
				test_index = _test_find_test_index(argv[i]);

				if (test_index <= 0)
				{
					fprintf(stderr, "Unknown test specified: \"%s\"\n", argv[i]);
					return CARGO_NUM_TESTS;
				}
			}
			else
			{
				test_index = atoi(argv[i]);
				fprintf(stderr, "%d\n", test_index);

				if ((test_index == 0) || (test_index > (int)CARGO_NUM_TESTS))
				{
					fprintf(stderr, "Invalid test number %s\n", argv[i]);
					return CARGO_NUM_TESTS;
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
		return _tests_print_usage(argv[0]);
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

		fprintf(stderr, "\n%sStart Test %2d:%s - %s\n",
			CARGO_COLOR_CYAN, test_index + 1, CARGO_COLOR_RESET, t->name);

		fprintf(stderr, "%s", CARGO_COLOR_DARK_GRAY);
		t->error = t->f();
		fprintf(stderr, "%s", CARGO_COLOR_RESET);

		fprintf(stderr, "%sEnd Test %2d:%s ",
			CARGO_COLOR_CYAN, test_index + 1, CARGO_COLOR_RESET);

		if (t->error)
		{
			fprintf(stderr, "[%sFAIL%s] %s\n",
				CARGO_COLOR_RED, CARGO_COLOR_RESET, t->error);
			was_error++;
		}
		else
		{
			fprintf(stderr, "[%sSUCCESS%s]\n",
				CARGO_COLOR_GREEN, CARGO_COLOR_RESET);

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
			if (!shortlist)
			{
				printf(" [%sNOT RUN%s] %2lu: %s\n",
					CARGO_COLOR_DARK_GRAY, CARGO_COLOR_RESET, (i + 1), tests[i].name);
			}

			continue;
		}

		if (tests[i].success)
		{
			printf(" [%sSUCCESS%s] %2lu: %s\n",
				CARGO_COLOR_GREEN, CARGO_COLOR_RESET, (i + 1), tests[i].name);
		}
		else
		{
			fprintf(stderr, " [%sFAILED%s]  %2lu: %s - %s\n",
				CARGO_COLOR_RED, CARGO_COLOR_RESET,
				(i + 1), tests[i].name, tests[i].error);
		}
	}

	if (was_error)
	{
		fprintf(stderr, "\n[[%sFAIL%s]] ", CARGO_COLOR_RED, CARGO_COLOR_RESET);
	}
	else
	{
		printf("\n[[%sSUCCESS%s]] ", CARGO_COLOR_GREEN, CARGO_COLOR_RESET);
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
	memset(types, 0, sizeof(types));

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
	int ret = 0;
	cargo_t cargo;

	cargo_init(&cargo, argv[0]);

	// TODO: Make a real example.

	cargo_print_usage(cargo);
	cargo_destroy(&cargo);
	return ret;
}

#endif // CARGO_EXAMPLE

// LCOV_EXCL_END
