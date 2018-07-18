#include <command_line_logic.h>

int command_line_logic(char* input_str, char* output_str, size_t buf_len, MotorArray* array) {
	/* clear the output string */
	bzero(output_str, buf_len);
	/* bite off the first token (word) of the input and store its location in command */
	char* command = strtok(input_str, " "); /* command is the POINTER, not the string itself */

	/* empty carriage return */
	if (command == NULL) {
		/* add length of "Expected command \n" to output_str */ //TODO
		output_str += snprintf(output_str, buf_len, "Expected command \n");

	/* ls */
	} else if (strcmp(command, "ls") == 0) {

		/* bite off second token of the input and store it in dev_name */
		char* dev_name = strtok(NULL, " ");

		/* placeholder pointer for looping through motors */
		Motor* select;

		/* no other tokens after 'ls'- just list all the devices */
		if (!dev_name) {
			/* loop through all devices */
			for (int i = 0; i < motor_array_length(); i++) {
				select = &((Motor*)array)[i]; /* store current motor in var select */

				output_str += snprintf(output_str, buf_len, "%10s 0x%hhx\n", select->name, select->config.address);
			}
		/* 2nd inputted token is a foreign device name */
		} else if (!(select = motor_match_string(array, dev_name))) {
			output_str += snprintf(output_str, buf_len, "Unrecognized device name or address '%s' \n", dev_name);
		/* 2nd inputted token is a recognized device name */
		} else {
			/* write the device name */
			output_str = motor_write_config_str(output_str, &select->config);
		}
	/* set */
	} else if (strcmp(command, "set") == 0) {
		/* placeholder device pointer */
		Motor* select;

		/* bite off second token of the input and store it in dev_name */
		char* dev_name = strtok(NULL, " ");

		/* no other tokens after 'set'- complain that there's no device */
		if (!dev_name) {
			output_str += snprintf(output_str, buf_len, "Missing device name. \n");
		/* 2nd inputted token is a foreign device name */
		} else if (!(select = motor_match_string(array, dev_name))) {
			output_str += snprintf(output_str, buf_len, "Unrecognized device name or address '%s' \n", dev_name);
		/* 2nd inputted token is a recognized device name */
		} else {
			/* bite off next token of input and store it in param_name */
			char* param_name = strtok(NULL, " ");

		/* I haven't gotten to commenting anything past this */
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

	/* clear */
	} else if (strcmp(command, "clear") == 0) {
		output_str += snprintf(output_str, buf_len, "\e[3J\e[H\e[2J");

	/* stop */
	} else if (strcmp(command, "stop") == 0) {
		return 0;

	/* something foreign */
	} else {
		output_str += snprintf(output_str, buf_len, "Unrecognized command '%s' \n", command);
	}
	output_str += snprintf(output_str, buf_len, "\n> ");
	return 1;
}
