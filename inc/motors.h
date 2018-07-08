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
	MOTOR_VARS()
} MotorVarNum;
#undef VAR

#define VAR(NAME) #NAME,
const char* motor_var_name_strings[] = {
	MOTOR_VARS()
};
#undef VAR

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

size_t motor_array_length () {
	return sizeof(MotorArray) / sizeof(Motor);
}

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

int motor_array_from_config_dir(int bus_fd, char* dir, MotorArray* motor_array) {
#define MOTOR(NAME) motor_from_config_file(bus_fd, dir, #NAME, &motor_array->NAME) &&
	return MOTORS() 1;
#undef MOTOR
}

int motor_rewrite_to_config_file(char* dir, char* name, Motor* motor) {
	FILE* file_ptr = file_ptr_config_file(dir, name, "w");
	if (file_ptr) {
		motor_write_config(file_ptr, &motor->config);
		fclose(file_ptr);
		return 1;
	} else {
		
	}
}

int motor_array_rewrite_config_dir(char* dir, MotorArray* motor_array) {
#define MOTOR(NAME) motor_rewrite_to_config_file(dir, #NAME, &motor_array->NAME) &&
	return MOTORS() 1;
#undef MOTOR
}

#endif
