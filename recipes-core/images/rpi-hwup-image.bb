# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
	bmp280 \
	i2c-tools \
	mpu9250 \
	test-gy-91 \
	"
