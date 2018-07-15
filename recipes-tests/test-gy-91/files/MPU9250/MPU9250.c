#include "MPU9250.h"

volatile int mpu_file = 0;		// ioctl mpu_file
uint8_t Gscale = GFS_1000DPS;
uint8_t Ascale = AFS_2G;
volatile float aRes = 0.0f;
volatile float gRes = 0.0f;; // scale resolutions per LSB for the sensors
volatile uint8_t aConfig = 0;
volatile uint8_t gConfig = 0;

void getGparam() {
	switch (Gscale)
	{
		// Possible gyro scales (and their register bit settings) are:
		// 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS  (11). 
			// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
		case GFS_250DPS:
			  gRes = 250.0/32768.;
			  gConfig = 0x00;
			  break;
		case GFS_500DPS:
			  gRes = 500.0/32768.;
			  gConfig = 0x01;
			  break;
		case GFS_1000DPS:
			  gRes = 1000.0/32768.;
			  gConfig = 0x02;
			  break;
		case GFS_2000DPS:
			  gRes = 2000.0/32768.;
			  gConfig = 0x03;
			  break;
	}
}

void getAparam() {
	switch (Ascale)
	{
		// Possible accelerometer scales (and their register bit settings) are:
		// 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11). 
			// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
		case AFS_2G:
			  aRes = 2.0/32768.0;
			  aConfig = 0x00;
			  break;
		case AFS_4G:
			  aRes = 4.0/32768.0;
			  aConfig = 0x01;
			  break;
		case AFS_8G:
			  aRes = 8.0/32768.0;
			  aConfig = 0x02;
			  break;
		case AFS_16G:
			  aRes = 16.0/32768.0;
			  aConfig = 0x03;
			  break;
	}
}

void enableI2CMaster(void) {
	ioctl(mpu_file, IMU_INT_PIN_CFG, 0x00); // disable bypass
	usleep(10000);
	ioctl(mpu_file, IMU_I2C_MST_CTRL, 0x5D); // 400kHz, wait data ready, stop condition
	usleep(10000);
	ioctl(mpu_file, IMU_INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
	usleep(10000);
	ioctl(mpu_file, IMU_USR_CTRL, 0x20); /// enable AUX
	usleep(10000);
}

void initMPU9250(void) {
	uint8_t c;
	// wake up device
	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors 
	usleep(100000); // Wait for all registers to reset 

	// get stable time source
	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x01);  // Auto select clock source to be PLL gyroscope reference if ready else
	usleep(200000); 

	// Configure Gyro and Thermometer
	// Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively; 
	// minimum sleep time for this setting is 5.9 ms, which means sensor fusion update rates cannot
	// be higher than 1 / 0.0059 = 170 Hz
	// DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
	// With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz
	ioctl(mpu_file, IMU_CONFIG, 0x03);  // set DLP_CFG to 3 to get 1Khz sample rate on gyroscope

	// Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
	ioctl(mpu_file, IMU_SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; a rate consistent with the filter update rate 
		                    // determined inset in CONFIG above

	// Set gyroscope full scale range
	// Range selects FS_SEL and GFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
	ioctl(mpu_file, GET_GYRO_CONFIG, &c); // get current GYRO_CONFIG register value
	// c = c & ~0xE0; // Clear self-test bits [7:5] 
	c = c & ~0x03; // Clear Fchoice bits [1:0] 
	c = c & ~0x18; // Clear GFS bits [4:3] -> 250dps
	c = c | Gscale << 3; // Set full scale range for the gyro
	//c = c | 0x03; // set fchoice[1:0] to 11, then in IMU_CONFIG, set DLP_CFG to 3 to get 1Khz | BW = 41Hz sample rate on gyroscope
	ioctl(mpu_file, SET_GYRO_CONFIG, c); // Write new GYRO_CONFIG value to register

	// Set accelerometer full-scale range configuration
	ioctl(mpu_file, GET_ACCEL_CONFIG, &c); // get current ACCEL_CONFIG register value
	// c = c & ~0xE0; // Clear self-test bits [7:5] 
	c = c & ~0x18;  // Clear AFS bits [4:3]
	c = c | Ascale << 3; // Set full scale range for the accelerometer 
	ioctl(mpu_file, SET_ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

	// Set accelerometer sample rate configuration
	// It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
	// accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
	ioctl(mpu_file, GET_ACCEL_CONFIG2, &c); // get current ACCEL_CONFIG2 register value
	c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])  
	c = c | 0x03;  // set fchoice to 1 and set accelerometer rate to 1 kHz and bandwidth to 41 Hz
	ioctl(mpu_file, SET_ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value
	// The accelerometer, gyro, and thermometer are set to 1 kHz sample rates, 
	// but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

	// Configure Interrupts and Bypass Enable
	// Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
	// clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips 
	// can join the I2C bus and all can be controlled by the Arduino as master
	ioctl(mpu_file, IMU_INT_PIN_CFG, 0x20);	// disable bypass
	ioctl(mpu_file, IMU_INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
	usleep(100000);
}

void readAccelData(int16_t *dest) {
	uint8_t data[6];
	int i;
	ioctl(mpu_file, GET_ACCEL_DATA, data);
	dest[0] = ((int16_t)data[0] << 8) | data[1];
	dest[1] = ((int16_t)data[2] << 8) | data[3];
	dest[2] = ((int16_t)data[4] << 8) | data[5];
}

void readGyroData(int16_t *dest) {
	uint8_t data[6];
	int i;
	ioctl(mpu_file, GET_GYRO_DATA, data);
	dest[0] = ((int16_t)data[0] << 8) | data[1];
	dest[1] = ((int16_t)data[2] << 8) | data[3];
	dest[2] = ((int16_t)data[4] << 8) | data[5];
}

float mpu9250_get_temperature() {
	float temperature;
	uint8_t TempRawData[2];
	ioctl(mpu_file, GET_TEMPERATURE, TempRawData);
	temperature = (float) ((TempRawData[0] << 8) | TempRawData[1]);
	temperature = temperature / 333.87 + 21.0;
	return temperature;
}

void MPU9250SelfTest(float * destination) // Should return percent deviation from factory trim values, +/- 14 or less deviation is a pass
{
	uint8_t rawData[6] = {0, 0, 0, 0, 0, 0};
	uint8_t selfTest[6];
	uint8_t selftest_ax,selftest_ay,selftest_az,selftest_gx,selftest_gy,selftest_gz;
	int32_t gAvg[3] = {0}, aAvg[3] = {0}, aSTAvg[3] = {0}, gSTAvg[3] = {0};
	float factoryTrim[6];
	uint8_t FS = 0;
	int i, ii;

	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x80);	// reset internal register and restore default settings

	ioctl(mpu_file, IMU_SMPLRT_DIV, 0x00);    // Set gyro sample rate to 1 kHz
	ioctl(mpu_file, IMU_CONFIG, 0x02);        // Set gyro sample rate to 1 kHz and DLPF to 92 Hz
	ioctl(mpu_file, SET_GYRO_CONFIG, FS<<3);  // Set full scale range for the gyro to 250 dps
	ioctl(mpu_file, SET_ACCEL_CONFIG2, 0x02); // Set accelerometer rate to 1 kHz and bandwidth to 92 Hz
	ioctl(mpu_file, SET_ACCEL_CONFIG, FS<<3); // Set full scale range for the accelerometer to 2 g

	for( ii = 0; ii < 200; ii++) {  // get average current values of gyro and acclerometer

		ioctl(mpu_file, GET_ACCEL_DATA, rawData);        // Read the six raw data registers into data array
		aAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
		aAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;  
		aAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ; 

		ioctl(mpu_file, GET_GYRO_DATA, rawData);       // Read the six raw data registers sequentially into data array
		gAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
		gAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;  
		gAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ; 
	}
	for (ii =0; ii < 3; ii++) {  // Get average of 200 values and store as average current readings
		aAvg[ii] /= 200;
		gAvg[ii] /= 200;
	}
	// Configure the accelerometer for self-test
	ioctl(mpu_file, SET_ACCEL_CONFIG, 0xE0); // Enable self test on all three axes and set accelerometer range to +/- 2 g
	ioctl(mpu_file, SET_GYRO_CONFIG,  0xE0); // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
	usleep(25000);  // sleep a while to let the device stabilize
	for(ii = 0; ii < 200; ii++) {  // get average self-test values of gyro and acclerometer

		ioctl(mpu_file, GET_ACCEL_DATA, rawData);   // Read the six raw data registers into data array
		aSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
		aSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;  
		aSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ; 

		ioctl(mpu_file, GET_GYRO_DATA, rawData);  // Read the six raw data registers sequentially into data array
		gSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
		gSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;  
		gSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ; 
	}

	for (ii =0; ii < 3; ii++) {  // Get average of 200 values and store as average self-test readings
		aSTAvg[ii] /= 200;
		gSTAvg[ii] /= 200;
	}
	// Configure the gyro and accelerometer for normal operation
	ioctl(mpu_file, SET_ACCEL_CONFIG, 0x00);  
	ioctl(mpu_file, SET_GYRO_CONFIG,  0x00);
	usleep(25000);  // sleep a while to let the device stabilize
	
	// Retrieve accelerometer and gyro factory Self-Test Code from USR_Reg
	ioctl(mpu_file, IMU_SELF_TEST_X_ACCEL, &selftest_ax); // X-axis accel self-test results
	selfTest[0] = selftest_ax;
	ioctl(mpu_file, IMU_SELF_TEST_Y_ACCEL, &selftest_ay); // Y-axis accel self-test results
	selfTest[1] = selftest_ay;
	ioctl(mpu_file, IMU_SELF_TEST_Z_ACCEL, &selftest_az); // Z-axis accel self-test results
	selfTest[2] = selftest_az;
	ioctl(mpu_file, IMU_SELF_TEST_X_GYRO, &selftest_gx);  // X-axis gyro self-test results
	selfTest[3] = selftest_gx;
	ioctl(mpu_file, IMU_SELF_TEST_Y_GYRO, &selftest_gy);  // Y-axis gyro self-test results
	selfTest[4] = selftest_gy;
	ioctl(mpu_file, IMU_SELF_TEST_Z_GYRO, &selftest_gz);  // Z-axis gyro self-test results
	selfTest[5] = selftest_gz;

	// Retrieve factory self-test value from self-test code reads
	factoryTrim[0] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[0] - 1.0) )); // FT[Xa] factory trim calculation
	factoryTrim[1] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[1] - 1.0) )); // FT[Ya] factory trim calculation
	factoryTrim[2] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[2] - 1.0) )); // FT[Za] factory trim calculation
	factoryTrim[3] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[3] - 1.0) )); // FT[Xg] factory trim calculation
	factoryTrim[4] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[4] - 1.0) )); // FT[Yg] factory trim calculation
	factoryTrim[5] = (float)(2620/1<<FS)*(pow( 1.01 , ((float)selfTest[5] - 1.0) )); // FT[Zg] factory trim calculation

	// Report results as a ratio of (STR - FT)/FT; the change from Factory Trim of the Self-Test Response
	
	// To get percent, must multiply by 100
	for (i = 0; i < 3; i++) {
		destination[i]   = 100.0*((float)(aSTAvg[i] - aAvg[i]))/factoryTrim[i] - 100.;   // Report percent differences
		destination[i+3] = 100.0*((float)(gSTAvg[i] - gAvg[i]))/factoryTrim[i+3] - 100.; // Report percent differences
	}
}

/*void calibrateMPU9250(float *dest1, float *dest2) {
	uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
	uint16_t ii, packet_count, fifo_count;
	int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

	printf("calibrate gyroscope and accelerometer. Please don't move the device\n");
	sleep(3);
	// reset device
	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x80);
	usleep(100000);

	// get stable time source; Auto select clock source to be PLL gyroscope reference if ready 
	// else use the internal oscillator, bits 2:0 = 001
	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x01);
	ioctl(mpu_file, IMU_PWR_MGMT_2, 0x00);
	usleep(200000);                                    

	// Configure device for bias calculation
	ioctl(mpu_file, IMU_INT_ENABLE, 0x00);  	// Disable all interrupts
	ioctl(mpu_file, IMU_FIFO_EN, 0x00);   	// Disable FIFO
	ioctl(mpu_file, IMU_PWR_MGMT_1, 0x00);   	// Turn on internal clock source
	ioctl(mpu_file, IMU_I2C_MST_CTRL, 0x5D); 	// 400kHz, wait data ready, stop btw read
	ioctl(mpu_file, IMU_USR_CTRL, 0x00);	// Disable FIFO and I2C master modes
	ioctl(mpu_file, IMU_USR_CTRL, 0x0C);	// Reset FIFO and DMP
	ioctl(mpu_file, IMU_USR_CTRL, 0x20);	// enable AUX
	usleep(15000);

	// Configure MPU6050 gyro and accelerometer for bias calculation
	ioctl(mpu_file, IMU_CONFIG, 0x01);      // Set low-pass filter to 188 Hz
	ioctl(mpu_file, IMU_SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
	ioctl(mpu_file, SET_GYRO_CONFIG, gConfig);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
	ioctl(mpu_file, SET_ACCEL_CONFIG, aConfig); // Set accelerometer full-scale to 2 g, maximum sensitivity

	uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
	uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

	// Configure FIFO to capture accelerometer and gyro data for bias calculation
	ioctl(mpu_file, IMU_USR_CTRL, 0x40);   // Enable FIFO  
	ioctl(mpu_file, IMU_FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
	usleep(40000); // accumulate 40 samples in 40 milliseconds = 480 bytes
	// At end of sample accumulation, turn off FIFO sensor read
	ioctl(mpu_file, IMU_FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO

// fifo_count = 0 ; buffer always empty?

	ioctl(mpu_file, IMU_FIFO_COUNTH, data); // read FIFO sample count
	//printf("%02X\t%02x\n", data[0], data[1]);
	fifo_count = ((uint16_t)data[0] << 8) | data[1];
	packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging
	for (ii = 0; ii < packet_count; ii++) {
		int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
		ioctl(mpu_file, IMU_GET_FIFO_R_W, data); // read data for averaging
		accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
		accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
		accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;    
		gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
		gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
		gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;
		accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
		accel_bias[1] += (int32_t) accel_temp[1];
		accel_bias[2] += (int32_t) accel_temp[2];
		gyro_bias[0]  += (int32_t) gyro_temp[0];
		gyro_bias[1]  += (int32_t) gyro_temp[1];
		gyro_bias[2]  += (int32_t) gyro_temp[2];
	}
	accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
	accel_bias[1] /= (int32_t) packet_count;
	accel_bias[2] /= (int32_t) packet_count;
	gyro_bias[0]  /= (int32_t) packet_count;
	gyro_bias[1]  /= (int32_t) packet_count;
	gyro_bias[2]  /= (int32_t) packet_count;

	if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) accelsensitivity;}  // Remove gravity from the z-axis accelerometer bias calculation
	else {accel_bias[2] += (int32_t) accelsensitivity;}
	// Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
	data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
	data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
	data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
	data[3] = (-gyro_bias[1]/4)       & 0xFF;
	data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
	data[5] = (-gyro_bias[2]/4)       & 0xFF;

	// Push gyro biases to hardware registers
	ioctl(mpu_file, IMU_XG_OFFSET_H, data[0]);
	ioctl(mpu_file, IMU_XG_OFFSET_L, data[1]);
	ioctl(mpu_file, IMU_YG_OFFSET_H, data[2]);
	ioctl(mpu_file, IMU_YG_OFFSET_L, data[3]);
	ioctl(mpu_file, IMU_ZG_OFFSET_H, data[4]);
	ioctl(mpu_file, IMU_ZG_OFFSET_L, data[5]);
	// Output scaled gyro biases for display in the main program
	dest1[0] = (float) gyro_bias[0]/(float) gyrosensitivity;  
	dest1[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
	dest1[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

	// Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
	// factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
	// non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
	// compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
	// the accelerometer biases calculated above must be divided by 8.

	int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
	ioctl(mpu_file, IMU_GET_XA_OFFSET, data); // Read factory accelerometer trim values
	accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
	ioctl(mpu_file, IMU_GET_YA_OFFSET, data);
	accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
	ioctl(mpu_file, IMU_GET_ZA_OFFSET, data);
	accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
	uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
	uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

	for(ii = 0; ii < 3; ii++) {
		if((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
	}

	// Construct total accelerometer bias, including calculated average accelerometer bias from above
	accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
	accel_bias_reg[1] -= (accel_bias[1]/8);
	accel_bias_reg[2] -= (accel_bias[2]/8);

	data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
	data[1] = (accel_bias_reg[0])      & 0xFF;
	data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
	data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
	data[3] = (accel_bias_reg[1])      & 0xFF;
	data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
	data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
	data[5] = (accel_bias_reg[2])      & 0xFF;
	data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers
	// Apparently this is not working for the acceleration biases in the MPU-9250
	// Are we handling the temperature correction bit properly?
	// Push accelerometer biases to hardware registers
	ioctl(mpu_file, IMU_XA_OFFSET_H, data[0]);
	ioctl(mpu_file, IMU_XA_OFFSET_L, data[1]);
	ioctl(mpu_file, IMU_YA_OFFSET_H, data[2]);
	ioctl(mpu_file, IMU_YA_OFFSET_L, data[3]);
	ioctl(mpu_file, IMU_ZA_OFFSET_H, data[4]);
	ioctl(mpu_file, IMU_ZA_OFFSET_L, data[5]);
	// Output scaled accelerometer biases for display in the main program
	dest2[0] = (float)accel_bias[0]/(float)accelsensitivity; 
	dest2[1] = (float)accel_bias[1]/(float)accelsensitivity;
	dest2[2] = (float)accel_bias[2]/(float)accelsensitivity;
}*/

void calibrateMPU9250(float *dest1, float *dest2) {
	int16_t GyrRawData[3] = {0};
	int16_t gyr_bias[3] = {0};
	int16_t gyr_min[3] = {0};	// déclaré à 0. sinon on peut avoir des valeurs de l'ordre de -50000
	int16_t gyr_max[3] = {0};
	int16_t avg_rad = 0;
	int i, j, sample_count;

	// init gyr_max et gyr_min. sinon, on risque d'avoir une valeur min à 0 a cause de la valeur de déclaration
	printf("IMU Calibration please do not move device\n");
	sleep(4);
	readGyroData(GyrRawData);
	for (j = 0; j < 3; j++) {
		gyr_max[j] = GyrRawData[j];
		gyr_min[j] = GyrRawData[j];
	}

	// shoot for ~fifteen seconds of gyr data
	sample_count = 1500;  // at 100 Hz ODR, new gyro data is available every 10 ms
	for(i = 0; i < sample_count; i++) {
		readGyroData(GyrRawData);
		for (j = 0; j < 3; j++) {
			if(GyrRawData[j] > gyr_max[j]) gyr_max[j] = GyrRawData[j];
			if(GyrRawData[j] < gyr_min[j]) gyr_min[j] = GyrRawData[j];
			//printf("raw=%d\n", GyrRawData[j]);
		}
		usleep(12000);  // at 100 Hz ODR, new gyr data is available every 10 ms
	}
	//printf("max:\nx= %d\ny=%d\nz=%d\nmin:x=%d\ny=%d\nz=%d\n", gyr_max[0], gyr_max[1], gyr_max[2], gyr_min[0], gyr_min[1], gyr_min[2]);
	// Get hard iron correction (offset/bias correction)
	gyr_bias[0]  = (gyr_max[0] + gyr_min[0])/2;  // get average x gyr bias in counts
	gyr_bias[1]  = (gyr_max[1] + gyr_min[1])/2;  // get average y gyr bias in counts
	gyr_bias[2]  = (gyr_max[2] + gyr_min[2])/2;  // get average z gyr bias in counts
	//printf("bias:\nx= %d\ny=%d\nz=%d\n", gyr_bias[0], gyr_bias[1], gyr_bias[2]);

	dest1[0] = (float)(gyr_bias[0])*gRes;  // save gyr biases in deg/sec for main program
	dest1[1] = (float)(gyr_bias[1])*gRes;
	dest1[2] = (float)(gyr_bias[2])*gRes;

	printf("IMU Calibration done!\n");
}
