#ifndef MOTOR_PROTOCOL_H
#define MOTOR_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Don't actually send anything */
#define DEBUG 1

#define VARS() \
	VAR(set_target) \
	VAR(set_position) \
	VAR(set_kp) \
	VAR(set_ki) \
	VAR(set_kd) \
	VAR(set_enable) \

#define VAR(NAME) NAME,
typedef enum VarId {
	VARS()
} VarId;
#undef VAR

#if DEBUG
#define VAR(NAME) #NAME,
const char* string_names[] = {
	VARS()
};
#undef VAR
#endif

void send_var(int fd, VarId id, float value);

#endif
