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

typedef struct MotionServer {
	ServerConfig server_config;
	MotorConfig gantry_config;
	int debug_i2c;
	int i2c_bus_fd;

	/* polling file descriptor table */
	struct {
		struct pollfd joystick_fd;
		struct pollfd cmdline_fd;
		struct pollfd server_fd;
		struct pollfd new_fd;
	} fd_array;
	size_t fd_array_size;
	struct pollfd* fd_array_ptr;

} MotionServer;

void motion_server_init_configs(MotionServer* server, char* config_dir);
void motion_server_command_line_init(MotionServer* server);
void motion_server_init_socket(MotionServer* server);
void init_motion_server(MotionServer* server, char* config_dir, int debug_i2c);
int run_server_loop(MotionServer* server);
void clean_up_server(MotionServer* server);

int get_first_i2c_fd ();
void line_handler(char* line);
void option_reassign_socket(struct pollfd* target, int new_fd);

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
	init_motion_server(&server, argv[1], debug_i2c);
	inform_log(log_info, "Server init finished");
	while (run_server_loop(&server));
	clean_up_server(&server);

	/* Main loop */
	inform_log(log_info, "Server loop ended, exiting gracefully.");

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

void motion_server_init_configs(MotionServer* server, char* config_dir) {

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
	motor_load_or_write_defaults_from_dir(config_dir, "gantry.cfg", &server->gantry_config);

}

void motion_server_command_line_init(MotionServer* server) {
	/* Create GNU readline command line handler */
	rl_callback_handler_install("> ", line_handler);

	/* Command line file descriptor */
	server->fd_array.cmdline_fd.fd = fileno(stdin);
	server->fd_array.cmdline_fd.events = POLLIN;
}

void motion_server_init_socket(MotionServer* server) {
	/* Calculate size of internal fd array */
	server->fd_array_size = sizeof(server->fd_array) / sizeof(struct pollfd);
	server->fd_array_ptr = (struct pollfd*)&server->fd_array;
	inform_log(log_warn, "%p, %p, %p", server->fd_array, server->fd_array_ptr);

	/* Joystick file descriptor */
	server->fd_array.joystick_fd.events = POLLIN | POLLHUP;
	server->fd_array.joystick_fd.fd = -1; /* -1 indicates disconnected */
	
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

void init_motion_server(MotionServer* server, char* config_dir, int debug_i2c) {
	server->debug_i2c = debug_i2c;

	inform_log(log_warn, "what %p, %p, %p", server->fd_array, server->fd_array_ptr);

	/* Read server configs */
	motion_server_init_configs(server, config_dir);

	/* If we aren't debugging, find and open an I2C device */
	server->i2c_bus_fd = -1;
	if (!server->debug_i2c && (server->i2c_bus_fd = get_first_i2c_fd()) == -1) {
		inform_log(log_fail, "Not in debug mode, and failed to find a suitible i2c bus in /dev/. Exiting.");
		exit(EXIT_FAILURE);
	}

	/* Create socket and begin listening */
	motion_server_init_socket(server);

	/* Enable and initialize command line */
	motion_server_command_line_init(server);
}

int run_server_loop(MotionServer* server) {
	int keep_running = 1;

	int n = poll(server->fd_array_ptr, server->fd_array_size, -1);
	//inform_log(log_info, "Res: %i %s", n, strerror(errno));

	/* Handle joystick input */
	if (server->fd_array.joystick_fd.revents & POLLIN) {
		char buf[512] = {0};
		size_t n_read = read(server->fd_array.joystick_fd.fd, buf, 512);
		inform_log(log_info, "Joystick says: %s", buf);
	}

	/* Handle command line activity */
	if (server->fd_array.cmdline_fd.revents & POLLIN) {
		rl_callback_read_char();
		if (*last_command_line != '\0') {
			char* command_root = strtok(last_command_line, " ");
			if (strcmp(command_root, "ls") == 0) {
				char* list_target = strtok(NULL, " ");
				if (list_target != NULL) {
					if (strcmp(list_target, "connect") == 0) {
						inform_log(log_silent, "Joystick:   %s", server->fd_array.joystick_fd.fd != -1 ? "Y" : "N");
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
			} else {
				inform_log(log_info, "Unrecognized command: %s", command_root);
			}
			*last_command_line = '\0';
		}
	}

	/* Accept new clients */
	if (server->fd_array.server_fd.revents & POLLIN) {
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

	/* Handle id on unassigned connection */
	if (server->fd_array.new_fd.revents & POLLIN) {
		char buf[512] = {0};
		size_t n_read = read(server->fd_array.new_fd.fd, buf, 512);
		if (strcmp(buf, "id:joystick\n") == 0) {
			inform_log(log_info, "Joystick assigned");
			option_reassign_socket(&server->fd_array.joystick_fd, server->fd_array.new_fd.fd);
			server->fd_array.new_fd.fd = -1;
		}
	}

	/* Safely close and negate hung up sockets */
	if ((server->fd_array.joystick_fd.revents & POLLHUP && server->fd_array.joystick_fd.fd != -1) || !keep_running) {
		inform_log(log_warn, "Joystick has disconnected");
		option_reassign_socket(&server->fd_array.joystick_fd, -1);
	}
	if ((server->fd_array.new_fd.revents & POLLHUP && server->fd_array.new_fd.fd != -1) || !keep_running) {
		inform_log(log_warn, "Unassigned client has disconnected");
		option_reassign_socket(&server->fd_array.new_fd, -1);
	}
	return keep_running;
}

void clean_up_server(MotionServer* server) {
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
