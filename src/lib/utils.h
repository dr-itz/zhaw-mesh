#ifndef NET_UTILS_H
#define NET_UTILS_H

/**
 * Utility functions
 *
 * Written by Daniel Ritz
 */

#include <stdio.h>
#include <sys/time.h>

/**
 * checks the return value for error, reports the error
 * @param ret the return value of some function to check
 * @return true value if an error occured, 0 otherwise
 */
int _check_error(const char *fct, const char *file, int line, int ret);
#define check_error(ret) \
	_check_error(__func__, __FILE__, __LINE__, ret)

extern int debug;

/**
 * enabled/disables verbose message printing
 * @param ena bool to enable/disable messages
 */
void set_debug(int ena);

/**
 * set the debug message prefix
 * @param pf the prefix. this is strdup()d
 */
void debug_prefix(char *pf);

/**
 * verbose messages printf() wrapper
 * only prints messages in verbose mode
 */
void _dbg(const char *fmt, ...);
#define dbg(...)			\
	do {			 		\
		if (debug)			\
			_dbg(__VA_ARGS__);	\
	} while (0)


typedef unsigned long long mstime_t;

/**
 * @return the milliseconds part of gettimeofday()
 */
static inline mstime_t time_current()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 + t.tv_usec / 1000;
}

#endif
