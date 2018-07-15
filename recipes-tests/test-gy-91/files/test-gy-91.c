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

#include "gy-91.h"

#define GYR_DEADBAND 1		// [deg/s]
#define RAD_TO_DEG 57.29578
#define PI 3.14159265358979323846
// Complementary filter
#define DT 0.005	// gyro sample = 200 Hz
//#define AA 0.97		// complementary filter constant

// get us of the time of date
int mymicros()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}
// get ms of the time of date
int mymillis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec)/1000;
}

int main(int argc, char *argv[])
{
	uint8_t id = 0;		// ID des composants
	int test_select = 0;
	int calib_mpu = 0; 
	int calib_ak = 0;
	int nineDOF = 0;
	int print_value = 0;
	unsigned int print_interval = 500;
	int sensor_value = 0;
	// fichier d'ecriture
	int output_file;
	unsigned long int sample = 0;
	uint32_t sumCount = 0, count = 0, delt_t = 0;
	
	// calculate integration interval
	uint32_t lastUpdate = 0, Now = 0;
	float sum =0.0f;

	float mpu_temp;		// mpu's temperature

	float SelfTest[6];	// holds results of gyro and accelerometer self test

	// calibration variables
	float magCalibration[3] = {0};
	float magBias[3] = {0};
	float magScale[3] = {0};
	float gyroBias[3] = {0};
	float accelBias[3] = {0};

	float hard_iron[3];
	float soft_iron[3];

	//get raw data
	int16_t AccelRawData[3];
	int16_t GyroRawData[3];
	int16_t MagRawData[3];

	// Convert into floats
	float ax,ay,az;
	float gx,gy,gz;
	float mx,my,mz;

	//complementary filter variables
	float AccXangle;
	float AccYangle;
	float AccZangle;
	float MagXangle;
	float MagYangle;
	float MagZangle;
	float CFX;
	float CFY;
	float CFZ;
	int startInt;

	// angles 
	float roll;
	float pitch;
	float yaw;

	// barometer
	uint8_t pressure[3];
	int16_t dig_pressure_raw[9], dig_temp_raw[3];
	double pres = 0, bmp_temp = 0;
	double altitude;
	double pressure_reference = 101325.;	// pressure reference (Pa) to get altitude


// setup
	system("mknod /dev/mpu9250 c 231 0");		// create char mode mpu_file
	mpu_file = open("/dev/mpu9250", O_RDWR);	// open file corresponding to mpu driver
	system("mknod /dev/bmp280 c 232 0");		// create char mode mpu_file
	bmp_file = open("/dev/bmp280", O_RDWR);		// open file corresponding to bmp driver

	getAparam();		// get aRes and aConfig
	getGparam();		// get gRes and gConfig
	getMres();		// get mRes

	if(argc < 4) {
		printf("First argument, type of test : \n");
		printf("0 : exit\n");
		printf("1 : MPU9250 + AK8963 calibration; 9 axis quaternions\n");
		printf("2 : MPU9250 calibration; 9 axis quaternions\n");
		printf("3 : AK8963 calibration; 9 axis quaternions\n");
		printf("4 : no calibration; 9 axis quaternions\n");
		printf("5 : MPU9250 calibration; 6 axis quaternions\n");
		printf("6 : no calibration; 6 axis quaternions\n");

		printf("Second argument, type of output :\n");
		printf("1 : print sensor values\n");
		printf("2 : output for magnetometer sphere\n");
		printf("3 : output 3D cube\n");
		printf("4 : output for arrow\n");

		printf("Third argument, print interval in ms\n");
		exit(1);
	}
	// convert to int
	test_select = atoi(argv[1]);
	sensor_value = atoi(argv[2]);
	print_interval = atoi(argv[3]);
	switch(test_select) {
		case 0:
			exit(1);
			break;
		case 1:
			calib_mpu = 1;
			calib_ak = 1;
			nineDOF = 1;
			break;
		case 2:
			calib_mpu = 1;
			calib_ak = 0;
			nineDOF = 1;
			break;
		case 3:
			calib_mpu = 0;
			calib_ak = 1;
			nineDOF = 1;
			break;
		case 4:
			calib_mpu = 0;
			calib_ak = 0;
			nineDOF = 1;
			break;
		case 5:
			calib_mpu = 1;
			calib_ak = 0;
			nineDOF = 0;
			break;
		case 6:
			calib_mpu = 0;
			calib_ak = 0;
			nineDOF = 0;
			break;
		default:
			printf("forbidden value. First argument, type of test :\n");
			printf("0 : exit\n");
			printf("1 : MPU9250 + AK8963 calibration; 9 axis quaternions\n");
			printf("2 : MPU9250 calibration; 9 axis quaternions\n");
			printf("3 : AK8963 calibration; 9 axis quaternions\n");
			printf("4 : no calibration; 9 axis quaternions\n");
			printf("5 : MPU9250 calibration; 6 axis quaternions\n");
			printf("6 : no calibration; 6 axis quaternions\n");
			exit(1);
			break;
	}
	switch(sensor_value) {
		case 1:
			print_value = 1;
			break;
		case 2:
			print_value = 2;
			break;
		case 3:
			print_value = 3;
			break;
		case 4:
			print_value = 4;
			break;
		default:
			printf("forbidden value. Second argument type of output :\n");
			printf("1 : print sensor values\n");
			printf("2 : output for magnetometer sphere\n");
			printf("3 : output 3D cube\n");
			printf("4 : output for arrow\n");
			exit(1);
			break;
	}

// BMP280 Barometer
	ioctl(bmp_file, BMP280_WHOAMI, &id);  // Read WHO_AM_I register for BMP280
	printf("BMP280 ID read is %02X\n", id);
	if(id != 0x58) {			// WHO_AM_I should always be 0x58
		printf("Could not connect to BMP280: ID read is %02X instead of 0x58, reboot device may correct issue\n", id);
		return (-1);
	}
	printf("BMP280 is online...\n");
	initBMP280(dig_temp_raw, dig_pressure_raw);
	// get initial pressure to get reference altitude
	pressure_reference = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw); // fisrt measure is wrong (=68784.847177), don't understand why
	sleep(1);
	pressure_reference = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw); // second measure is right

// MPU9250 IMU
	ioctl(mpu_file, GET_IMU_WIA, &id);  // Read WHO_AM_I register for AK8963
	printf("MPU9250 ID read is %02X\n", id);
	if (id != 0x73) 			// WHO_AM_I should always be 0x73
	{
		printf("Could not connect to MPU9250: ID read is %02X instead of 0x73, reboot device may correct issue\n", id);
		return (-1);
	}
	printf("MPU9250 is online...\n");
	MPU9250SelfTest(SelfTest); // Start by performing self test and reporting values
	printf("x-axis self test: acceleration trim within : %f percent of factory value\n", SelfTest[0]);
	printf("y-axis self test: acceleration trim within : %f percent of factory value\n", SelfTest[1]);
	printf("z-axis self test: acceleration trim within : %f percent of factory value\n", SelfTest[2]);
	printf("x-axis self test: gyration trim within : %f percent of factory value\n", SelfTest[3]);
	printf("y-axis self test: gyration trim within : %f percent of factory value\n", SelfTest[4]);
	printf("z-axis self test: gyration trim within : %f percent of factory value\n", SelfTest[5]);
	sleep(1);
	initMPU9250();
	sleep(1);
	// calibrateMPU9250
	if(calib_mpu == 1) {
		calibrateMPU9250(gyroBias, accelBias); // Calibrate gyro and accelerometers, load biases in bias registers
	}

// AK8963 Magnetometer
	// init i2c bus for magnetometer
	enableI2CMaster();
	sleep(1);			// 1s
	// reset AK8963 to avoid wrong ID
	writeAK8963(AK8963_I2C_ADDR, AK8963_CNTL2, 0x01);
	sleep(1);			// 1s

	// Read WHO_AM_I register for AK8963
	readAK8963(AK8963_I2C_ADDR, AK8963_WIA, &id, 1);
	printf("AK8963 ID read is %02X\n", id);
	if(id != 0x48) {
		printf("Could not connect to AK8963: ID read is %02X instead of 0x48, reboot device may correct issue\n", id);
		//return (-1);
	}
	printf("AK8963 is online...\n");
	initAK8963(magCalibration);	// init magnetometer and get calibration value (scale)
	sleep(1);
	printf("AK8963 initialized for active data mode....\n"); // Initialize device for active mode read of magnetometer

	//  printf("Calibration values: ");
	printf("X-Axis sensitivity adjustment value %f\n", magCalibration[0]);
	printf("Y-Axis sensitivity adjustment value %f\n", magCalibration[1]);
	printf("Z-Axis sensitivity adjustment value %f\n", magCalibration[2]);
	sleep(1);
	// Get magnetometer calibration from AK8963 ROM
	if(calib_ak == 1) {
		calibrateAK8963(magCalibration, hard_iron, soft_iron);
	}

	count = mymillis();	// setup time
	// open result file
	/*if ((output_file = open("/media/card/testimu.csv", O_CREAT | O_RDWR, 0644)) < 0)	// create file on SD card
	{
		printf("cannot create file /media/card/testimu.csv\n");
		return (-1);
	}
	dprintf(output_file, "Sample,ax,ay,az,gx,gy,gz,mx,my,mz,pitch,roll,yaw,altitude\n");*/

// loop
	while(1)
	{
		startInt  = mymillis();	// pour obtenir le deltat entre chaque lecture des capteur, cet information est utilisé pour le calcul des quaternions dans la bibliothèque
		// accelerometer
		readAccelData(AccelRawData);
		// gyroscope
		readGyroData(GyroRawData);
		// magnetometer
		readMagData(MagRawData);
// Accelerometer
		// convert into g
		ax=(float) (AccelRawData[0])*aRes;	// g
		ay=(float) (AccelRawData[1])*aRes;	// g
		az=(float) (AccelRawData[2])*aRes;	// g
		// remove zero offset
		/*ax = ax - accelBias[0];
		ay = ay - accelBias[1];
		az = az - accelBias[2];*/
// Gyroscope
		// convert into deg/s
		gx=(float) (GyroRawData[0])*gRes;	// deg/s
		gy=(float) (GyroRawData[1])*gRes;	// deg/s
		gz=(float) (GyroRawData[2])*gRes;	// deg/s

		if(calib_mpu ==1) {
			// remove zero offset
			gx = gx - gyroBias[0];
			gy = gy - gyroBias[1];
			gz = gz - gyroBias[2];
			// Dead-Band
			if(abs(gx) < GYR_DEADBAND)
				gx = 0;
			if(abs(gy) < GYR_DEADBAND)
				gy = 0;
			if(abs(gz) < GYR_DEADBAND)
				gz = 0;
		}
// Magnetometer
		// User environmental axis correction in milliGauss, should be automatically calculated with the eight
		if(calib_ak == 1) {	// if we calibrate the magnetometer, use the value
			magBias[0] = hard_iron[0];
			magBias[1] = hard_iron[1];
			magBias[2] = hard_iron[2];
		} else {		// else use chosen value
			magBias[0] = 0.;
			magBias[1] = 0.;
			magBias[2] = 0.;
		}
		// Calculate the magnetometer values in milliGauss
		mx = ((float)(MagRawData[0])*mRes*magCalibration[0] - magBias[0]);	// mGauss
		my = ((float)(MagRawData[1])*mRes*magCalibration[1] - magBias[1]);	// mGauss
		mz = ((float)(MagRawData[2])*mRes*magCalibration[2] - magBias[2]);	// mGauss
		
		// calcul de l'interval pour le calcul des quaternion dans la bibliothèque
		Now = mymillis();
		deltat = ((Now - lastUpdate)/1000.); // set integration time by time elapsed since last filter update
		lastUpdate = Now;
		sum = sum + deltat;
		sumCount++;

// AHRS mahony and madgwick algorithm
		if(nineDOF == 1)	// if we use magnetometer -> 9 axis
			MahonyAHRSupdate((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az,mx,my,mz);
			//MadgwickAHRSupdate((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az,mx,my,mz);
		else			// else -> 6 axis
			MahonyAHRSupdateIMU((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az);
			//MadgwickAHRSupdateIMU((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az);

// ci dessous, beaucoup de code en commentaire car utilisé pour testé différente manière d'obtenir les résultats finaux. les lignes non commenté sont celles qui ont donné les meilleurs résultats
// calculate angles
		/*yaw = atan2(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);
		pitch = -asin(2.0f * (q1 * q3 - q0 * q2));
		roll = atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3);*/

		roll = -atan2(2*q1*q2 - 2*q0*q3, 2*q0*q0 + 2*q1*q1 - 1);
		pitch = asin(2*q1*q3 + 2*q0*q2);
		yaw = -atan2(2*q2*q3 - 2*q0*q1, 2*q0*q0 + 2*q3*q3 - 1);

		/*yaw = atan2(2.0f*q1*q2 - 2.0f*q0*q3, 2.0f*q0*q0 + 2.0f*q1*q1 - 1.0f);
		pitch = -asin(2.0f*q1*q3 + 2.0f*q0*q2);
		roll = atan2(2.0f*q2*q3 - 2.0f*q0*q1, 2.0f*q0*q0 + 2.0f*q3*q3 -1.0f);*/
// AHRS_filterUupdate
/*		filterUpdate((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az,mx,my,mz);
		yaw = atan2(2.0f*SEq_2*SEq_3 - 2.0f*SEq_1*SEq_4, 2.0f*SEq_1*SEq_1 + 2.0f*SEq_2*SEq_2 - 1.0f);
		pitch = -asin(2.0f*SEq_2*SEq_4 + 2.0f*SEq_1*SEq_3);
		roll = atan2(2.0f*SEq_3*SEq_4 - 2.0f*SEq_1*SEq_2, 2.0f*SEq_1*SEq_1 + 2.0f*SEq_4*SEq_4 -1.0f);*/

		pitch *= 180.0f / PI;
		yaw *= 180.0f / PI;
		yaw -= 0.57; // Declination at Paris, France is 0 degrees 34 minutes and 12 seconds EAST on 2017-10-26
		roll *= 180.0f / PI;

// Complementary filer
		/*AccXangle = (atan2(ay,az)+M_PI)*RAD_TO_DEG;
		AccYangle = (atan2(ax,az)+M_PI)*RAD_TO_DEG;
		AccZangle = (atan2(ax,ay)+M_PI)*RAD_TO_DEG;

		MagXangle = (atan2(my,mz)+M_PI)*RAD_TO_DEG;
		MagYangle = (atan2(mx,mz)+M_PI)*RAD_TO_DEG;
		MagZangle = (atan2(mx,my)+M_PI)*RAD_TO_DEG;

		CFX = AA*(CFX+gx*DT) +(1 - AA) * AccXangle;
		CFY = AA*(CFY+gy*DT) +(1 - AA) * AccYangle;
		CFZ = AA*(CFZ+gz*DT) +(1 - AA) * MagZangle;*/

		// temperature
		mpu_temp = mpu9250_get_temperature();
		// Barometer
		bmp_temp = bmp280_get_temperature(dig_temp_raw, NULL);
		pres = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw);
		altitude = get_altitude(pres, pressure_reference);

		if((mymillis() - count) > print_interval) {	// print every print_interval ms
			if(print_value == 1) {
				printf("X-acceleration: %f mg\n", 1000*ax);
				printf("Y-acceleration: %f mg\n", 1000*ay);
				printf("Z-acceleration: %f mg\n", 1000*az);
				printf("X-gyro rate: %f dps\n", gx);
				printf("Y-gyro rate: %f dps\n", gy);
				printf("Z-gyro rate: %f dps\n", gz);
				printf("X-mag field: %f mGauss\n", mx);
				printf("Y-mag field: %f mGauss\n", my);
				printf("Z-mag field: %f mGauss\n", mz);
				printf("Temperature from MPU9250 is %f degrees C\n", mpu_temp);
				printf("q0 = %f\nq1 = %f\nq2 = %f\nq3 = %f\n", q0, q1, q2, q3);
				printf("rpy : %f,%f,%f\n", roll, pitch, yaw);
				printf("Temperature from BMP280 is : %f degrees C\n", bmp_temp);
				printf("Pressure is %f hPa\n",pres/100);
				printf("Altitude is %f meters\n", altitude);
			} else if(print_value == 2) {
				printf("%f,%f,%f\n", mx, my, mz); // Magnetometer_Calibration output
			} else if (print_value == 3) {
				printf("%f,%f,%f,%f,%f,%f,%f,%f\n", q0,q1,q2,q3, roll*PI/180.0, pitch*PI/180.0, yaw*PI/180.0, pres); // avr_sample_imu10dof01_01 output
			} else if (print_value == 4) {
			printf("%f,%f,%f,%f\n", pitch, roll, yaw, altitude); // IMU_3D_arrow output with quaternions
			}
			//dprintf(output_file,"%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", sample, ax, ay, az, gx, gy, gz, mx, my, mz, pitch, roll, yaw, altitude); // output for CSV file
			sample=sample+print_interval; // iteration for CSV file
			count = mymillis();
		}
	}
	return 0;
}
