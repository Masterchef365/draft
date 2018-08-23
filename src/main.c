#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <server_config.h>
#include <sys/types.h>
#include <dirent.h>

int get_first_i2c_fd ();
const char* debug_i2c_flag_str = "-debug_i2c";

int main(int argc, char** argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir> %s\n", argv[0], debug_i2c_flag_str);
		exit(EXIT_FAILURE);
	}

	char* config_dir = argv[1];

	/* Debug I2C? */
	int debug_i2c = 0;
	if (argc == 3 && strcmp(argv[2], debug_i2c_flag_str) == 0) {
		debug_i2c = 1;
		fprintf(stderr, "Warning, running in I2C debug mode. Remove %s to disable. \n", debug_i2c_flag_str);
	}

	/* Warn user of nonexistant config dir and exit */
	if (closedir(opendir(config_dir)) == -1) {
		fprintf(stderr, "Config directory at %s does not exist\n", config_dir);
		exit(EXIT_FAILURE);
	}

	/* Load server config or default */
	ServerConfig server_config = {0};
	server_config.portno = 5060;
	server_config.timeout_ms = 10;
	server_load_or_write_defaults_from_dir(config_dir, "server.cfg", &server_config);

	/* If we aren't debugging, find and open an I2C device */
	int i2c_bus_fd = -1;
	if (!debug_i2c && (i2c_bus_fd = get_first_i2c_fd()) == -1) {
		fprintf(stderr, "Failed to find a suitible i2c bus in /dev/. Exiting.\n");
		exit(EXIT_FAILURE);
	}
}

/* Find an i2c device file and return the file descriptor. 
 * Searches /dev/i2c-* from 0 to MAX_I2C_DEV_SEARCH. */
int get_first_i2c_fd () {
	char dir_name_buf[64];
	int fd;
	for (int i = 0; i < 20; i++) {
		snprintf(dir_name_buf, 64, "/dev/i2c-%i", i);
		if ((fd = open(dir_name_buf, O_RDWR)) != -1) return fd;
	}
	return fd;
}
