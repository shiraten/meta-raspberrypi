LINUX_VERSION ?= "4.1.10"

SRCREV = "b74df9228c27f55361c065bc5dbfba88861cc771"
SRC_URI = " \
	git://github.com/raspberrypi/linux.git;protocol=git;branch=rpi-4.1.y \
        file://0001-add-imu-to-dts.patch \
	file://defconfig \
        "

#file://0001-add-driver-for-realtek-usb-key.patch
#file://0001-add-config-for-realtek-usb-wifi.patch

require linux-raspberrypi.inc

#do_compile() {
#	make bcm2709_defconfig
#	make
#}
