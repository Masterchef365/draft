#include <command_line_logic.h>

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
#define VAR(NAME) dprintf(print_fd, "%s, ", #NAME);
				MOTOR_VARS()
#undef VAR
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
