#include <util.h>

unsigned int str_hash(char* str) {
	unsigned int hash = 7890;
	int c;
	while ((c = *str++)) hash += (hash << 5) + c;
	return hash;
}

