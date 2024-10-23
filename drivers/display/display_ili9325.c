/*
 * Copyright (c) 2024 Andrea Merello <andrea.merello@gmail.com>
 *
 * Based on display_ili9xxx.c, which is:
 *  Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *  Copyright (c) 2019 Nordic Semiconductor ASA
 *  Copyright (c) 2020 Teslabs Engineering S.L.
 *  Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ilitek_ili9325

#define ILI9325_RESET_PULSE_TIME 5
#define ILI9325_RESET_WAIT_TIME 60

#include "display_ili9325.h"

//#include <zephyr/dt-bindings/display/ili9xxx.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_ili9325, CONFIG_DISPLAY_LOG_LEVEL);

struct ili9325_data {
	int dummy;
};

static int ili9325_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct ili9325_config *config = dev->config;
	val = sys_cpu_to_be16(val);
	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				      reg, (uint8_t *)(&val), 2);
}

static int ili9325_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct ili9325_config *config = dev->config;
	uint16_t val16;
	int ret;

	ret = mipi_dbi_command_read(config->mipi_dev, &config->dbi_config,
				    &reg, 1, (uint8_t *)(&val16), 2);
	if (ret < 0)
		return ret;
	*val = sys_be16_to_cpu(val16);
	return 0;
}

static int ili9325_set_mem_area(const struct device *dev, const uint16_t x,
				const uint16_t y, const uint16_t w,
				const uint16_t h)
{
	int ret;

	ret = ili9325_reg_write(dev, 0x50, x);
	if (ret < 0)
		return ret;
	ret = ili9325_reg_write(dev, 0x51, x + w - 1U);
	if (ret < 0)
		return ret;
	ret = ili9325_reg_write(dev, 0x52, y);
	if (ret < 0)
		return ret;

	return ili9325_reg_write(dev, 0x53, y + h - 1U);
}

static int ili9325_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct ili9325_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;

	int ret;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * 2 * desc->height) <=
			 desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
		x, y);
	ret = ili9325_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
		mipi_desc.height = 1;
		mipi_desc.buf_size = desc->pitch * 2;
	} else {
		write_h = desc->height;
		mipi_desc.height = desc->height;
		mipi_desc.buf_size = desc->width * 2 * write_h;
		nbr_of_writes = 1U;
	}

	mipi_desc.width = desc->width;
	/* Per MIPI API, pitch must always match width */
	mipi_desc.pitch = desc->width;

	ret = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, 0x22, NULL, 0);
	if (ret < 0)
		return ret;

	for (write_cnt = 0U; write_cnt < nbr_of_writes; ++write_cnt) {
		ret = mipi_dbi_write_display(config->mipi_dev,
					   &config->dbi_config,
					   write_data_start,
					   &mipi_desc,
					   PIXEL_FORMAT_RGB_565);
		if (ret < 0) {
			return ret;
		}

		write_data_start += desc->pitch * 2;
	}

	return 0;
}



static void ili9325_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	capabilities->x_resolution = 320;
	capabilities->y_resolution = 240;

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static uint16_t reg_magic[][2] = {
	{0x00e7,0x0010},
        {0x0000,0x0001},
        {0x0001,0x0100},
        {0x0002,0x0700},
        {0x0003,(1<<12)|(3<<4)|(0<<3)},
        {0x0004,0x0000},
        {0x0008,0x0207},
        {0x0009,0x0000},
        {0x000a,0x0000},
        {0x000c,0x0001},
        {0x000d,0x0000},
        {0x000f,0x0000},
        {0x0010,0x0000},
        {0x0011,0x0007},
        {0x0012,0x0000},
        {0x0013,0x0000},
	{0xFFFF, 50},
        {0x0010,0x1590},
        {0x0011,0x0227},
        {0xFFFF, 50},
        {0x0012,0x009c},
        {0xFFFF, 50},
        {0x0013,0x1900},
        {0x0029,0x0023},
        {0x002b,0x000e},
        {0xFFFF, 50},
        {0x0020,0x0000},
        {0x0021,0x013f},
	{0xFFFF, 50},
        {0x0030,0x0007},
        {0x0031,0x0707},
        {0x0032,0x0006},
        {0x0035,0x0704},
        {0x0036,0x1f04},
        {0x0037,0x0004},
        {0x0038,0x0000},
        {0x0039,0x0706},
        {0x003c,0x0701},
        {0x003d,0x000f},
        {0xFFFF, 50},
        {0x0050,0x0000},
        {0x0051,0x00ef},
        {0x0052,0x0000},
        {0x0053,0x013f},
        {0x0060,0xa700},
        {0x0061,0x0001},
        {0x006a,0x0000},
        {0x0080,0x0000},
        {0x0081,0x0000},
        {0x0082,0x0000},
        {0x0083,0x0000},
        {0x0084,0x0000},
        {0x0085,0x0000},
        {0x0090,0x0010},
        {0x0092,0x0000},
        {0x0093,0x0003},
        {0x0095,0x0110},
        {0x0097,0x0000},
        {0x0098,0x0000},
        {0x0007,0x0133},
        {0x0020,0x0000},
        {0x0021,0x013f}
};

static int ili9325_configure(const struct device *dev)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(reg_magic); i++) {
		if (reg_magic[i][0] == 0xffff) {
			k_msleep(reg_magic[i][1]);
		} else {
			ret = ili9325_reg_write(dev, reg_magic[i][0], reg_magic[i][1]);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int ili9325_init(const struct device *dev)
{
	const struct ili9325_config *config = dev->config;
	uint16_t val;
	int ret;

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI DBI device is not ready");
		return -ENODEV;
	}

	if (mipi_dbi_reset(config->mipi_dev, ILI9325_RESET_PULSE_TIME) == 0)
		k_sleep(K_MSEC(ILI9325_RESET_WAIT_TIME));

	ret = ili9325_reg_read(dev, 0x0, &val);
	if (ret < 0) {
		LOG_ERR("Could not read ID reg (%d)", ret);
		return ret;
	}

	if (val != 0x9325 && val != 0x9328) {
		LOG_ERR("Invalid device ID 0x(%x)", val);
		return -ENODEV;
	}

	LOG_INF("Display detected (0x%x)", val);

	ret = ili9325_configure(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure display (%d)", ret);
		return ret;
	}
	return 0;
}

static const struct display_driver_api ili9325_api = {
	.write = ili9325_write,
	.get_capabilities = ili9325_get_capabilities,
};

#define ILI9325(id) 		                                               \
	static const struct ili9325_config ili9325_config_##id = {             \
		.mipi_dev = DEVICE_DT_GET(DT_INST_PARENT(id)),		\
	};                                                                     \
									       \
	static struct ili9325_data ili9325_data_##id;                           \
									       \
	DEVICE_DT_INST_DEFINE(id, &ili9325_init, PM_DEVICE_DT_INST_GET(id),	\
			      &ili9325_data_##id, &ili9325_config_##id, POST_KERNEL, \
			      CONFIG_DISPLAY_INIT_PRIORITY, &ili9325_api);



DT_INST_FOREACH_STATUS_OKAY(ILI9325);
