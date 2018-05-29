#include "gy-91.h"

int main(int argc, char *argv[]){
	int test_select = 0;
	int calib_mpu = 0; 
	int calib_ak = 0;
	int nineDOF = 0;
	int print_value = 0;
	unsigned int print_interval = 500;
	int sensor_value = 0;

	uint32_t count = 0;
	int fd;
	char shared[128] = IMU_MEM;
	sem_t * sem;
	char semaphore[128] = IMU_SEM;

	imu_t * pos;

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
	pos = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(pos == MAP_FAILED) {
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


	if(argc < 4) {
		printf("First argument, type of test : \n");
		printf("0 : exit\n");
		printf("1 : MPU9250 + AK8963 calibration; 9 axis quaternions\n");

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
			break;
		default:
			printf("forbidden value. First argument, type of test :\n");
			printf("0 : exit\n");
			printf("1 : MPU9250 + AK8963 calibration; 9 axis quaternions\n");
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

	while(1) {
		if((mymillis() - count) > print_interval) {	// print every print_interval ms
			sem_wait(sem);	// on attend que le sémaphore soit libre
			if(print_value == 1) {
				printf("X-acceleration: %f mg\n", 1000*pos->ax);
				printf("Y-acceleration: %f mg\n", 1000*pos->ay);
				printf("Z-acceleration: %f mg\n", 1000*pos->az);
				printf("X-gyro rate: %f dps\n", pos->gx);
				printf("Y-gyro rate: %f dps\n", pos->gy);
				printf("Z-gyro rate: %f dps\n", pos->gz);
				printf("X-mag field: %f mGauss\n", pos->mx);
				printf("Y-mag field: %f mGauss\n", pos->my);
				printf("Z-mag field: %f mGauss\n", pos->mz);
				printf("Temperature from MPU9250 is %f degrees C\n", pos->mpu_temp);
				printf("quat0 = %f\nquat1 = %f\nquat2 = %f\nquat3 = %f\n", pos->quat0, pos->quat1, pos->quat2, pos->quat3);
				printf("rpy : %f,%f,%f\n", pos->roll, pos->pitch, pos->yaw);
				printf("Temperature from BMP280 is : %f degrees C\n", pos->bmp_temp);
				printf("Pressure is %f hPa\n",pos->pressure/100);
				printf("Altitude is %f meters\n", pos->altitude);
			} else if(print_value == 2) {
				printf("%f,%f,%f\n", pos->mx, pos->my, pos->mz); // Magnetometer_Calibration output
			} else if (print_value == 3) {
				printf("%f,%f,%f,%f,%f,%f,%f,%f\n", pos->quat0,pos->quat1,pos->quat2,pos->quat3, pos->roll*PI/180.0, pos->pitch*PI/180.0, pos->yaw*PI/180.0, pos->pressure); // avr_sample_imu10dof01_01 output
			} else if (print_value == 4) {
			printf("%f,%f,%f,%f\n", pos->pitch, pos->roll, pos->yaw, pos->altitude); // IMU_3D_arrow output with quaternions
			}
			sem_post(sem);	// on lache le sémaphore
			count = mymillis();
		}
	}

}
