#include "log.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

void log_printf(struct logger *logger, enum log_level level, const char *format, ...)
{
	char p[1024];
	va_list ap;
	va_start(ap, format);
	(void)vsnprintf(p, 1024, format, ap);
	va_end(ap);
	fprintf(stderr, "%s, %s", logger->name, p);
}
