#ifndef MOTION_SERVER_H
#define MOTION_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <server_config.h>
#include <motor_config.h>
#include <calib_config.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>
#include <prettylog.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <stropts.h>
#include <linux/i2c-dev.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h> 

#include <motor.h>

/* Motion server struct. Contains necessary info to run the motion server. */
typedef struct MotionServer {
	ServerConfig server_config;
	CalibConfig calib_config;
	struct {
		MotorConfig gantry_config;
	} motor_array;
	size_t motor_array_size;
	MotorConfig* motor_array_ptr;

	int debug_i2c;
	int i2c_bus_fd;
	int i2c_last_addr;

	/* polling file descriptor tagged array */
	struct {
		struct pollfd vision_fd;
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
