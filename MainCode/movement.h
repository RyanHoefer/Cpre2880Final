#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "open_interface.h"

double move(oi_t *sensor_data, double distance_mm, int speed);
double move_dumb(oi_t *sensor_data, double distance_mm, int speed);
void turn_right(oi_t *sensor_data, double degrees);
void turn_left(oi_t *sensor_data, double degrees);

#endif
