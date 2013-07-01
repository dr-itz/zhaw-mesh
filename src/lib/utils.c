/*
 * Generic utility functions, thread safe by a global lock where necessary
 *
 * Written by Daniel Ritz
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "utils.h"

int debug = 0;
static char *prefix;

pthread_mutex_t util_mutex = PTHREAD_MUTEX_INITIALIZER;

int _check_error(const char *fct, const char *file, int line, int ret)
{
	if (ret >= 0)
		return 0;

	pthread_mutex_lock(&util_mutex);
	fprintf(stderr, "Error %d in %s() at %s:%d, %s\n",
		-ret, fct, file, line, strerror(-ret));
	pthread_mutex_unlock(&util_mutex);

	return 1;
}

void _dbg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	pthread_mutex_lock(&util_mutex);
	if (prefix)
		fprintf(stderr, "%s", prefix);
	vfprintf(stderr, fmt, ap);
	pthread_mutex_unlock(&util_mutex);

	va_end(ap);
}

void set_debug(int ena)
{
	debug = ena;
}

void debug_prefix(char *pf)
{
	prefix = strdup(pf);
}
