/*
 * platform_wm8731.c: wm8731 platform data initilization file
 *
 * (C) Copyright 2013 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <asm/intel-mid.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/i2c.h>

static struct regulator_consumer_supply vwm8731_consumer[] = {
	REGULATOR_SUPPLY("AVDD", "1-001a"),
	REGULATOR_SUPPLY("HPVDD", "1-001a"),
	REGULATOR_SUPPLY("DCVDD", "1-001a"),
	REGULATOR_SUPPLY("DBVDD", "1-001a"),
};

static struct regulator_init_data vwm8731_data = {
		.constraints = {
			.always_on = 1,
		},
		.num_consumer_supplies	=	ARRAY_SIZE(vwm8731_consumer),
		.consumer_supplies	=	vwm8731_consumer,
};

static struct fixed_voltage_config vwm8731_config = {
	.supply_name	= "VCC_1.8V",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &vwm8731_data,
};

static struct platform_device vwm8731_device = {
	.name = "reg-fixed-voltage",
	.id = 0,
	.dev = {
		.platform_data = &vwm8731_config,
	},
};

static struct platform_device *wm8731_reg_devices[] __initdata = {
        &vwm8731_device
};

static struct i2c_board_info wm8731_info = {
	I2C_BOARD_INFO("wm8731", 0x1a),
};

void __init *wm8731_platform_data(void *info)
{
	int bus_num = 1;

	platform_add_devices(wm8731_reg_devices,
			ARRAY_SIZE(wm8731_reg_devices));

	i2c_register_board_info(bus_num, &wm8731_info, 1);

	return;
}
