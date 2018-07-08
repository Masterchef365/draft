#ifndef COMMAND_LINE_LOGIC_H
#define COMMAND_LINE_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <motors.h>
#include <util.h>

/* Decide what to do about a MotorArray based on user input. */
int command_line_logic(char* input_str, char* output_str, size_t buf_len, MotorArray* array);

#endif
