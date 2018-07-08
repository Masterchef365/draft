#ifndef COMMAND_LINE_LOGIC_H
#define COMMAND_LINE_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <motors.h>
#include <util.h>

/* Decide what to do about a MotorArray based on user input. */
int command_line_logic(int print_fd, char* message, MotorArray* array);

#endif
