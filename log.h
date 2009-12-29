#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

struct logger {
	char *name;
	int verbose;
};

enum log_level {
	ERR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG
};

void log_printf(struct logger *logger, enum log_level level, const char *format, ...);

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#endif
