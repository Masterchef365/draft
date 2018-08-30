#ifndef MOTION_SERVER_H
#define MOTION_SERVER_H

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

/* Motion server struct. Contains necessary info to run the motion server. */
typedef struct MotionServer {
	ServerConfig server_config;
	MotorConfig gantry_config;

	int debug_i2c;
	int i2c_bus_fd;

	/* polling file descriptor tagged array */
	struct {
		struct pollfd joystick_fd;
		struct pollfd cmdline_fd;
		struct pollfd server_fd;
		struct pollfd new_fd;
	} fd_array;
	size_t fd_array_size;
	struct pollfd* fd_array_ptr;

} MotionServer;

void motion_server_init(MotionServer* server, char* config_dir, int debug_i2c);
int  motion_server_loop(MotionServer* server);
void motion_server_destruct(MotionServer* server);

#endif
