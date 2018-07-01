DESCRIPTION = "GY-91 test program"
LICENSE = "CLOSED"
PR = "r0"

SRC_URI = " \
	file://test-gy-91.c \
	file://gy-91.h \
	file://AK8963/ \
	file://MPU9250/ \
	file://BMP280/ \
	file://libAHRS/ \
"

S = "${WORKDIR}"

do_compile() {
	${CC} test-gy-91.c libAHRS/AHRS.c BMP280/BMP280.c AK8963/AK8963.c MPU9250/MPU9250.c -o test-gy-91 -lm
}

do_install() {
        install -d ${D}${bindir}
        install -m 0744 ${S}/test-gy-91 ${D}${bindir}
}
