#ifndef UTIL_H
#define UTIL_H

#define MAX_I2C_DEV_SEARCH 20

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Find an i2c device file and return the file descriptor. 
 * Searches /dev/i2c-* from 0 to MAX_I2C_DEV_SEARCH. */
int get_first_i2c_fd ();

/* Format a config file name and open 
 * a FILE pointer to the resulting file */
FILE* file_ptr_config_file (char* dir, char* name, char* modes);

/* Convert a string into a unique hash */
unsigned int str_hash(char* str);

#endif
