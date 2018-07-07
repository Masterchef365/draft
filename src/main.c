#include <client_handler.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define JOY_N_BYTES 64

typedef enum FieldNum {
	set_target, set_position, set_kp, set_ki, set_kd, set_enable,
} FieldNum;

/*
void send_var (int fd, FieldNum num, float value) {
	unsigned char received = 0;
	do {
		dprintf(fd, "[%hhc %f\n", (unsigned char)num, value);
		read(fd, &received, 1);
		usleep(1000);
	} while (received != (char)num);
}
*/

void send_var (int fd, FieldNum num, float value) {
	printf("[%i %f\n", num, value);
}

int main () {
	int device;
	/*
	char *filename = "/dev/i2c-1";
	if ((device = open(filename, O_RDWR)) < 0) {
		perror("Failed to open the i2c bus");
		exit(1);
	}
	int addr = 0x04;
	if (ioctl(device, I2C_SLAVE, addr) < 0) {
		perror("Failed to acquire bus access and/or talk to slave");
		exit(1);
	}
	*/

	float pos = 0;
	double Kp=0.4, Ki=0.08, Kd=0.020;
	send_var(device, set_kd, Kd);
	send_var(device, set_ki, Ki);
	send_var(device, set_kp, Kp);
	send_var(device, set_position, pos);
	send_var(device, set_enable, 1.0);

	ClientHandler handler = create_client_handler(5000, 20);

	int iter = 0;
	while (1) {
		handle_connections(&handler);
		if (handler.clients.joystick.fd != -1 && handler.clients.joystick.revents & POLLIN && iter % 50 == 0) {
			char buf[JOY_N_BYTES];
			bzero(buf, JOY_N_BYTES);
			if (handle_read(&handler.clients.joystick, buf, JOY_N_BYTES) > 0) {
				printf("Joystick message: %s\n", buf);
				int joy_pos;
				if (sscanf(buf, "axs 1 %i\n", &joy_pos) == 1) {
					printf("%i\n", joy_pos);
					send_var(device, set_target, joy_pos / 30);
				}
			}
		}
		iter++;
	}
	close(device);
	close_server(&handler);
}
