SUMMARY = "MPU9250 Linux kernel module"
LICENSE = "CLOSED"
inherit module
PR = "r0"

SRC_URI = " \
	file://mpu9250.c \
	file://mpu9250.h \
	file://Makefile \
"

S = "${WORKDIR}"
