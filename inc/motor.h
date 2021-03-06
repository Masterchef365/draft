#ifndef MOTOR_H
#define MOTOR_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <prettylog.h>

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

#define KEY(NAME) motor_key_##NAME,
enum MotorKey {
	motor_key_none,
	MOTOR_KEYS()
	motor_key_count
};
#undef KEY

union MotorKeyMessage {
	struct {
		enum MotorKey id : 7;
		unsigned char rw : 1;
	} id_rw;
	unsigned char num;
};

enum MotorKey motor_key_from_string(char* input);

void motor_send_var(int fd, enum MotorKey key, float value);

char* motor_string_from_key(enum MotorKey key);

float motor_read_var(int fd, enum MotorKey key);

#endif
