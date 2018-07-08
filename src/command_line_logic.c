#include <command_line_logic.h>

int command_line_logic(char* input_str, char* output_str, size_t buf_len, MotorArray* array) {
	bzero(output_str, buf_len);
	char* command = strtok(input_str, " ");
	if (command == NULL) {
		output_str += snprintf(output_str, buf_len, "Expected command \n");
	} else if (strcmp(command, "ls") == 0) {
		char* dev_name = strtok(NULL, " ");
		Motor* select;
		if (!dev_name) {
			for (int i = 0; i < motor_array_length(); i++) {
				select = &((Motor*)array)[i];
				output_str += snprintf(output_str, buf_len, "%10s 0x%hhx\n", select->name, select->config.address);
			}
		} else if (!(select = motor_match_string(array, dev_name))) {
			output_str += snprintf(output_str, buf_len, "Unrecognized device name or address '%s' \n", dev_name);
		} else {
			output_str = motor_write_config_str(output_str, &select->config);
		}
	} else if (strcmp(command, "set") == 0) {
		Motor* select;
		char* dev_name = strtok(NULL, " ");
		if (!dev_name) {
			output_str += snprintf(output_str, buf_len, "Missing device name. \n");
		} else if (!(select = motor_match_string(array, dev_name))) {
			output_str += snprintf(output_str, buf_len, "Unrecognized device name or address '%s' \n", dev_name);
		} else {
			char* param_name = strtok(NULL, " ");
			MotorVarNum id;
			if (!param_name || !(id = motor_match_var_string(param_name))) {
				output_str += snprintf(output_str, buf_len, "Parameter must be one of the following:\n");
#define VAR(NAME) output_str += snprintf(output_str, buf_len, "%s, ", #NAME);
				MOTOR_VARS()
#undef VAR
				output_str += snprintf(output_str, buf_len, "\n");
			} else {
				char* param_value = strtok(NULL, " ");
				float value;
				if (!param_value) {
					output_str += snprintf(output_str, buf_len, "Missing value. \n");
				} else if (sscanf(param_value, "%f", &value) != 1) {
					output_str += snprintf(output_str, buf_len, "Failed to parse %s as float \n", param_value);
				} else {
					motor_send_var(select, id, value);
					output_str += snprintf(output_str, buf_len, "Sent!\n");
				}
			}
		}
	} else if (strcmp(command, "clear") == 0) {
		output_str += snprintf(output_str, buf_len, "\e[3J\e[H\e[2J");
	} else if (strcmp(command, "stop") == 0) {
		return 0;
	} else {
		output_str += snprintf(output_str, buf_len, "Unrecognized command '%s' \n", command);
	}
	output_str += snprintf(output_str, buf_len, "\n> ");
	return 1;
}
