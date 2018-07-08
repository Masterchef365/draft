#include <util.h>

int get_first_i2c_fd () {
	char dir_name_buf[64];
	int fd;
	for (int i = 0; i < MAX_I2C_DEV_SEARCH; i++) {
		snprintf(dir_name_buf, 64, "/dev/i2c-%i", i);
		if ((fd = open(dir_name_buf, O_RDWR)) != -1) return fd;
	}
	return fd;
}

FILE* file_ptr_config_file (char* dir, char* name, char* modes) {
	char config_dir[256];
	FILE* file_ptr;
	snprintf(config_dir, 256, "%s/%s.cfg", dir, name);
	if (!(file_ptr = fopen(config_dir, modes))) {
		fprintf(stderr, "Failure opening %s; %s\n", config_dir, strerror(errno));
		return NULL;
	} else {
		return file_ptr;
	}
}

