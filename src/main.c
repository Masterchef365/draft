#include <stdio.h>
#include <stdlib.h>
#include <motors.h>

int main(int argc, char **argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char* config_dir = argv[1];

	MotorArray array;
	if (!motor_array_from_config_dir(0, config_dir, &array)) {
		fprintf(stderr, "Failure opening one or more config files. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < motor_array_length(); i++) {
		Motor* motor = &((Motor*)&array)[i];
		printf("=== %s === \n", motor->name);
		motor_write_config(stdout, &motor->config); 
	}

	motor_array_rewrite_config_dir(config_dir, &array);	
}
