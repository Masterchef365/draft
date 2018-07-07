#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#define CONFIG_STRUCT_NAME ServerConfig
#define CONFIG_FUNCTION_PREFIX(NAME) server_##NAME
#define MEMBERS() \
	MEMBER(float, Kp, "%f") \
	MEMBER(float, Ki, "%f") \
	MEMBER(float, Kd, "%f") \
	MEMBER(unsigned char, address, "0x%hhx") \

#include <make_config.h>
#undef MEMBERS

#endif
