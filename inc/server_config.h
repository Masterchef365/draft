#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#define CONFIG_STRUCT_NAME ServerConfig
#define CONFIG_MANAGER_STRUCT_NAME ServerConfigManager
#define CONFIG_FUNCTION_PREFIX(NAME) server_##NAME
#define MEMBERS() \
	MEMBER(int, portno, "%i") \
	MEMBER(int, timeout_ms, "%i") \

#include <make_config.h>

#undef CONFIG_STRUCT_NAME
#undef CONFIG_FUNCTION_PREFIX
#undef CONFIG_MANAGER_STRUCT_NAME
#undef MEMBERS

#endif
