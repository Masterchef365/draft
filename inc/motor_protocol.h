#ifndef MOTOR_PROTOCOL_H
#define MOTOR_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_I2C 1

#define VARS() \
	VAR(set_target) \
	VAR(set_position) \
	VAR(set_kp) \
	VAR(set_ki) \
	VAR(set_kd) \
	VAR(set_enable) \

#define VAR(NAME) NAME,
typedef enum VarId { VARS() } VarId;
#undef VAR

#if DEBUG_I2C
extern const char* string_names[];
#endif

void motor_send_var(int fd, VarId id, float value);

#endif
