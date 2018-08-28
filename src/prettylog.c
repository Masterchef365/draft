#include <prettylog.h>

static char* warn_level_names[] = {
	"INFO",
	"WARN",
	"FAIL",
};

static char* warn_level_color_codes[] = {
	"",
	"\e[1m\e[33m",
	"\e[1m\e[31m",
};

void inform_log(enum WarnLevel level, char* fmt, ...) {
	/* Clear the current line and carriage return in anticipation of a command
	 * line being present */
	printf("\e[1K\r");

	/* Count required length and create a buffer of sufficient size */
	size_t size = 1 + snprintf(NULL, 0, "\r%s[ %s ]\e[m %s\n", warn_level_color_codes[level], warn_level_names[level], fmt );
	char new_fmt[size];

	/* Write out the new format string */
	snprintf(new_fmt, size, "\r%s[ %s ]\e[m %s\n", warn_level_color_codes[level], warn_level_names[level], fmt );

	/* Write to console */
	va_list args;
	va_start(args, fmt);
	vprintf(new_fmt, args);
	va_end(args);

	/* Draw readline cursor */
	rl_forced_update_display();
}
