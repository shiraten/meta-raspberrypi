/**
 * InvenSense MPU9250 Register Map.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MPU9250_REG_H_INCLUDED
#define MPU9250_REG_H_INCLUDED

#include "AK8963/AK8963.h"
#include "MPU9250/MPU9250.h"
#include "BMP280/BMP280.h"
#include "libAHRS/AHRS.h"

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
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/select.h>

#define IMU_MEM		"/imumem"	// mémoire partagée
#define IMU_SEM		"/imusem"	// sémaphore

#define GYR_DEADBAND 1		// [deg/s]
#define RAD_TO_DEG 57.29578
#define PI 3.14159265358979323846
// Complementary filter
#define DT 0.005	// gyro sample = 200 Hz
//#define AA 0.97		// complementary filter constant

typedef struct {
	float ax;
	float ay;
	float az;
	float gx;
	float gy;
	float gz;
	float mx;
	float my;
	float mz;
	float quat0;
	float quat1;
	float quat2;
	float quat3;
	float roll;
	float pitch;
	float yaw;
	float pressure;
	float altitude;
	float mpu_temp;
	float bmp_temp;
} imu_t;

// get ms of the time of date
int mymicros()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}
int mymillis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec)/1000;
}

#endif
