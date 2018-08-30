#ifndef MOTOR_H
#define MOTOR_H

#include <string.h>

#define MOTOR_KEYS() \
	KEY(target) \
	KEY(position) \
	KEY(kp) \
	KEY(ki) \
	KEY(kd) \
	KEY(mode) \
	KEY(home) \
	KEY(home_pwm) \
	KEY(enable) \
	KEY(max_pwm) \

#define KEY(NAME) NAME,
enum MotorKey {
	motor_num_none,
	MOTOR_KEYS()
	motor_key_count
};
#undef KEY

extern char* motor_key_names[];

enum MotorKey motor_key_from_string(char* input);

void motor_send_var();

#endif
