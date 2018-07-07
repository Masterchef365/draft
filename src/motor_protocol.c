#include <motor_protocol.h>

#if DEBUG
void send_var(int fd, VarId id, float value) {
	printf("Message (%i): %s to %f\n", fd, string_names[id], value);
}
#else
void send_var(int fd, VarId id, float value) {
	unsigned char received = 0;
	do {
		dprintf(fd, "[%c %f\n", (unsigned char)id, value);
		read(fd, &received, 1);
		usleep(1000);
	} while (received != (char)id);
}
#endif
