/* Copyright (c) 2024 Andrea Merello <andrea.merello@gmail.com>
 *
 * Based on display_ili9xxx.h, which is:
 *  Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *  Copyright (c) 2019 Nordic Semiconductor ASA
 *  Copyright (c) 2020 Teslabs Engineering S.L.
 *  Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9325_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9325_H_

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/util.h>

struct ili9325_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
};

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9325_H_ */
