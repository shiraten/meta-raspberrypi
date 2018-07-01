#include "gy-91.h"

int main(int argc, char *argv[])
{
	// fichier d'ecriture
	uint32_t sumCount = 0, count = 0, delt_t = 0;
	
	// calculate integration interval
	uint32_t lastUpdate = 0, Now = 0;
	float sum =0.0f;

	float mpu_temp;

	float SelfTest[6];    // holds results of gyro and accelerometer self test

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

	//complementary filter
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

	int fd;
	char shared[128] = IMU_MEM;
	imu_t * position;
	sem_t * sem;
	char semaphore[128] = IMU_SEM;
	/*ouverture du segment de mémoire*/
	if((fd = shm_open(shared, O_RDWR | O_CREAT, 0600)) == -1) {
		perror(shared);
		exit(EXIT_FAILURE);
	}
	/*taille du segment*/
	if(ftruncate(fd, sizeof(int)) != 0) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	/*projection du segment dans l'espace mémoire du processus*/
	position = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(position == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	/* ouverture du semaphore */
	sem = sem_open(semaphore, O_RDWR);
	if(sem == SEM_FAILED) {
		if(errno != ENOENT) {
			perror(IMU_SEM);
			exit(EXIT_FAILURE);
		}
		sem = sem_open(semaphore, O_RDWR | O_CREAT, 0666, 1);
		if(sem == SEM_FAILED) {
			perror(IMU_SEM);
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "[%d] Creation de %s\n", getpid(), semaphore);
	}




// setup
	system("mknod /dev/mpu9250 c 231 0");		// create char mode mpu_file
	mpu_file = open("/dev/mpu9250", O_RDWR);	// open file corresponding to mpu driver
	system("mknod /dev/bmp280 c 232 0");		// create char mode mpu_file
	bmp_file = open("/dev/bmp280", O_RDWR);		// open file corresponding to bmp driver

	getAparam();		// get aRes and aConfig
	getGparam();		// get gRes and gConfig
	getMres();		// get mRes

// BMP280 Barometer
	uint8_t b;
	ioctl(bmp_file, BMP280_WHOAMI, &b);  // Read WHO_AM_I register for BMP280
	printf("BMP280 ID read is %02X\n", b);
	if(b != 0x58) {
		printf("Could not connect to BMP280: ID read is %02X instead of 0x58, reboot device may correct issue\n", b);
		return (-1);
	}
	printf("BMP280 is online...\n");
	initBMP280(dig_temp_raw, dig_pressure_raw);
	// get initial pressure to get reference altitude
	pressure_reference = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw); // fisrt measure is wrong (=68784.847177)
	sleep(1);
	pressure_reference = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw); // second measure is right

// MPU9250 IMU
	uint8_t c;
	ioctl(mpu_file, GET_IMU_WIA, &c);  // Read WHO_AM_I register for AK8963
	printf("MPU9250 ID read is %02X\n", c);
	if (c != 0x73) // WHO_AM_I should always be 0x73
	{
		printf("Could not connect to MPU9250: ID read is %02X instead of 0x73, reboot device may correct issue\n", c);
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
	// calibrateMPU9250
	calibrateMPU9250(gyroBias, accelBias); // Calibrate gyro and accelerometers, load biases in bias registers

	sleep(1);

	initMPU9250();
	sleep(1);

// AK8963 Magnetometer
	uint8_t d;
	initAK8963(magCalibration);
	readAK8963(AK8963_I2C_ADDR, AK8963_WIA, &d, 1);  // Read WHO_AM_I register for AK8963
	printf("AK8963 ID read is %02X\n", d);
	if(d != 0x48) {
		printf("Could not connect to AK8963: ID read is %02X instead of 0x48, reboot device may correct issue\n", d);
		return (-1);
	}
	printf("AK8963 is online...\n");
	sleep(1);
	
	printf("AK8963 initialized for active data mode....\n"); // Initialize device for active mode read of magnetometer

	//  printf("Calibration values: ");
	printf("X-Axis sensitivity adjustment value %f\n", magCalibration[0]);
	printf("Y-Axis sensitivity adjustment value %f\n", magCalibration[1]);
	printf("Z-Axis sensitivity adjustment value %f\n", magCalibration[2]);
	sleep(1);
	// Get magnetometer calibration from AK8963 ROM
	calibrateAK8963(magCalibration, hard_iron, soft_iron);

	count = mymillis();	// setup time

	// procédure pour passer en mode démon (daemon) :
	pid_t pid;

	// - remonte à la racine du système pour la création d'un démon
	chdir("/");

	// - passe en arrière plan
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	// - création d'une nouvelle session
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	// - deuxième fork
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	// - fermeture de tous les descripteurs de fchiers que le shell aurait pu transmettre
	//for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
	//	close(i);

// loop
	while(1)
	{
		startInt  = mymillis();
		// temperature
		mpu_temp = mpu9250_get_temperature();
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
// Magnetometer
		// User environmental axis correction in milliGauss, should be automatically calculated with eight
		magBias[0] = hard_iron[0];
		magBias[1] = hard_iron[1];
		magBias[2] = hard_iron[2];

		// Calculate the magnetometer values in milliGauss
		mx = ((float)(MagRawData[0])*mRes*magCalibration[0] - magBias[0]);	// mGauss
		my = ((float)(MagRawData[1])*mRes*magCalibration[1] - magBias[1]);	// mGauss
		mz = ((float)(MagRawData[2])*mRes*magCalibration[2] - magBias[2]);	// mGauss
		
		Now = mymillis();
		deltat = ((Now - lastUpdate)/1000.);		// set integration time by time elapsed since last filter update
		lastUpdate = Now;
		sum = sum + deltat;
		sumCount++;

// AHRS mahony+madgwick
		MahonyAHRSupdate((gx*PI/180.0),(gy*PI/180.0),(gz*PI/180.0),ax,ay,az,mx,my,mz);
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

		// Barometer
		bmp_temp = bmp280_get_temperature(dig_temp_raw, NULL);
		pres = bmp280_get_pressure(dig_temp_raw, dig_pressure_raw);
		altitude = get_altitude(pres, pressure_reference);

		// update shared memory
		position->ax = ax;
		position->ay = ay;
		position->az = az;
		position->gx = gx;
		position->gy = gy;
		position->gz = gz;
		position->mx = mx;
		position->my = my;
		position->mz = mz;
		position->quat0 = q0;
		position->quat1 = q1;
		position->quat2 = q2;
		position->quat3 = q3;
		position->roll = roll;
		position->pitch = pitch;
		position->yaw = yaw;
		position->pressure = pres;
		position->altitude = altitude;
		position->mpu_temp = mpu_temp;
		position->bmp_temp = bmp_temp;
	}
	return 0;
}
