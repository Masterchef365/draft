#include <motor.h>

#define KEY(NAME) #NAME,
char* motor_key_names[] = {
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
