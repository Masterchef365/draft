#include <stdio.h>
#include <stdlib.h>
#include <motion_server.h>

const char* debug_i2c_flag_str = "-debug_i2c"; /* Flag string to display and match against */

int main(int argc, char** argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir> %s\n", argv[0], debug_i2c_flag_str);
		exit(EXIT_FAILURE);
	}

	/* Debug I2C? */
	int debug_i2c = 0;
	if (argc == 3 && strcmp(argv[2], debug_i2c_flag_str) == 0) {
		debug_i2c = 1;
		inform_log(log_warn, "Running in I2C debug mode. Remove %s to disable.", debug_i2c_flag_str);
	}

	MotionServer server;
	motion_server_init(&server, argv[1], debug_i2c);
	inform_log(log_info, "Server init finished");
	while (motion_server_loop(&server));
	motion_server_destruct(&server);

	/* Main loop */
	inform_log(log_info, "Server loop ended, exiting gracefully.");

}
