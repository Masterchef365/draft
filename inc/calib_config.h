#ifndef CALIB_CONFIG_H
#define CALIB_CONFIG_H

#define CONFIG_STRUCT_NAME CalibConfig
#define CONFIG_FUNCTION_PREFIX(NAME) calib_##NAME
#define MEMBERS() \
	MEMBER(float, offset_mm, "%f") \
	MEMBER(float, enc_per_mm, "%f") \

#include <make_config.h>

#undef CONFIG_STRUCT_NAME
#undef CONFIG_FUNCTION_PREFIX
#undef MEMBERS

#endif
