#include <stdio.h>
#include <stdlib.h>
#include <client_handler.h>

#define JOY_N_BYTES 64

int main () {
	ClientHandler handler = create_client_handler(5000, 20);
	while (1) {
		handle_connections(&handler);
		if (handler.clients.joystick.fd != -1 && handler.clients.joystick.revents & POLLIN) {
			char buf[JOY_N_BYTES];
			bzero(buf, JOY_N_BYTES);
			if (handle_read(&handler.clients.joystick, buf, JOY_N_BYTES) > 0) {
				printf("Joystick message: %s\n", buf);
			}
		}
	}
	close_server(&handler);
}
