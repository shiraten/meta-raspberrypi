SUMMARY = "BMP280 Linux kernel module"
LICENSE = "CLOSED"
inherit module
PR = "r0"

SRC_URI = " \
	file://bmp280.c \
	file://bmp280.h \
	file://Makefile \
"

S = "${WORKDIR}"
