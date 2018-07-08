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

#define VAR(NAME) #NAME,
const char* motor_var_name_strings[] = {
	"none",
	MOTOR_VARS()
};
#undef VAR

/* Send a variable to a motor over I2C */
#if DEBUG_I2C
void motor_send_var(Motor* motor, MotorVarNum id, float value) {
	printf("%s: set %s = %f\n", motor->name, motor_var_name_strings[id], value);
}
#else
void motor_send_var(Motor* motor, MotorVarNum id, float value) {
	unsigned char received = 0;
	do {
		dprintf(motor->fd, "[%c %f\n", (unsigned char)id, value);
		read(motor->fd, &received, 1);
		usleep(1000);
	} while (received != (char)id);
}
#endif

/* Get number of motors in MotorArray, mainly used to 
 * hack the struct into an array and use it as such. */
size_t motor_array_length () {
	return sizeof(MotorArray) / sizeof(Motor);
}

/* Get number of params in MotorVarNum */
size_t motor_var_num_length() {
	return sizeof(motor_var_name_strings) / sizeof(char*);
}

MotorVarNum motor_match_var_string (char* input) {
	for (int i = 0; i < motor_var_num_length(); i++) {
		if (strcmp(input, motor_var_name_strings[i]) == 0) return i;
	}
	return 0;
}

Motor* motor_match_string(MotorArray* array, char* input) {
	Motor* iter = (Motor*)array;
	unsigned char address = 0;
	sscanf(input, "0x%hhx", &address);
	for (int i = 0; i < motor_array_length(); i++) {
		if (strcmp(input, iter->name) == 0 || address == iter->config.address) {
			return iter;
		}
		iter++;
	}
	return NULL;
}

/* Load a motor's configuration from the specified dir 
 * based on it's name and construct it appropriately */
int motor_from_config_file(int i2c_bus_fd, char* dir, char* name, Motor* motor) {
	FILE* file_ptr = file_ptr_config_file(dir, name, "r");
	if (file_ptr) {
		motor->config = motor_parse_config(file_ptr);
		motor->fd = dup(i2c_bus_fd);
		ioctl(motor->fd, I2C_SLAVE, motor->config.address);
		strncpy(motor->name, name, DEVICE_NAME_LENGTH);
		
		motor_send_var(motor, motor_num_kp, motor->config.Kp);
		motor_send_var(motor, motor_num_ki, motor->config.Ki);
		motor_send_var(motor, motor_num_kd, motor->config.Kd);

		fclose(file_ptr);
		return 1;
	} else {
		return 0;
	}
}

/* Load a MotorArray struct at the specified dir, 
 * and using the specified i2c bus fd */
int motor_array_from_config_dir(int bus_fd, char* dir, MotorArray* motor_array) {
#define MOTOR(NAME) motor_from_config_file(bus_fd, dir, #NAME, &motor_array->NAME) &&
	return MOTORS() 1;
#undef MOTOR
}

/* Rewrite the config file for a motor */
int motor_rewrite_to_config_file(char* dir, char* name, Motor* motor) {
	FILE* file_ptr = file_ptr_config_file(dir, name, "w");
	if (file_ptr) {
		motor_write_config(file_ptr, &motor->config);
		fclose(file_ptr);
		return 1;
	} else {
		
	}
}

/* Rewrite the config file for an entire MotorArray */
int motor_array_rewrite_config_dir(char* dir, MotorArray* motor_array) {
#define MOTOR(NAME) motor_rewrite_to_config_file(dir, #NAME, &motor_array->NAME) &&
	return MOTORS() 1;
#undef MOTOR
}

#endif
