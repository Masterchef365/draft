#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <server_config.h>
#include <motor_config.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>
#include <prettylog.h>
#include <readline/readline.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h> 

const char* debug_i2c_flag_str = "-debug_i2c"; /* Flag string to display and match against */
char last_command_line[512]; /* What was last input on the command line */

int get_first_i2c_fd ();
void line_handler(char* line);
void option_reassign_socket(struct pollfd* target, int new_fd);

int main(int argc, char** argv) {
	/* Warn user of incorrect usage and exit */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <config dir> %s\n", argv[0], debug_i2c_flag_str);
		exit(EXIT_FAILURE);
	}

	/* Config directory */
	char* config_dir = argv[1];

	/* Debug I2C? */
	int debug_i2c = 0;
	if (argc == 3 && strcmp(argv[2], debug_i2c_flag_str) == 0) {
		debug_i2c = 1;
		inform_log(log_warn, "Running in I2C debug mode. Remove %s to disable.", debug_i2c_flag_str);
	}

	/* Warn user of nonexistant config dir and exit */
	if (closedir(opendir(config_dir)) == -1) {
		inform_log(log_fail, "Config directory at %s does not exist, exiting.", config_dir);
		exit(EXIT_FAILURE);
	}

	/* Load server config or default */
	ServerConfig server_config = {0};
	server_config.portno = 5060;
	server_config.timeout_ms = 10;
	server_load_or_write_defaults_from_dir(config_dir, "server.cfg", &server_config);

	/* If we aren't debugging, find and open an I2C device */
	int i2c_bus_fd = -1;
	if (!debug_i2c && (i2c_bus_fd = get_first_i2c_fd()) == -1) {
		inform_log(log_fail, "Not in debug mode, and failed to find a suitible i2c bus in /dev/. Exiting.");
		exit(EXIT_FAILURE);
	}

	/* Load motor configurations */
	MotorConfig gantry_config = {0};
	motor_load_or_write_defaults_from_dir(config_dir, "gantry.cfg", &gantry_config);

	/* Create readline command line handler */
	rl_callback_handler_install("> ", line_handler);

	/* Set up polling file descriptor table */
	struct {
		//struct pollfd vision_fd;
		struct pollfd joystick_fd;
		struct pollfd cmdline_fd;
		struct pollfd server_fd;
		struct pollfd new_fd;
	} fd_array = {0};
	size_t fd_array_size = sizeof(fd_array) / sizeof(struct pollfd);
	struct pollfd* fd_array_ptr = (struct pollfd*)&fd_array;

	/* Command line file descriptor */
	fd_array.cmdline_fd.fd = fileno(stdin);
	fd_array.cmdline_fd.events = POLLIN;

	/* Joystick file descriptor */
	fd_array.joystick_fd.events = POLLIN | POLLHUP;
	fd_array.joystick_fd.fd = -1; /* -1 indicates disconnected */
	
	/* Unkown connection file descriptor; 
	 * handles new unassigned connections. */
	fd_array.new_fd.events = POLLIN | POLLHUP;
	fd_array.new_fd.fd = -1; /* -1 indicates disconnected */

	/* Server file descriptor. Handles async client acception. */
	fd_array.server_fd.events = POLLIN | POLLHUP;
	fd_array.server_fd.fd = -1; /* -1 indicates disconnected */

	/* Server socket */
	struct sockaddr_in serv_addr; /* Server address struct */
	serv_addr.sin_port = htons(server_config.portno);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	fd_array.server_fd.fd = socket(AF_INET, SOCK_STREAM, 0);

	/* Reuse server port. When exited incorrectly, the server can leave behind some data. */
	{
		int tmp;
		if (setsockopt(fd_array.server_fd.fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
			inform_log(log_fail, "Reusing server socket; %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	/* Bind the server socket to the address */
	if (bind(fd_array.server_fd.fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		inform_log(log_fail, "Binding server socket; %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Start listening on the server socket */
	listen(fd_array.server_fd.fd, 5);

	/* Main loop */
	int keep_running = 1;
	while (keep_running) if (poll(fd_array_ptr, fd_array_size, -1)) {
		/* Handle joystick input */
		if (fd_array.joystick_fd.revents & POLLIN) {
			char buf[512] = {0};
			size_t n_read = read(fd_array.joystick_fd.fd, buf, 512);
			inform_log(log_info, "Joystick says: %s", buf);
		}

		/* Handle command line activity */
		if (fd_array.cmdline_fd.revents & POLLIN) {
			rl_callback_read_char();
			if (*last_command_line != '\0') {
				char* command_root = strtok(last_command_line, " ");
				if (strcmp(command_root, "ls") == 0) {
					char* list_target = strtok(NULL, " ");
					if (list_target != NULL) {
						if (strcmp(list_target, "connect") == 0) {
							inform_log(log_silent, "Joystick:   %s", fd_array.joystick_fd.fd != -1 ? "Y" : "N");
							inform_log(log_silent, "Unassigned: %s", fd_array.new_fd.fd != -1 ? "Y" : "N");
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
				} else {
					inform_log(log_info, "Unrecognized command: %s", command_root);
				}
				*last_command_line = '\0';
			}
		}

		/* Accept new clients */
		if (fd_array.server_fd.revents & POLLIN) {
			struct sockaddr_in cli_addr; /* Client address struct */
			socklen_t clilen = sizeof(cli_addr);

			/* Remove old client, sorry bud you took too long. */
			int new_connection = accept(fd_array.server_fd.fd, (struct sockaddr *) &cli_addr, &clilen);
			option_reassign_socket(&fd_array.new_fd, new_connection);

			/* Display connected IP */
			char ipstr[INET6_ADDRSTRLEN];
    		inet_ntop(AF_INET, &cli_addr.sin_addr, ipstr, sizeof ipstr);
			inform_log(log_info, "New client from [%s]", ipstr);
		}

		/* Handle id on unassigned connection */
		if (fd_array.new_fd.revents & POLLIN) {
			char buf[512] = {0};
			size_t n_read = read(fd_array.new_fd.fd, buf, 512);
			if (strcmp(buf, "id:joystick\n") == 0) {
				inform_log(log_info, "Joystick assigned");
				option_reassign_socket(&fd_array.joystick_fd, fd_array.new_fd.fd);
				fd_array.new_fd.fd = -1;
			}
		}

		/* Safely close and negate hung up sockets */
		if ((fd_array.joystick_fd.revents & POLLHUP && fd_array.joystick_fd.fd != -1) || !keep_running) {
			inform_log(log_warn, "Joystick has disconnected");
			option_reassign_socket(&fd_array.joystick_fd, -1);
		}
		if ((fd_array.new_fd.revents & POLLHUP && fd_array.new_fd.fd != -1) || !keep_running) {
			inform_log(log_warn, "Unassigned client has disconnected");
			option_reassign_socket(&fd_array.new_fd, -1);
		}
	}

	inform_log(log_info, "Server loop ended, exiting gracefully.");

	/* Clean up readline */
	rl_reset_line_state();
	rl_restore_prompt();
	rl_cleanup_after_signal();
	rl_callback_handler_remove();

	/* De-uglify the command line */
	putchar('\n');

	/* Close I2C bus */
	if (!debug_i2c) close(i2c_bus_fd);
}

/* Find an i2c device file and return the file descriptor. 
 * Searches /dev/i2c-* from 0 to MAX_I2C_DEV_SEARCH. */
int get_first_i2c_fd () {
	char dir_name_buf[64];
	int fd;
	for (int i = 0; i < 20; i++) {
		snprintf(dir_name_buf, 64, "/dev/i2c-%i", i);
		if ((fd = open(dir_name_buf, O_RDWR)) != -1) return fd;
	}
	return fd;
}

/* Fake line handler, workaround */
void line_handler(char* line) {
	strcpy(last_command_line, line);
}

void option_reassign_socket(struct pollfd* target, int new_fd) {
	if (target->fd != -1) {
		close(target->fd);
		if (new_fd != -1) 
			inform_log(log_warn, "Kicked an old client in favor of a new connection");
	}
	target->fd = new_fd;
}
