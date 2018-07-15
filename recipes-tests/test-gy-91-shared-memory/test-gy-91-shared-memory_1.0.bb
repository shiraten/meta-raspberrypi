SUMMARY = "GY-91 test program with semaphore"
LICENSE = "CLOSED"
inherit module
PR = "r0"

SRC_URI = " \
	file://test-gy-91.c \
	file://read-gy-91.c \
	file://gy-91.h \
	file://AK8963/ \
	file://MPU9250/ \
	file://BMP280/ \
	file://libAHRS/ \
"

S = "${WORKDIR}"

do_compile() {
	${CC} test-gy-91.c libAHRS/AHRS.c BMP280/BMP280.c AK8963/AK8963.c MPU9250/MPU9250.c -o test-gy-91-shared-memory -lm -lrt -lpthread
	${CC} read-gy-91.c -o read-gy-91 -lrt -lpthread
}
