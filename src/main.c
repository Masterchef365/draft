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

int get_first_i2c_fd ();
const char* debug_i2c_flag_str = "-debug_i2c";

char last_command_line[512];

void line_handler(char* line);

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
	} fd_array = {0};
	size_t fd_array_size = sizeof(fd_array) / sizeof(struct pollfd);
	struct pollfd* fd_array_ptr = (struct pollfd*)&fd_array;

	/* Command line file descriptor */
	fd_array.cmdline_fd.fd = fileno(stdin);
	fd_array.cmdline_fd.events = POLLIN;

	/* Joystick file descriptor */
	fd_array.joystick_fd.events = POLLIN | POLLHUP;
	fd_array.joystick_fd.fd = -1; /* -1 indicates disconnected */

	/* Main loop */
	int keep_running = 1;
	while (keep_running) if (poll(fd_array_ptr, fd_array_size, -1)) {
		inform_log(log_warn, "LOOP");

		/* Handle command line activity */
		if (fd_array.cmdline_fd.revents & POLLIN) {
			rl_callback_read_char();
			if (*last_command_line != '\0') {
				inform_log(log_info, "You entered: %s", last_command_line);
				if (*last_command_line == 'q' && strlen(last_command_line) == 1) keep_running = 0;
				*last_command_line = '\0';
			}
		}

		/* Safely close and negate hung up sockets */
		for (int i = 0; i < fd_array_size; i++) {
			struct pollfd* select = &fd_array_ptr[i];
			if (select->revents & POLLHUP) {
				close(select->fd);
				select->fd = -1;
			}
		}
	}

	inform_log(log_info, "Server loop ended, freeing memory and quitting.");
	
	/* Clean up readline */
	rl_reset_line_state();
	rl_restore_prompt();
	rl_cleanup_after_signal();
	rl_callback_handler_remove();

	/* De-ugly the command line */
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
