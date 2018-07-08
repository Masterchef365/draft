#include <stdio.h>
#include <stdlib.h>
#include <motors.h>
#include <util.h>

int command_line_logic(int print_fd, char* message, MotorArray* array) {
	char* command = strtok(message, " ");
	if (command == NULL) {
		dprintf(print_fd, "Expected command \n");
	} else if (strcmp(command, "ls") == 0) {
		char* dev_name = strtok(NULL, " ");
		Motor* select;
		if (!dev_name) {
			for (int i = 0; i < motor_array_length(); i++) {
				select = &((Motor*)array)[i];
				dprintf(print_fd, "%10s 0x%hhx\n", select->name, select->config.address);
			}
		} else if (!(select = motor_match_string(array, dev_name))) {
			dprintf(print_fd, "Unrecognized device name or address '%s' \n", dev_name);
		} else {
			motor_write_config(stdout, &select->config);
		}
	} else if (strcmp(command, "set") == 0) {
		Motor* select;
		char* dev_name = strtok(NULL, " ");
		if (!dev_name) {
			dprintf(print_fd, "Missing device name. \n");
		} else if (!(select = motor_match_string(array, dev_name))) {
			dprintf(print_fd, "Unrecognized device name or address '%s' \n", dev_name);
		} else {
			char* param_name = strtok(NULL, " ");
			MotorVarNum id;
			if (!param_name || !(id = motor_match_var_string(param_name))) {
				dprintf(print_fd, "Parameter must be one of the following:\n");
				for (int i = 0; i < sizeof(motor_var_name_strings) / sizeof(char*); i++) {
					dprintf(print_fd, "%s, ", motor_var_name_strings[i]);
				}
				dprintf(print_fd, "\n");
			} else {
				char* param_value = strtok(NULL, " ");
				float value;
				if (!param_value) {
					dprintf(print_fd, "Missing value. \n");
				} else if (sscanf(param_value, "%f", &value) != 1) {
					dprintf(print_fd, "Failed to parse %s as float \n", param_value);
				} else {
					motor_send_var(select, id, value);
					dprintf(print_fd, "Sent!\n");
				}
			}
		}
	} else if (strcmp(command, "clear") == 0) {
		dprintf(print_fd, "\e[3J\e[H\e[2J");
	} else if (strcmp(command, "q") == 0) {
		return 0;
	} else {
		dprintf(print_fd, "Unrecognized command '%s' \n", command);
	}
	return 1;
}

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
