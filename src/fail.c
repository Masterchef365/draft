#include <fail.h>

void fail(char* where) {
	fprintf(stderr, "Error %s: %s\n", where, strerror(errno));
	exit(EXIT_FAILURE);
}
