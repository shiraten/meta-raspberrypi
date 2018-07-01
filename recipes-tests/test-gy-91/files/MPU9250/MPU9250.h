#ifndef MPU9250_H_
#define MPU9250_H_

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

// Commands ID
#define GET_IMU_WIA				300
#define GET_TEMPERATURE				301
#define SET_ACCEL_CONFIG			302
#define SET_ACCEL_CONFIG2			303
#define GET_ACCEL_CONFIG			304
#define GET_GYRO_CONFIG				305
#define SET_GYRO_CONFIG				306
#define GET_ACCEL_DATA				307
#define GET_GYRO_DATA				308
#define IMU_PWR_MGMT_1				309
#define IMU_PWR_MGMT_2				310
#define IMU_INT_ENABLE				311
#define IMU_FIFO_EN				312
#define IMU_I2C_MST_CTRL			313
#define IMU_USR_CTRL				314
#define IMU_CONFIG				315
#define IMU_SMPLRT_DIV				316
#define IMU_FIFO_COUNTH				317
#define IMU_XA_OFFSET_H				318
#define IMU_XA_OFFSET_L				319
#define IMU_YA_OFFSET_H				320
#define IMU_YA_OFFSET_L				321
#define IMU_ZA_OFFSET_H				322
#define IMU_ZA_OFFSET_L				323
#define IMU_XG_OFFSET_H				324
#define IMU_XG_OFFSET_L				325
#define IMU_YG_OFFSET_H				326
#define IMU_YG_OFFSET_L				327
#define IMU_ZG_OFFSET_H				328
#define IMU_ZG_OFFSET_L				329
#define IMU_GET_XA_OFFSET			330
#define IMU_GET_YA_OFFSET			331
#define IMU_GET_ZA_OFFSET			332
#define IMU_GET_FIFO_R_W			333
#define IMU_INT_PIN_CFG				334
#define GET_ACCEL_CONFIG2			335
#define IMU_INT_STATUS				336
#define IMU_SELF_TEST_X_ACCEL			337
#define IMU_SELF_TEST_Y_ACCEL			338
#define IMU_SELF_TEST_Z_ACCEL			339
#define IMU_SELF_TEST_X_GYRO			340
#define IMU_SELF_TEST_Y_GYRO			341
#define IMU_SELF_TEST_Z_GYRO			342

enum Ascale {
  AFS_2G = 0,
  AFS_4G,
  AFS_8G,
  AFS_16G
};

enum Gscale {
  GFS_250DPS = 0,
  GFS_500DPS,
  GFS_1000DPS,
  GFS_2000DPS
};

extern volatile int mpu_file;		// ioctl mpu_file

void getGparam();
void getAparam();
void initMPU9250(void);
void calibrateMPU9250(float *dest1, float *dest2);
void MPU9250SelfTest(float * destination);
void readAccelData(int16_t *dest);
void readGyroData(int16_t *dest);
float mpu9250_get_temperature(void);
extern volatile float aRes;
extern volatile float gRes; // scale resolutions per LSB for the sensors
extern volatile uint8_t aConfig;
extern volatile uint8_t gConfig;
void enableI2CMaster(void);

#endif
