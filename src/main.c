#include <client_handler.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <motor_config.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	MotorConfigManager gantry_config = 
		motor_config_manager_load_from_directory(argv[1], "gantry.cfg");

	motor_write_config(stdout, &gantry_config.config);

	motor_config_manager_close_and_rewrite(&gantry_config);
}
