# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
	bmp280 \
	i2c-tools \
	kbd-keymaps \
	kernel-modules \
	keyboard \
	libtar \
	mpu9250 \
	nano \
	PWM-tests \	
	test-gy-91 \
	wiringpi \
	"
