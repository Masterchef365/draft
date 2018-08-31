#include <motor.h>

#define KEY(NAME) #NAME,
static char* motor_key_names[] = {
	"<none>",
	MOTOR_KEYS()
	"<invalid>"
};
#undef KEY

enum MotorKey motor_key_from_string(char* input) {
	for (int i = 0; i < motor_key_count; i++) {
		if (strcmp(input, motor_key_names[i]) == 0) return i;
	}
	return motor_key_count;
}

void motor_send_var(int fd, enum MotorKey key, float value) {
	union MotorKeyMessage msg;
	msg.id_rw.id = key;
	msg.id_rw.rw = 1;
	dprintf(fd, "%c%f\n", msg.num, value);
}

float motor_read_var(int fd, enum MotorKey key) {
	union MotorKeyMessage msg_out;
	msg_out.id_rw.id = key;
	msg_out.id_rw.rw = 0;
	dprintf(fd, "%c%f\n", msg_out.num, 0.0);

	union MotorKeyMessage msg_in;
	char buf[512];
	size_t n_read = read(fd, buf, 512);
	inform_log(log_info, "n_read: %i", n_read);
	float num;
	sscanf("%c%f", &msg_in.num, &num);
	if (msg_in.id_rw.id != msg_out.id_rw.id) {
		inform_log(log_warn, "Receive did not match send!");
	}
	return num;
}

char* motor_string_from_key(enum MotorKey key) {
	return motor_key_names[key];	
}
