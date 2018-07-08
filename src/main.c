#include <stdio.h>
#include <stdlib.h>
#include <motors.h>
#include <util.h>
#include <command_line_logic.h>

int main(int argc, char **argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char* config_dir = argv[1];

	int i2c_bus_fd = -1;
#if !DEBUG_I2C
	if ((i2c_bus_fd = get_first_i2c_fd()) == -1) {
		fprintf(stderr, "Failed to find a suitible i2c bus in /dev/. Exiting.\n");
		exit(EXIT_FAILURE);
	}
#endif

	MotorArray array;
	if (!motor_array_from_config_dir(i2c_bus_fd, config_dir, &array)) {
		fprintf(stderr, "Failure opening one or more config files. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < motor_array_length(); i++) {
		Motor* motor = &((Motor*)&array)[i];
		printf("=== %s === \n", motor->name);
		motor_write_config(stdout, &motor->config); 
	}

	putchar('\n');
	char buf[256];
	do {
		printf("> ");
		fgets(buf, 256, stdin);
		buf[strlen(buf) - 1] = '\0';
	} while (command_line_logic(STDOUT_FILENO, buf, &array));

	motor_array_rewrite_config_dir(config_dir, &array);	
}
