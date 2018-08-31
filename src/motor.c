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
	dprintf(fd, "%u%f\n", msg.num, value);
}

char* motor_string_from_key(enum MotorKey key) {
	return motor_key_names[key];	
}
