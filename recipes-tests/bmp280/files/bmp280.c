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

#include "bmp280.h"

static int bmp280_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int bmp280_remove(struct i2c_client *client);
static int bmp280_shutdown(struct i2c_client *client);

static struct bmp280_data {
	struct i2c_client *client;
	const struct i2c_device_id *id;
};

struct bmp280_data bmp280;

static int i2c_read_regs(struct i2c_client *client, uint8_t *cmd, uint8_t len, uint8_t *buf, uint8_t out_len)
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

static int bmp280_whoami(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest) {
	uint8_t cmd[1];
	uint8_t data[1];

	cmd[0] = BMP280_REGISTER_CHIPID;
	i2c_read_regs(client, &cmd, 1, &data, 1);

	dest[0] = data[0];

	return 0;
}

static int bmp280_config(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = BMP280_REGISTER_CONFIG;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int bmp280_control(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = BMP280_REGISTER_CONTROL;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int bmp280_reset(struct i2c_client *client, const struct i2c_device_id *id, uint8_t config) {
	uint8_t cmd[2];

	cmd[0] = BMP280_REGISTER_SOFTRESET;
	cmd[1] = config;
	if (i2c_master_send(client, cmd, 2) < 0) {
		pr_debug("failed to write buf to i2c\n");
		// ERROR HANDLING: i2c transaction failed
		return -1;
	}

	return 0;
}

static int bmp280_get_temperature(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = BMP280_REGISTER_TEMPDATA;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i = 0; i< lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int bmp280_get_dig_temperature(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = BMP280_REGISTER_DIG_T1;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i = 0; i< lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int bmp280_get_pressure(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = BMP280_REGISTER_PRESSUREDATA;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i = 0; i< lenght; i++) {
		dest[i] = data[i];
		//printk("%02X\n",data[i]);
	}

	return 0;
}

static int bmp280_get_dig_pressure(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = BMP280_REGISTER_DIG_P1;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i = 0; i< lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int bmp280_status(struct i2c_client *client, const struct i2c_device_id *id, uint8_t *dest, int lenght) {
	uint8_t cmd[1];
	uint8_t data[lenght];
	int i;

	cmd[0] = BMP280_REGISTER_STATUS;
	i2c_read_regs(client, &cmd, 1, &data, lenght);

	for(i = 0; i< lenght; i++) {
		dest[i] = data[i];
	}

	return 0;
}

static int bmp280_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	printk("[bmp280] driver probed\n");

	bmp280.client = client;
	bmp280.id = id;

	return 0;
}

static int bmp280_remove(struct i2c_client *client) {
	printk("bmp280 driver removed\n");
	return 0;
}

static int bmp280_shutdown(struct i2c_client *client) {
	printk("bmp280 driver is shutting down\n");
	return 0;
}

// i2c driver structure
// check https://www.kernel.org/doc/Documentation/i2c/writing-clients
static const struct i2c_device_id bmp280_idtable[] = {
	{"bmp280", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, bmp280_idtable);

static struct i2c_driver bmp280_driver = {
	.driver    = {
		.name  = "bmp280",
		.owner = THIS_MODULE,
	},
	.id_table  = bmp280_idtable,
	.probe     = bmp280_probe,
	.remove    = bmp280_remove,
	.shutdown  = bmp280_shutdown,
};


int data_lenght = 1;

static int my_ioctl_function(struct file *file, unsigned int cmd, uint8_t *args)
{
	pr_debug("[bmp280]: my_ioctl_function\n");
	int status = 0;

	switch(cmd)
	{
		case BMP280_WHOAMI :
			status = bmp280_whoami(bmp280.client, bmp280.id, args);
			break;
		case BMP280_CFG :
			status = bmp280_config(bmp280.client, bmp280.id, args);
			break;
		case BMP280_CTRL :
			status = bmp280_control(bmp280.client, bmp280.id, args);
			break;
		case BMP280_RESET :
			status = bmp280_reset(bmp280.client, bmp280.id, args);
			break;
		case BMP280_GET_TEMP :
			status = bmp280_get_temperature(bmp280.client, bmp280.id, args, 3);
			break;
		case BMP280_GET_DIG_TEMP :
			status = bmp280_get_dig_temperature(bmp280.client, bmp280.id, args, 6);
			break;
		case BMP280_GET_PRESSURE :
			status = bmp280_get_pressure(bmp280.client, bmp280.id, args, 3);
			break;
		case BMP280_GET_DIG_PRESSURE :
			status = bmp280_get_dig_pressure(bmp280.client, bmp280.id, args, 18);
			break;
		case BMP280_STATUS :
			status = bmp280_status(bmp280.client, bmp280.id, args, 1);
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

static int __init bmp280_init(void)
{
	pr_debug("bmp280 init\n");
	register_chrdev(MAJOR, DEVICE_NAME, &fops);	// char device registeration
	return i2c_add_driver(&bmp280_driver);
}
module_init(bmp280_init);

static void __exit bmp280_exit(void)
{
	pr_debug("bmp280 exit\n");
	unregister_chrdev(MAJOR, DEVICE_NAME);
	i2c_del_driver(&bmp280_driver);
}
module_exit(bmp280_exit);


MODULE_AUTHOR("Maxime Duchene");
MODULE_DESCRIPTION("bmp280 module test");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
