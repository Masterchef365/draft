#include <motors.h>

extern char* motor_num_var_get_string(MotorVarNum num) {
	switch (num) {
#define VAR(NAME) case motor_num_##NAME: return #NAME;
		MOTOR_VARS()
#undef VAR
		default: return 0;
	}
}

#if DEBUG_I2C
void motor_send_var(Motor* motor, MotorVarNum id, float value) {
	printf("%s: set %s = %f\n", motor->name, motor_num_var_get_string(id), value);

	/* TODO: Move this logic somewhere else? */
	switch (id) {
		case motor_num_kp: motor->config.Kp = value; break;
		case motor_num_ki: motor->config.Ki = value; break;
		case motor_num_kd: motor->config.Kd = value; break;
		case motor_num_max_pwm: motor->config.max_pwm = value; break;
		case motor_num_home_pwm: motor->config.home_pwm = value; break;
		default: break;
	}
}
#else
void motor_send_var(Motor* motor, MotorVarNum id, float value) {
	unsigned char received = 0;
	do {
		dprintf(motor->fd, "[%c %f\n", (unsigned char)id, value);
		read(motor->fd, &received, 1);
		usleep(1000);
	} while (received != (char)id);

	/* TODO: Move this logic somewhere else? */
	switch (id) {
		case motor_num_kp: motor->config.Kp = value; break;
		case motor_num_ki: motor->config.Ki = value; break;
		case motor_num_kd: motor->config.Kd = value; break;
		case motor_num_max_pwm: motor->config.max_pwm = value; break;
		case motor_num_home_pwm: motor->config.home_pwm = value; break;
		default: break;
	}
}
#endif

size_t motor_array_length () {
	return sizeof(MotorArray) / sizeof(Motor);
}

MotorVarNum motor_match_var_string (char* input) {
#define VAR(NAME) if (strcmp(input, #NAME) == 0) return motor_num_##NAME;
	MOTOR_VARS()
#undef VAR
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

int motor_from_config_file(int i2c_bus_fd, char* dir, char* name, Motor* motor) {
	FILE* file_ptr = file_ptr_config_file(dir, name, "r");
	if (file_ptr) {
		motor->config = motor_parse_config(file_ptr);
#if !DEBUG_I2C
		motor->fd = dup(i2c_bus_fd);
		if (ioctl(motor->fd, I2C_SLAVE, motor->config.address) == -1) {
			fprintf(stderr, "Failed to communicate with slave address %hhx\n", motor->config.address);
			return 0;
		}
#endif
		strncpy(motor->name, name, DEVICE_NAME_LENGTH);

		/* TODO: Move this logic somewhere else? */
		motor_send_var(motor, motor_num_kp, motor->config.Kp);
		motor_send_var(motor, motor_num_ki, motor->config.Ki);
		motor_send_var(motor, motor_num_kd, motor->config.Kd);
		motor_send_var(motor, motor_num_max_pwm, motor->config.max_pwm);
		motor_send_var(motor, motor_num_home_pwm, motor->config.home_pwm);

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
		return 0;
	}
}

int motor_array_rewrite_config_dir(char* dir, MotorArray* motor_array) {
#define MOTOR(NAME) motor_rewrite_to_config_file(dir, #NAME, &motor_array->NAME) &&
	return MOTORS() 1;
#undef MOTOR
}
