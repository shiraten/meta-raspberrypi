#ifndef BMP280_H_
#define BMP280_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */

// BMP280
#define BMP280_WHOAMI				400
#define BMP280_RESET				401
#define BMP280_CFG				402
#define BMP280_CTRL				403
#define BMP280_GET_TEMP				404
#define BMP280_GET_PRESSURE			405
#define BMP280_GET_DIG_TEMP			406
#define BMP280_GET_DIG_PRESSURE			407
#define BMP280_STATUS				408


extern volatile int bmp_file;			// ioctl bmp_file

void initBMP280(int16_t *dig_temp_raw,int16_t *dig_pressure_raw);
void readPressureCoefficients(int16_t *dig_pressure_raw);
void readTemperatureCoefficients(int16_t *dig_temp_raw);
double bmp280_get_temperature(int16_t *dig_temp_raw, int32_t *t_fine);
double bmp280_get_pressure(int16_t *dig_temp_raw, int16_t *dig_pressure_raw);
float get_altitude(double pressure, double pressure_reference);




#endif
