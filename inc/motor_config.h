#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#define CONFIG_STRUCT_NAME MotorConfig
#define CONFIG_MANAGER_STRUCT_NAME MotorConfigManager
#define CONFIG_FUNCTION_PREFIX(NAME) motor_##NAME
#define MEMBERS() \
	MEMBER(float, Kp, "%f") \
	MEMBER(float, Ki, "%f") \
	MEMBER(float, Kd, "%f") \
	MEMBER(int, max_pwm, "%i") \
	MEMBER(int, home_pwm, "%i") \
	MEMBER(unsigned char, address, "0x%hhx") \

#include <make_config.h>

#undef CONFIG_STRUCT_NAME
#undef CONFIG_FUNCTION_PREFIX
#undef CONFIG_MANAGER_STRUCT_NAME
#undef MEMBERS

#endif
