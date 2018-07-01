#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/kmod.h>
#include <linux/types.h>

#include <linux/fs.h>		// file structure (filp_open)
#include <linux/moduleparam.h>	// use command line arguments (eg : insmod mymodule myvariable=5)
#include <linux/string.h>
#include <linux/ioctl.h>

#include "mpu9250.h"

static int mpu9250_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mpu9250_remove(struct i2c_client *client);
static int mpu9250_shutdown(struct i2c_client *client);

static struct mpu9250_data {
	struct i2c_client *client;
	const struct i2c_device_id *id;
};

struct mpu9250_data mpu9250;

static int i2c_read_regs(struct i2c_client *client, u8 *cmd, u8 len, u8 *buf, u8 out_len)
{
	int ret;
	/* structure du message à envoyer */
	struct i2c_msg msg[2] = {
	{
		.addr = client->addr,
		.flags = 0,		// write
		.len = len,
		.buf = cmd,
	},
	{
		.addr = client->addr,
		.flags = I2C_M_RD,	// read
		.len = out_len,
		.buf = buf,
	}
	};

	/* on envoi le message et on lit le retour */
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_debug("cannot transfer\n");
		dev_err(&client->dev, "I2C read failed\n");
		return ret;
	}

	return 0;
}

// Slv0 function
static int mpu9250_i2c_slv0_ctrl(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];
	pr_debug("slv0 ctrl : %02X\n", config);

	cmd[0] = MPU9250_I2C_SLV0_CTRL;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

// Slv0 function
static int mpu9250_i2c_slv0_addr(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];
	pr_debug("slv0 addr : %02X\n", config);

	cmd[0] = MPU9250_I2C_SLV0_ADDR;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

// Slv0 function
static int mpu9250_i2c_slv0_reg(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];
	pr_debug("slv0 reg : %02X\n", config);

	cmd[0] = MPU9250_I2C_SLV0_REG;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

// Slv0 function
static int mpu9250_i2c_slv0_d0(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];
	pr_debug("slv0 d0 : %02X\n", config);

	cmd[0] = MPU9250_I2C_SLV0_DO;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_i2c_slv0_ext_sens_data_00(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = MPU9250_EXT_SENS_DATA_00;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i=0; i < lenght; i++) {
		dest[i] = data[i];
		pr_debug("slv0 ext data : %02X\n", data[i]);
	}

	return 0;
}

static int mpu9250_whoami(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = MPU9250_FIFO_WHO_AM_I;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}

static int mpu9250_int_status(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = MPU9250_INT_STATUS;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}

static int mpu9250_temperature(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[2];

	cmd[0] = MPU9250_TEMP_OUT_H;
	i2c_read_regs(client, &cmd, 1, &data, 2);

	dest[0] = data[0];
	dest[1] = data[1];

	return 0;
}

static int mpu9250_set_accel_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t accel_config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ACCEL_CONFIG;
	cmd[1] = accel_config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_set_accel_config2(struct i2c_client *client, const struct i2c_device_id *id, uint8_t accel_config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ACCEL_CONFIG_2;
	cmd[1] = accel_config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_set_gyro_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t gyro_config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_GYRO_CONFIG;
	cmd[1] = gyro_config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_get_accel_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = MPU9250_ACCEL_CONFIG;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}
static int mpu9250_get_accel_config2(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = MPU9250_ACCEL_CONFIG_2;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}

static int mpu9250_get_gyro_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = MPU9250_GYRO_CONFIG;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}

static int mpu9250_get_accel_data(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	int i, lenght = 6;
	uint8_t data[lenght];

	cmd[0] = MPU9250_ACCEL_XOUT_H;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int mpu9250_get_gyro_data(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	int i, lenght = 6;
	uint8_t data[lenght];

	cmd[0] = MPU9250_GYRO_XOUT_H;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int mpu9250_pwr_mgmt_1(struct i2c_client *client, const struct i2c_device_id *id, uint8_t pwr_config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_PWR_MGMT_1;
	cmd[1] = pwr_config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_pwr_mgmt_2(struct i2c_client *client, const struct i2c_device_id *id, uint8_t pwr_config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_PWR_MGMT_2;
	cmd[1] = pwr_config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_int_enable(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_INT_ENABLE;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_int_pin_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_INT_PIN_CFG;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_fifo_enable(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_FIFO_EN;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_i2c_master_control(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_I2C_MST_CTRL;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_user_control(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_USER_CTRL;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_CONFIG;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_sample_rate(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_SMPLRT_DIV;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_fifo_count(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[2];

	cmd[0] = MPU9250_FIFO_COUNTH;
	i2c_read_regs(client, &cmd, 1, &data, 2);

	dest[0] = data[0];
	dest[1] = data[1];

	return 0;
}


static int mpu9250_xa_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_XA_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_xa_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_XA_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_ya_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_YA_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_ya_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_YA_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_za_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ZA_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_za_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ZA_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_xg_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_XG_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_xg_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_XG_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_yg_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_YG_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_yg_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_YG_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_zg_h_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ZG_OFFSET_H;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_zg_l_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = MPU9250_ZG_OFFSET_L;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int mpu9250_get_xa_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[2];

	cmd[0] = MPU9250_XA_OFFSET_H;
	i2c_read_regs(client, &cmd, 1, &data, 2);

	dest[0] = data[0];
	dest[1] = data[1];

	return 0;
}

static int mpu9250_get_ya_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[2];

	cmd[0] = MPU9250_YA_OFFSET_H;
	i2c_read_regs(client, &cmd, 1, &data, 2);

	dest[0] = data[0];
	dest[1] = data[1];

	return 0;
}

static int mpu9250_get_za_offset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[2];

	cmd[0] = MPU9250_ZA_OFFSET_H;
	i2c_read_regs(client, &cmd, 1, &data, 2);

	dest[0] = data[0];
	dest[1] = data[1];

	return 0;
}

static int mpu9250_get_fifo_rw(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=12;
	uint8_t data[lenght];

	cmd[0] = MPU9250_FIFO_R_W;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++) {
		pr_debug("module %d\n", data[i]);
		dest[i] = data[i];
	}

	return 0;
}

static int mpu9250_selftest_x_accel(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_X_ACCEL;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_selftest_y_accel(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_Y_ACCEL;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_selftest_z_accel(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_Z_ACCEL;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_selftest_x_gyro(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_X_GYRO;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_selftest_y_gyro(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_Y_GYRO;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_selftest_z_gyro(struct i2c_client *client, const struct i2c_device_id *id, int16_t *dest) {
	uint8_t cmd[1];
	int i, lenght=1;
	uint8_t data[lenght];

	cmd[0] = MPU9250_SELF_TEST_Z_GYRO;
	i2c_read_regs(client, &cmd, 1, &data, lenght);
	for(i=0; i < lenght; i++)
		dest[i] = data[i];

	return 0;
}

static int mpu9250_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	printk("[mpu9250] driver probed\n");

	mpu9250.client = client;
	mpu9250.id = id;

	return 0;
}

static int mpu9250_remove(struct i2c_client *client) {
	printk("mpu9250 driver removed\n");
	return 0;
}

static int mpu9250_shutdown(struct i2c_client *client) {
	printk("mpu9250 driver is shutting down\n");
	return 0;
}

// i2c driver structure
// check https://www.kernel.org/doc/Documentation/i2c/writing-clients
static const struct i2c_device_id mpu9250_idtable[] = {
	{"mpu9250", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mpu9250_idtable);

static struct i2c_driver mpu9250_driver = {
	.driver    = {
		.name  = "mpu9250",
		.owner = THIS_MODULE,
	},
	.id_table  = mpu9250_idtable,
	.probe     = mpu9250_probe,
	.remove    = mpu9250_remove,
	.shutdown  = mpu9250_shutdown,
};


int data_lenght = 1;

static int my_ioctl_function(struct file *file, unsigned int cmd, uint8_t *args)
{
	pr_debug("[mpu9250]: my_ioctl_function\n");
	int status = 0;

	switch(cmd)
	{
		case GET_IMU_WIA:
			pr_debug("GET_IMU_WIA\n");
			status = mpu9250_whoami(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_INT_STATUS:
			pr_debug("IMU_INT_STATUS\n");
			status = mpu9250_int_status(mpu9250.client, mpu9250.id, args);
			break;
		case GET_TEMPERATURE:
			pr_debug("GET_TEMPERATURE\n");
			status = mpu9250_temperature(mpu9250.client, mpu9250.id, args);
			break;
		case SET_ACCEL_CONFIG:
			pr_debug("SET_ACCEL_CONFIG\n");
			status = mpu9250_set_accel_config(mpu9250.client, mpu9250.id, args);
			break;
		case SET_ACCEL_CONFIG2:
			pr_debug("SET_ACCEL_CONFIG2\n");
			status = mpu9250_set_accel_config2(mpu9250.client, mpu9250.id, args);
			break;
		case GET_ACCEL_CONFIG:
			pr_debug("GET_ACCEL_CONFIG\n");
			status = mpu9250_get_accel_config(mpu9250.client, mpu9250.id, args);
			break;
		case GET_ACCEL_CONFIG2:
			pr_debug("GET_ACCEL_CONFIG2\n");
			status = mpu9250_get_accel_config2(mpu9250.client, mpu9250.id, args);
			break;
		case GET_GYRO_CONFIG:
			pr_debug("GET_GYRO_CONFIG\n");
			status = mpu9250_get_gyro_config(mpu9250.client, mpu9250.id, args);
			break;
		case GET_ACCEL_DATA:
			pr_debug("GET_ACCEL_DATA\n");
			status = mpu9250_get_accel_data(mpu9250.client, mpu9250.id, args);
			break;
		case SET_GYRO_CONFIG:
			pr_debug("SET_GYRO_CONFIG\n");
			status = mpu9250_set_gyro_config(mpu9250.client, mpu9250.id, args);
			break;
		case GET_GYRO_DATA:
			pr_debug("GET_GYRO_DATA\n");
			status = mpu9250_get_gyro_data(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_PWR_MGMT_1:
			pr_debug("IMU_PWR_MGMT_1\n");
			status = mpu9250_pwr_mgmt_1(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_PWR_MGMT_2:
			pr_debug("IMU_PWR_MGMT_2\n");
			status = mpu9250_pwr_mgmt_2(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_INT_ENABLE:
			pr_debug("IMU_INT_ENABLE\n");
			status = mpu9250_int_enable(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_INT_PIN_CFG:
			pr_debug("IMU_INT_PIN_CFG\n");
			status = mpu9250_int_pin_config(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_FIFO_EN:
			pr_debug("IMU_FIFO_EN\n");
			status = mpu9250_fifo_enable(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_I2C_MST_CTRL:
			pr_debug("IMU_I2C_MST_CTRL\n");
			status = mpu9250_i2c_master_control(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_USR_CTRL:
			pr_debug("IMU_USR_CTRL\n");
			status = mpu9250_user_control(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_CONFIG:
			pr_debug("IMU_CONFIG\n");
			status = mpu9250_config(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SMPLRT_DIV:
			pr_debug("IMU_SMPLRT_DIV\n");
			status = mpu9250_sample_rate(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_FIFO_COUNTH:
			pr_debug("IMU_FIFO_COUNTH\n");
			status = mpu9250_fifo_count(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_XA_OFFSET_H:
			pr_debug("IMU_XA_OFFSET_H\n");
			status = mpu9250_xa_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_XA_OFFSET_L:
			pr_debug("IMU_XA_OFFSET_L\n");
			status = mpu9250_xa_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_YA_OFFSET_H:
			pr_debug("IMU_YA_OFFSET_H\n");
			status = mpu9250_ya_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_YA_OFFSET_L:
			pr_debug("IMU_YA_OFFSET_L\n");
			status = mpu9250_ya_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_ZA_OFFSET_H:
			pr_debug("IMU_ZA_OFFSET_H\n");
			status = mpu9250_za_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_ZA_OFFSET_L:
			pr_debug("IMU_ZA_OFFSET_L\n");
			status = mpu9250_za_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_XG_OFFSET_H:
			pr_debug("IMU_XG_OFFSET_H\n");
			status = mpu9250_xg_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_XG_OFFSET_L:
			pr_debug("IMU_XG_OFFSET_L\n");
			status = mpu9250_xg_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_YG_OFFSET_H:
			pr_debug("IMU_YG_OFFSET_H\n");
			status = mpu9250_yg_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_YG_OFFSET_L:
			pr_debug("IMU_YG_OFFSET_L\n");
			status = mpu9250_yg_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_ZG_OFFSET_H:
			pr_debug("IMU_ZG_OFFSET_H\n");
			status = mpu9250_zg_h_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_ZG_OFFSET_L:
			pr_debug("IMU_ZG_OFFSET_L\n");
			status = mpu9250_zg_l_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_GET_XA_OFFSET:
			pr_debug("IMU_GET_XA_OFFSET\n");
			status = mpu9250_get_xa_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_GET_YA_OFFSET:
			pr_debug("IMU_GET_YA_OFFSET\n");
			status = mpu9250_get_ya_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_GET_ZA_OFFSET:
			pr_debug("IMU_GET_ZA_OFFSET\n");
			status = mpu9250_get_ya_offset(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_GET_FIFO_R_W:
			pr_debug("IMU_GET_FIFO_R_W\n");
			status = mpu9250_get_fifo_rw(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_X_ACCEL:
			pr_debug("IMU_SELF_TEST_X_ACCEL\n");
			status = mpu9250_selftest_x_accel(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_Y_ACCEL:
			pr_debug("IMU_SELF_TEST_Y_ACCEL\n");
			status = mpu9250_selftest_y_accel(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_Z_ACCEL:
			pr_debug("IMU_SELF_TEST_Z_ACCEL\n");
			status = mpu9250_selftest_z_accel(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_X_GYRO:
			pr_debug("IMU_SELF_TEST_X_GYRO\n");
			status = mpu9250_selftest_x_gyro(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_Y_GYRO:
			pr_debug("IMU_SELF_TEST_Y_GYRO\n");
			status = mpu9250_selftest_y_gyro(mpu9250.client, mpu9250.id, args);
			break;
		case IMU_SELF_TEST_Z_GYRO:
			pr_debug("IMU_SELF_TEST_Z_GYRO\n");
			status = mpu9250_selftest_z_gyro(mpu9250.client, mpu9250.id, args);
			break;
		case SLV0_CTRL:
			pr_debug("SLV0_CTRL\n");
			status = mpu9250_i2c_slv0_ctrl(mpu9250.client, mpu9250.id, args);
			break;
		case SLV0_ADDR:
			pr_debug("SLV0_ADDR\n");
			status = mpu9250_i2c_slv0_addr(mpu9250.client, mpu9250.id, args);
			break;
		case SLV0_REG:
			pr_debug("SLV0_REG\n");
			status = mpu9250_i2c_slv0_reg(mpu9250.client, mpu9250.id, args);
			if(args == AK8963_XOUT_L) {
				data_lenght = 7;
			} else if(args == AK8963_ASAX) {
				data_lenght = 3;
			} else if(args == AK8963_ST1) {
				data_lenght = 8;
			} else {
				data_lenght = 1;
			}
			break;
		case SLV0_D0:
			pr_debug("SLV0_D0\n");
			status = mpu9250_i2c_slv0_d0(mpu9250.client, mpu9250.id, args);
			break;
		case SLV0_EXT_DATA_0:
			pr_debug("SLV0_EXT_DATA_0\n");
			status = mpu9250_i2c_slv0_ext_sens_data_00(mpu9250.client, mpu9250.id, args, data_lenght);
			break;
		default : 
			pr_debug("default\n");
			status = -EINVAL;
			break;
	}
	return status;
}

/* accès a la fonction depuis l'espace utilisateur */
static struct file_operations fops = {
	.unlocked_ioctl = my_ioctl_function,
};

static int __init mpu9250_init(void)
{
	pr_debug("mpu9250 init\n");
	register_chrdev(MAJOR, DEVICE_NAME, &fops);	// char device registeration
	return i2c_add_driver(&mpu9250_driver);
}
module_init(mpu9250_init);

static void __exit mpu9250_exit(void)
{
	pr_debug("mpu9250 exit\n");
	unregister_chrdev(MAJOR, DEVICE_NAME);
	i2c_del_driver(&mpu9250_driver);
}
module_exit(mpu9250_exit);


MODULE_AUTHOR("Maxime Duchene");
MODULE_DESCRIPTION("mpu9250 module");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
