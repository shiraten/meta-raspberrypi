#include "BMP280.h"

volatile int bmp_file = 0;

void initBMP280(int16_t *dig_temp_raw,int16_t *dig_pressure_raw) {
	readTemperatureCoefficients(dig_temp_raw);
	readPressureCoefficients(dig_pressure_raw);
	//ioctl(bmp_file, BMP280_CFG, 0x00);	// t_sb 500us
	ioctl(bmp_file, BMP280_CTRL, 0x53);	// p_oversampling = 4 ; t_oversampling = 2 ; normal mode
}
void readTemperatureCoefficients(int16_t *dig_temp_raw) {
	uint8_t dig_temp[6];
	ioctl(bmp_file, BMP280_GET_DIG_TEMP, dig_temp);
	dig_temp_raw[0] = (dig_temp[0] | (dig_temp[1] << 8));
	dig_temp_raw[1] = (dig_temp[2] | (dig_temp[3] << 8));
	dig_temp_raw[2] = (dig_temp[4] | (dig_temp[5] << 8));
}

void readPressureCoefficients(int16_t *dig_pressure_raw) {
	uint8_t dig_pressure[18];
	ioctl(bmp_file, BMP280_GET_DIG_PRESSURE, dig_pressure);
	dig_pressure_raw[0] = (dig_pressure[0] | (dig_pressure[1] << 8));
	dig_pressure_raw[1] = (dig_pressure[2] | (dig_pressure[3] << 8));
	dig_pressure_raw[2] = (dig_pressure[4] | (dig_pressure[5] << 8));
	dig_pressure_raw[3] = (dig_pressure[6] | (dig_pressure[7] << 8));
	dig_pressure_raw[4] = (dig_pressure[8] | (dig_pressure[9] << 8));
	dig_pressure_raw[5] = (dig_pressure[10] | (dig_pressure[11] << 8));
	dig_pressure_raw[6] = (dig_pressure[12] | (dig_pressure[13] << 8));
	dig_pressure_raw[7] = (dig_pressure[14] | (dig_pressure[15] << 8));
	dig_pressure_raw[8] = (dig_pressure[16] | (dig_pressure[17] << 8));
}

// return temperature in degrees C, resolution is 0.01 DegC. output value of 5123 equals to 51.23 DegC
double bmp280_get_temperature(int16_t *dig_temp_raw, int32_t *ret_t_fine) {
	uint8_t dig_temp[6], temp[3];
	uint32_t adc_T, t_fine;
	float var1, var2, T;

	// Get temperature
	ioctl(bmp_file, BMP280_GET_TEMP, temp);
	adc_T = ((temp[0]<<12) + (temp[1]<<4) + (temp[2]>>4));
	// calculate temperature in degrees C
	var1 = (((double)adc_T)/16384.0 - ((double)((uint16_t)dig_temp_raw[0]))/1024.0) * ((double)dig_temp_raw[1]);
	var2 = ((((double)adc_T)/131072.0 - ((double)((uint16_t)dig_temp_raw[0]))/8192.0) * (((double)adc_T)/131072.0 - ((double)((uint16_t)dig_temp_raw[0]))/8192.0)) * ((double)dig_temp_raw[2]);
	t_fine = (uint32_t)(var1+var2);
	if(ret_t_fine != NULL) {
		*ret_t_fine = t_fine;
	}
	T = (var1+var2) / 5120;

	return T;
}

// return pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractionnam bits)
// output value of 24674867 represent 24674867/256 = 96386.2 Pa = 963.862 hPa
double bmp280_get_pressure(int16_t *dig_temp_raw, int16_t *dig_pressure_raw) {
	uint8_t dig_pressure[18], pressure[3];
	uint32_t t_fine;
	uint64_t adc_P;
	double var1, var2, P;
	// Get t_fine
	bmp280_get_temperature(dig_temp_raw, &t_fine);
	// Get pressure
	ioctl(bmp_file, BMP280_GET_PRESSURE, pressure);
	adc_P = ((pressure[0]<<12) + (pressure[1]<<4) + (pressure[2]>>4));
	//printf("%02X ; %02X ; %02X\n", pressure[0], pressure[1], pressure[2]);
	// calculate pressure
	var1 = ((double)t_fine / 2.0) - 64000.0;
	var2 = var1*var1 * (double)dig_pressure_raw[5] / 32768;
	var2 = var2 + var1 * ((double)dig_pressure_raw[4]) * 2.0;
	var2 = (var2/4.0) + (((double)dig_pressure_raw[3]) * 65536);
	var1 = (((double)dig_pressure_raw[2])*var1*var1/524288.0+((double)dig_pressure_raw[1])*var1)/524288.0;
	var1 = (1.0+var1/32768.0)*((double)((uint16_t)dig_pressure_raw[0]));
	if(var1 == 0.0)
		return 0;	// avoid exception caused by division by zero
	P = 1048576.0 - (double)adc_P;
	P = (P - (var2/4096.0)) * 6250.0 / var1;
	var1 = ((double)dig_pressure_raw[8]) * P * P /2147483648.0;
	var2 = P * ((double)dig_pressure_raw[7]) / 32768.0;
	P = P + (var1 + var2 + ((double)dig_pressure_raw[6])) / 16.0 ;
	
	return P;
}

float get_altitude(double pressure, double pressure_reference) {
	return 44330.0 * (1-pow(pressure/pressure_reference, (1/5.255)));
}
