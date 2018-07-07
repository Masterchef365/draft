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
#include <server_config.h>

#include <motor_protocol.h>

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	MotorConfigManager gantry_config =
		motor_config_manager_load_from_directory(argv[1], "gantry.cfg");

	ServerConfigManager server_config =
		server_config_manager_load_from_directory(argv[1], "server.cfg");

	motor_write_config(stdout, &gantry_config.config);
	server_write_config(stdout, &server_config.config);

	/*
	int device;
	char *filename = "/dev/i2c-1";
	if ((device = open(filename, O_RDWR)) < 0) {
		perror("Failed to open the i2c bus");
		exit(1);
	}
	int addr = 0x04;
	if (ioctl(device, I2C_SLAVE, addr) < 0) {
		perror("Failed to acquire bus access and/or talk to slave");
		exit(1);
	}
	*/

	motor_send_var(0, set_position, 0.0);

	//ClientHandler handler = create_client_handler(
	//server_config.config.portno, server_config.config.timeout_ms);

	//close_server(&handler);
	motor_config_manager_close_and_rewrite(&gantry_config);
	server_config_manager_close_and_rewrite(&server_config);
}
