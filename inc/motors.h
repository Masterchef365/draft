#ifndef MOTORS_H 
#define MOTORS_H 

#include <motor_config.h>
#include <stropts.h>
#include <linux/i2c-dev.h>
#include <util.h>

#define DEVICE_NAME_LENGTH 64
#define DEBUG_I2C 1

#define MOTOR_VARS() \
	VAR(target) \
	VAR(position) \
	VAR(kp) \
	VAR(ki) \
	VAR(kd) \
	VAR(mode) \
	VAR(home) \
	VAR(enable) \

#define MOTORS() \
	MOTOR(gantry) \
	MOTOR(forward) \

typedef struct Motor {
	char name[DEVICE_NAME_LENGTH];
	int fd;
	MotorConfig config;
} Motor;

#define MOTOR(NAME) Motor NAME;
typedef struct MotorArray {
	MOTORS()
} MotorArray;
#undef MOTOR

#define VAR(NAME) motor_num_##NAME,
typedef enum MotorVarNum {
	motor_num_none,
	MOTOR_VARS()
} MotorVarNum;
#undef VAR

/* Send a variable to a motor over I2C */
extern void motor_send_var(Motor* motor, MotorVarNum id, float value);

/* Get number of motors in MotorArray, mainly used to 
 * hack the struct into an array and use it as such. */
extern size_t motor_array_length ();

/* Convert a motor var number into a string */
extern char* motor_num_var_get_string(MotorVarNum num);

/* Match a string to it's corresponding MotorVarNum */
extern MotorVarNum motor_match_var_string (char* input);

/* Match a string to it's corresponding motor name */
extern Motor* motor_match_string(MotorArray* array, char* input);

/* Load a motor's configuration from the specified dir 
 * based on it's name and construct it appropriately */
extern int motor_from_config_file(int i2c_bus_fd, char* dir, char* name, Motor* motor);

/* Load a MotorArray struct at the specified dir, 
 * and using the specified i2c bus fd */
extern int motor_array_from_config_dir(int bus_fd, char* dir, MotorArray* motor_array);

/* Rewrite the config file for a motor */
extern int motor_rewrite_to_config_file(char* dir, char* name, Motor* motor);

/* Rewrite the config file for an entire MotorArray */
extern int motor_array_rewrite_config_dir(char* dir, MotorArray* motor_array);

#endif
