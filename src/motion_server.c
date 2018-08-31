#include <motion_server.h>

static int get_first_i2c_fd ();
static void line_handler(char* line);
static void option_reassign_socket(struct pollfd* target, int new_fd);

/* What was last input on the command line, dumb hack 
 * because readline has no method for a line completion 
 * without a callback. Eww. */
static char last_command_line[512]; 

static inline void motion_server_init_configs(MotionServer* server, char* config_dir);
static inline void motion_server_init_commandline(MotionServer* server);
static inline void motion_server_init_socket(MotionServer* server);
static inline void motion_server_new_client(MotionServer* server);
static inline void motion_server_new_client_set_id(MotionServer* server);
static inline void motion_server_select_motor_send(MotionServer* server, MotorConfig* motor, enum MotorKey key, float value);
static inline float motion_server_select_motor_read(MotionServer* server, MotorConfig* motor, enum MotorKey key);
static inline void motion_server_bootstrap_motor(MotionServer* server, MotorConfig* motor);
static inline MotorConfig* motion_server_motor_by_address(MotionServer* server, unsigned char address);
static inline int motion_server_parse_command(MotionServer* server, char* input_string);
static inline void motion_server_vision_handle(MotionServer* server, char* input_string);

/* Find an i2c device file and return the file descriptor. 
 * Searches /dev/i2c-* from 0 to MAX_I2C_DEV_SEARCH. */
int get_first_i2c_fd () {
	char dir_name_buf[64];
	int fd;
	for (int i = 0; i < 20; i++) {
		snprintf(dir_name_buf, 64, "/dev/i2c-%i", i);
		if ((fd = open(dir_name_buf, O_RDWR)) != -1) {
			inform_log(log_info, "I2C bus: %s", dir_name_buf);
			return fd;
		}
	}
	return fd;
}

/* Fake line handler, workaround */
static void line_handler(char* line) {
	strcpy(last_command_line, line);
}

static void option_reassign_socket(struct pollfd* target, int new_fd) {
	if (target->fd != -1) {
		close(target->fd);
		if (new_fd != -1) 
			inform_log(log_warn, "Kicked an old client in favor of a new connection");
	}
	target->fd = new_fd;
}

/* Note: We use the config here as it can supply info on what 
 * address the motor has and can mutate the settings in the config as such */
static void motion_server_select_motor_send(MotionServer* server, MotorConfig* motor, enum MotorKey key, float value) {
	if (key == motor_key_count || key == motor_key_none) {
		inform_log(log_warn, "Invalid motor key!");
		return;
	}

	if (server->debug_i2c) {
		inform_log(log_info, "I2C_DEBUG: (0x%hhx) %s = %f", motor->address, motor_string_from_key(key), value);
	} else {
		/* If the last address sent to/received from isn't the current 
		 * selected address, tell the fd to point to that address */
		if (motor->address != server->i2c_last_addr && ioctl(server->i2c_bus_fd, I2C_SLAVE, motor->address) == -1) {
			inform_log(log_fail, "Failed to communicate with slave address 0x%hhx\n", motor->address);
			return;
		}

		motor_send_var(server->i2c_bus_fd, key, value);
	}
}

static inline float motion_server_select_motor_read(MotionServer* server, MotorConfig* motor, enum MotorKey key) {
	if (key == motor_key_count || key == motor_key_none) {
		inform_log(log_warn, "Invalid motor key!");
		return 0.0;
	}

	if (server->debug_i2c) {
		inform_log(log_info, "I2C_DEBUG: (0x%hhx) %s = %f", motor->address, motor_string_from_key(key), 0.0);
		return 0.0;
	} else {
		/* If the last address sent to/received from isn't the current 
		 * selected address, tell the fd to point to that address */
		if (motor->address != server->i2c_last_addr && ioctl(server->i2c_bus_fd, I2C_SLAVE, motor->address) == -1) {
			inform_log(log_fail, "Failed to communicate with slave address 0x%hhx\n", motor->address);
			return 0.0;
		}
		return motor_read_var(server->i2c_bus_fd, key);
	}
}

static MotorConfig* motion_server_motor_by_address(MotionServer* server, unsigned char address) {
	for (int i = 0; i < server->motor_array_size; i++) {
		MotorConfig* select = &server->motor_array_ptr[i];
		if (select->address == address) {
			return select;
		}
	}
	return NULL;
}

static void motion_server_bootstrap_motor(MotionServer* server, MotorConfig* motor) {
	motion_server_select_motor_send(server, motor, motor_key_kp, motor->Kp);
	motion_server_select_motor_send(server, motor, motor_key_ki, motor->Ki);
	motion_server_select_motor_send(server, motor, motor_key_kd, motor->Kd);
	motion_server_select_motor_send(server, motor, motor_key_max_pwm, motor->max_pwm);
	motion_server_select_motor_send(server, motor, motor_key_home_pwm, motor->home_pwm);
	motion_server_select_motor_send(server, motor, motor_key_enable, 0);
	motion_server_select_motor_send(server, motor, motor_key_home, 0);
}

static void motion_server_init_configs(MotionServer* server, char* config_dir) {
	/* Turn the motor configs into an array */
	server->motor_array_size = sizeof(server->motor_array) / sizeof(MotorConfig);
	server->motor_array_ptr = (MotorConfig*)&server->motor_array;

	/* Warn user of nonexistant config dir and exit */
	if (closedir(opendir(config_dir)) == -1) {
		inform_log(log_fail, "Config directory at %s does not exist, exiting.", config_dir);
		exit(EXIT_FAILURE);
	}

	/* Load server config or default */
	server->server_config.portno = 5060;
	server->server_config.timeout_ms = 10;
	server_load_or_write_defaults_from_dir(config_dir, "server.cfg", &server->server_config);

	/* Load motor configurations */
	motor_load_or_write_defaults_from_dir(config_dir, "gantry.cfg", &server->motor_array.gantry_config);

	/* Load calibration configuration */
	calib_load_or_write_defaults_from_dir(config_dir, "calib.cfg", &server->calib_config);

	/* Bootstrap motors */
	for (int i = 0; i < server->motor_array_size; i++) {
		MotorConfig* select = &server->motor_array_ptr[i];
		motion_server_bootstrap_motor(server, select);
	}

}

static void motion_server_init_commandline(MotionServer* server) {
	/* Create GNU readline command line handler */
	rl_callback_handler_install("> ", line_handler);

	/* Command line file descriptor */
	server->fd_array.cmdline_fd.fd = fileno(stdin);
	server->fd_array.cmdline_fd.events = POLLIN;
}

static int motion_server_parse_command(MotionServer* server, char* input_string) {
	int keep_running = 1;
	char* command_root = strtok(input_string, " ");
	if (strcmp(command_root, "ls") == 0) {
		char* list_target = strtok(NULL, " ");
		if (list_target != NULL) {
			if (strcmp(list_target, "connect") == 0) {
				inform_log(log_silent, "Vision:   %s", server->fd_array.vision_fd.fd != -1 ? "Y" : "N");
				inform_log(log_silent, "Unassigned: %s", server->fd_array.new_fd.fd != -1 ? "Y" : "N");
			} else {
				inform_log(log_info, "Unrecognized list '%s'", list_target);
			}
		} else {
			inform_log(log_info, "Expected target");
		}
	} else if (strcmp(command_root, "q") == 0 || strcmp(command_root, "exit") == 0) {
		keep_running = 0;
	} else if (strcmp(command_root, "clear") == 0) {
		printf("\e[3J\e[H\e[2J");
		rl_forced_update_display();
	} else if (strcmp(command_root, "set") == 0 || strcmp(command_root, "read") == 0) {
		char* addrs_str = strtok(NULL, " ");
		char* field_str = strtok(NULL, " ");
		char* value_str = strtok(NULL, " ");
		if (!addrs_str || !field_str || !value_str) {
			inform_log(log_warn, "Expected command form: %s <addr> <field> <value>", command_root);
		} else {
			unsigned char address = 0;
			sscanf(addrs_str, "0x%hhx", &address);

			enum MotorKey key = motor_key_from_string(field_str);
			if (key == motor_key_count) {
				inform_log(log_info, "Motor key must be one of:");
				for (int i = 1; i < motor_key_count; i++) {
					inform_log(log_silent, "%s", motor_string_from_key(i));
				}
			}

			float value = atof(value_str);

			MotorConfig* config = motion_server_motor_by_address(server, address);
			if (config) {
				if (strcmp(command_root, "set") == 0) {
					motion_server_select_motor_send(server, config, key, value);
				} else {
					inform_log(log_info, "Controller sent back: %f", motion_server_select_motor_read(server, config, key));
				}
			} else {
				inform_log(log_info, "No motor at address %s", addrs_str);
			}
		}
	} else {
		inform_log(log_info, "Unrecognized command: %s", command_root);
	}
	return keep_running;
}

static void motion_server_vision_handle(MotionServer* server, char* input_string) {
	float target_vision = atof(input_string);
	float target_enc = (server->calib_config.enc_per_mm * target_vision) + server->calib_config.offset_mm;
	motion_server_select_motor_send(server, &server->motor_array.gantry_config, motor_key_target, target_enc);
}

static void motion_server_init_socket(MotionServer* server) {
	/* Calculate size of internal fd array */
	server->fd_array_size = sizeof(server->fd_array) / sizeof(struct pollfd);
	server->fd_array_ptr = (struct pollfd*)&server->fd_array;

	/* Vision file descriptor */
	server->fd_array.vision_fd.events = POLLIN | POLLHUP;
	server->fd_array.vision_fd.fd = -1; /* -1 indicates disconnected */

	/* Unkown connection file descriptor; 
	 * handles new unassigned connections. */
	server->fd_array.new_fd.events = POLLIN | POLLHUP;
	server->fd_array.new_fd.fd = -1; /* -1 indicates disconnected */

	/* Server file descriptor. Handles async client acception. */
	server->fd_array.server_fd.events = POLLIN | POLLHUP;
	server->fd_array.server_fd.fd = -1; /* -1 indicates disconnected */

	/* Server socket */
	struct sockaddr_in serv_addr; /* Server address struct */
	serv_addr.sin_port = htons(server->server_config.portno);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	server->fd_array.server_fd.fd = socket(AF_INET, SOCK_STREAM, 0);

	/* Reuse server port. When exited incorrectly, the server can leave behind some data. */
	{
		int tmp;
		if (setsockopt(server->fd_array.server_fd.fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
			inform_log(log_fail, "Reusing server socket; %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	/* Bind the server socket to the address */
	if (bind(server->fd_array.server_fd.fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		inform_log(log_fail, "Binding server socket; %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Start listening on the server socket */
	listen(server->fd_array.server_fd.fd, 5);
}

void motion_server_init(MotionServer* server, char* config_dir, int debug_i2c) {
	bzero(server, sizeof(MotionServer));
	server->debug_i2c = debug_i2c;

	/* If we aren't debugging, find and open an I2C device */
	server->i2c_bus_fd = -1;
	if (!server->debug_i2c && (server->i2c_bus_fd = get_first_i2c_fd()) == -1) {
		inform_log(log_fail, "Not in debug mode, and failed to find a suitible i2c bus in /dev/. Exiting.");
		exit(EXIT_FAILURE);
	}

	/* Read server configs */
	motion_server_init_configs(server, config_dir);

	/* Create socket and begin listening */
	motion_server_init_socket(server);

	/* Enable and initialize command line */
	motion_server_init_commandline(server);
}

static void motion_server_new_client(MotionServer* server) {
	struct sockaddr_in cli_addr; /* Client address struct */
	socklen_t clilen = sizeof(cli_addr);

	/* Remove old client, sorry bud you took too long. */
	int new_connection = accept(server->fd_array.server_fd.fd, (struct sockaddr *) &cli_addr, &clilen);
	option_reassign_socket(&server->fd_array.new_fd, new_connection);

	/* Display connected IP */
	char ipstr[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &cli_addr.sin_addr, ipstr, sizeof ipstr);
	inform_log(log_info, "New client from [%s]", ipstr);
}

static inline void motion_server_new_client_set_id(MotionServer* server) {
	char buf[512] = {0};
	size_t n_read = read(server->fd_array.new_fd.fd, buf, 512);
	if (strcmp(buf, "id:vision\n") == 0) {
		inform_log(log_info, "Vision assigned");
		option_reassign_socket(&server->fd_array.vision_fd, server->fd_array.new_fd.fd);
		server->fd_array.new_fd.fd = -1;
	}
}

int motion_server_loop(MotionServer* server) {
	int keep_running = 1;

	/* Block until we have activity on any input */ 
	poll(server->fd_array_ptr, server->fd_array_size, -1);

	/* Handle vision input */
	if (server->fd_array.vision_fd.revents & POLLIN) {
		char buf[512] = {0};
		size_t n_read = read(server->fd_array.vision_fd.fd, buf, 512);
		motion_server_vision_handle(server, buf);
	}

	/* Handle command line activity */
	if (server->fd_array.cmdline_fd.revents & POLLIN) {
		rl_callback_read_char();
		if (*last_command_line != '\0') {
			keep_running = motion_server_parse_command(server, last_command_line);	
			*last_command_line = '\0';
		}
	}

	/* Accept new clients */
	if (server->fd_array.server_fd.revents & POLLIN)
		motion_server_new_client(server);

	/* Handle id on unassigned connection */
	if (server->fd_array.new_fd.revents & POLLIN)
		motion_server_new_client_set_id(server);

	/* Safely close and negate hung up sockets */
	if ((server->fd_array.vision_fd.revents & POLLHUP && server->fd_array.vision_fd.fd != -1) || !keep_running) {
		inform_log(log_warn, "Vision has disconnected");
		option_reassign_socket(&server->fd_array.vision_fd, -1);
	}
	if ((server->fd_array.new_fd.revents & POLLHUP && server->fd_array.new_fd.fd != -1) || !keep_running) {
		inform_log(log_warn, "Unassigned client has disconnected");
		option_reassign_socket(&server->fd_array.new_fd, -1);
	}
	return keep_running;
}

void motion_server_destruct(MotionServer* server) {
	/* Stall the motors */
	for (int i = 0; i < server->motor_array_size; i++) {
		MotorConfig* select = &server->motor_array_ptr[i];
		motion_server_bootstrap_motor(server, select);
	}

	/* Clean up readline */
	rl_reset_line_state();
	rl_restore_prompt();
	rl_cleanup_after_signal();
	rl_callback_handler_remove();

	/* De-uglify the command line */
	putchar('\n');

	/* Close I2C bus */
	if (!server->debug_i2c) close(server->i2c_bus_fd);
}
