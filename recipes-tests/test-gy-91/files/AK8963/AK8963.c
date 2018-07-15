#include "AK8963.h"
#include "../MPU9250/MPU9250.h"

uint8_t Mscale = MFS_16BITS; // Choose either 14-bit or 16-bit magnetometer resolution
uint8_t Mmode = 0x06;        // 2 for 8 Hz, 6 for 100 Hz continuous magnetometer data read

volatile float mRes = 0.0f;

void getMres() {
	switch (Mscale)
	{
		// Possible magnetometer scales (and their register bit settings) are:
		// 14 bit resolution (0) and 16 bit resolution (1)
		case MFS_14BITS:
			  mRes = 10.*4912./8190.; // Proper scale to return milliGauss
			  break;
		case MFS_16BITS:
			  mRes = 10.*4912./32760.; // Proper scale to return milliGauss
			  break;
	}
}

void initAK8963(float *destination) {
	uint8_t rawData[3];

	//get calibrations values
	// sleep
	writeAK8963(AK8963_I2C_ADDR, AK8963_CNTL, 0x00);
	usleep(10000);				//10ms
	// access fuse
	writeAK8963(AK8963_I2C_ADDR, AK8963_CNTL, 0x0F);
	usleep(10000);				//10ms

	readAK8963(AK8963_I2C_ADDR, AK8963_ASAX, rawData, 3);	// get calibration values
	destination[0] = (float)(rawData[0]-128)/256.+1.;
	destination[1] = (float)(rawData[1]-128)/256.+1.;
	destination[2] = (float)(rawData[2]-128)/256.+1.;

	//printf("initAK8963 %02X\t%02X\t%02X\n", rawData[0], rawData[1], rawData[2]);

	//prepare to get data
	// sleep
	writeAK8963(AK8963_I2C_ADDR, AK8963_CNTL, 0x00);
	usleep(10000);				// 10ms
	// mode + scale
	writeAK8963(AK8963_I2C_ADDR, AK8963_CNTL, ((Mscale << 4) | Mmode));
	usleep(10000);
}

void readAK8963(uint8_t addr, uint8_t reg, uint8_t *dest, int lenght) {
	//printf("read : addr %02X ; reg %02X ; lenght %d\n", addr, reg, lenght);
	uint8_t data[lenght];
	int i;
	// disable slave to configure
	ioctl(mpu_file, SLV0_CTRL, 0x00);
	// set AK8963 device addr + read
	ioctl(mpu_file, SLV0_ADDR, (0x80 | addr));
	// set AK8963 register
	ioctl(mpu_file, SLV0_REG, reg);
	// reset SLV0_EXT_DATA_ register
	ioctl(mpu_file, SLV0_D0, 0x00);
	// enable I2C and set lenght
	ioctl(mpu_file, SLV0_CTRL, (0x80 | lenght));
	usleep(2000); // update SLV0_EXT_DATA_0 at 1000Hz (see SMPLRT_DIV register)
	// STOP I2C
	//ioctl(mpu_file, SLV0_CTRL, 0x00);
	// read result
	ioctl(mpu_file, SLV0_EXT_DATA_0, data);
	for(i=0; i < lenght; i++) {
		dest[i] = data[i];
	}
}

void writeAK8963(uint8_t addr, uint8_t reg, uint8_t arg) {
	//printf("write : addr %02X ; reg %02X ; arg %02x\n", addr, reg, arg);
	// disable slave to configure
	ioctl(mpu_file, SLV0_CTRL, 0x00);
	// set slave device addr + write
	ioctl(mpu_file, SLV0_ADDR, (0x00 | addr));
	// set slave register addr
	ioctl(mpu_file, SLV0_REG, reg);
	// data to write into ak8963 register
	ioctl(mpu_file, SLV0_D0, arg);
	// enable I2C and set 1 bytes
	ioctl(mpu_file, SLV0_CTRL, 0x81);
	// STOP I2C
	//ioctl(mpu_file, SLV0_CTRL, 0x00);
}

void calibrateAK8963(float *MagAdjValue, float *dest1, float *dest2)
{
	int16_t MagRawData[3] = {0};
	int16_t mag_bias[3] = {0};
	int16_t mag_scale[3] = {0};
	int16_t mag_min[3] = {0};	// déclaré à 0. sinon on peut avoir des valeurs de l'ordre de -50000
	int16_t mag_max[3] = {0};
	int16_t avg_rad = 0;
	int i, j, sample_count;

	// init mag_max et mag_min. sinon, on risque d'avoir une valeur min à 0 a cause de la valeur de déclaration
	readMagData(MagRawData);
	for (j = 0; j < 3; j++) {
		mag_max[j] = MagRawData[j];
		mag_min[j] = MagRawData[j];
	}

	printf("Mag Calibration: Wave device in a figure eight until done! (20s)\n");
	sleep(4);
	// shoot for ~fifteen seconds of mag data
	if(Mmode == 0x02) sample_count = 128;  // at 8 Hz ODR, new mag data is available every 125 ms
	if(Mmode == 0x06) sample_count = 1500;  // at 100 Hz ODR, new mag data is available every 10 ms
	for(i = 0; i < sample_count; i++) {
		readMagData(MagRawData);
		for (j = 0; j < 3; j++) {
			if(MagRawData[j] > mag_max[j]) mag_max[j] = MagRawData[j];
			if(MagRawData[j] < mag_min[j]) mag_min[j] = MagRawData[j];
			//printf("raw=%d\n", MagRawData[j]);
		}
		if(Mmode == 0x02) usleep(135000);  // at 8 Hz ODR, new mag data is available every 125 ms
		if(Mmode == 0x06) usleep(12000);  // at 100 Hz ODR, new mag data is available every 10 ms
	}
	//printf("max:\nx= %d\ny=%d\nz=%d\nmin:x=%d\ny=%d\nz=%d\n", mag_max[0], mag_max[1], mag_max[2], mag_min[0], mag_min[1], mag_min[2]);
	// Get hard iron correction (offset/bias correction)
	mag_bias[0]  = (mag_max[0] + mag_min[0])/2;  // get average x mag bias in counts
	mag_bias[1]  = (mag_max[1] + mag_min[1])/2;  // get average y mag bias in counts
	mag_bias[2]  = (mag_max[2] + mag_min[2])/2;  // get average z mag bias in counts
	//printf("bias:\nx= %d\ny=%d\nz=%d\n", mag_bias[0], mag_bias[1], mag_bias[2]);
	dest1[0] = (float)(mag_bias[0])*mRes*MagAdjValue[0];  // save mag biases in mG for main program
	dest1[1] = (float)(mag_bias[1])*mRes*MagAdjValue[1];
	dest1[2] = (float)(mag_bias[2])*mRes*MagAdjValue[2];
	
	// Get soft iron correction estimate (scale correction)
	mag_scale[0]  = (mag_max[0] - mag_min[0])/2;  // get average x axis max chord length in counts
	mag_scale[1]  = (mag_max[1] - mag_min[1])/2;  // get average y axis max chord length in counts
	mag_scale[2]  = (mag_max[2] - mag_min[2])/2;  // get average z axis max chord length in counts

	avg_rad = mag_scale[0] + mag_scale[1] + mag_scale[2];
	avg_rad /= 3.0;
	dest2[0] = ((float) avg_rad/((float)mag_scale[0]))*mRes;
	dest2[1] = ((float) avg_rad/((float)mag_scale[1]))*mRes;
	dest2[2] = ((float) avg_rad/((float)mag_scale[2]))*mRes;
	printf("Mag Calibration done!\n");
}

int readMagData(int16_t *dest) {
	uint8_t data[8]={0};
	uint8_t c = 0;
	readAK8963(AK8963_I2C_ADDR, AK8963_ST1, data, 8);	// wait for data ready
	c = data[0];
	//printf("%02X\n", c);
	//if(c&0x02)
	//	printf("data overrun\n");
	if(c&0x01) {
	//	printf("pass\n");
		c = data[7];	// End data read by reading ST2 register
		if(!(c & 0x08)) {	// check overflow
	//		printf("no overflow\n");
			dest[0] = (data[2] << 8) | data[1];
			dest[1] = (data[4] << 8) | data[3];
			dest[2] = (data[6] << 8) | data[5];
		}
	}
	return 0;
}
