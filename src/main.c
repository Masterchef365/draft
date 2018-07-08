#include <stdio.h>
#include <stdlib.h>
#include <motors.h>
#include <util.h>
#include <command_line_logic.h>
#include <client_handler.h>
#include <server_config.h>

int main(int argc, char** argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char* config_dir = argv[1];

	/* Load server config file or fail and write an empty one */
	FILE* server_config_file;
	ServerConfig server_config;
	if (!(server_config_file = file_ptr_config_file(config_dir, "server", "r"))) {
		server_config_file = file_ptr_config_file(config_dir, "server", "w");
		bzero(&server_config, sizeof(ServerConfig));
		server_write_config(server_config_file, &server_config);
		fclose(server_config_file);
		exit(EXIT_FAILURE);
	} else {
		server_config = server_parse_config(server_config_file);
		fclose(server_config_file);
	}

	int i2c_bus_fd = -1;
#if !DEBUG_I2C
	if ((i2c_bus_fd = get_first_i2c_fd()) == -1) {
		fprintf(stderr, "Failed to find a suitible i2c bus in /dev/. Exiting.\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* Load motor configs from directory */
	MotorArray motors;
	if (!motor_array_from_config_dir(i2c_bus_fd, config_dir, &motors)) {
		fprintf(stderr, "Failure opening one or more config files. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	ClientHandler handler = create_client_handler(server_config.portno, server_config.timeout_ms);

	int run_app = 1;
	const size_t cmd_buf_len = 1024;
	char cmd_in_buf[cmd_buf_len];
	char cmd_out_buf[cmd_buf_len];
	bzero(cmd_in_buf, cmd_buf_len);
	enum CommandLineState {
		cmd_idle, cmd_disconnected, cmd_wait_send
	} cmd_state = cmd_disconnected;

	char joystick_buf[64];
	do {
		handle_connections(&handler);

		if (cmd_state == cmd_disconnected && handler.clients.cmd.fd != -1) {
			snprintf(cmd_out_buf, cmd_buf_len, "BroccoliBot motor server v0.1\n> ");
			cmd_state = cmd_wait_send;
		}
		if (cmd_state != cmd_disconnected && handler.clients.cmd.fd == -1) cmd_state = cmd_disconnected;
		if (cmd_state == cmd_wait_send && handle_write(&handler.clients.cmd, cmd_out_buf, strlen(cmd_out_buf))) cmd_state = cmd_idle;
		if (cmd_state == cmd_idle && handle_read(&handler.clients.cmd, cmd_in_buf, cmd_buf_len)) {
			cmd_in_buf[strlen(cmd_in_buf) - 1] = '\0'; /* Remove trailing newline */
			run_app = command_line_logic(cmd_in_buf, cmd_out_buf, cmd_buf_len, &motors);
			cmd_state = cmd_wait_send;
			bzero(cmd_in_buf, cmd_buf_len);
		}

		if (handle_read(&handler.clients.joystick, joystick_buf, 64)) {
			char type_buf[32];
			int idx, value;
			if (sscanf(joystick_buf, "%s %i %i\n", type_buf, &idx, &value) == 3) {
				if (strcmp(type_buf, "axs") == 0) {
					if (idx == 1) motor_send_var(&motors.forward, motor_num_target, (float)value);
				}
			}
		}
	} while(run_app);
	hander_close(&handler);	

	motor_array_rewrite_config_dir(config_dir, &motors);	
}
