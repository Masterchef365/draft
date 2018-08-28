#ifndef PRETTY_LOG_H
#define PRETTY_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>

enum WarnLevel {
	log_info,
	log_warn,
	log_fail,
};

void inform_log(enum WarnLevel level, char* fmt, ...);
#endif
