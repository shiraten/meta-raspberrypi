#ifndef AK8963_H_
#define AK8963_H_

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

// AK8963 register for read/write AK8963 function
// Read-only Reg
#define AK8963_I2C_ADDR 			0x0C // slave address for the AK8963
#define AK8963_DEVICE_ID 			0x48
#define AK8963_WIA 				0x00
#define AK8963_INFO 				0x01
#define AK8963_ST1 				0x02 // data ready status bit 0
#define AK8963_XOUT_L 				0x03 // data
#define AK8963_XOUT_H 				0x04
#define AK8963_YOUT_L 				0x05
#define AK8963_YOUT_H 				0x06
#define AK8963_ZOUT_L 				0x07
#define AK8963_ZOUT_H 				0x08
#define AK8963_ST2 				0x09 // Data overflow bit 3 and data read error status bit 2
#define AK8963_CNTL 				0x0A // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK8963_ASTC 				0x0C // Self test control
#define AK8963_I2CDIS 				0x0F // I2C disable
#define AK8963_ASAX 				0x10 // Fuse ROM x-axis sensitivity adjustment value
#define AK8963_ASAY 				0x11 // Fuse ROM y-axis sensitivity adjustment value
#define AK8963_ASAZ 				0x12 // Fuse ROM z-axis sensitivity adjustment value

// Write/Read Reg
#define AK8963_CNTL1 				0x0A
#define AK8963_CNTL2 				0x0B
#define AK8963_ASTC 				0x0C
#define AK8963_TS1 				0x0D
#define AK8963_TS2 				0x0E
#define AK8963_I2CDIS 				0x0F

// for slave0 use
#define SLV0_CTRL				343
#define SLV0_ADDR				344
#define SLV0_REG				345
#define SLV0_D0					346
#define SLV0_EXT_DATA_0				347

extern volatile float mRes;		// 

enum Mscale {
  MFS_14BITS = 0, // 0.6 mG per LSB
  MFS_16BITS      // 0.15 mG per LSB
};

/*functions prototypes*/
void initAK8963(float *magCalibration);
void writeAK8963(uint8_t addr, uint8_t reg, uint8_t arg);
void readAK8963(uint8_t addr, uint8_t reg, uint8_t *dest, int lenght);
void calibrateAK8963(float *MagAdjRawData, float *dest1, float *dest2);
int readMagData(int16_t *dest);
void getMres();


#endif
