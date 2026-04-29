/* 
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/npm2100.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l6_e1_main, CONFIG_LOG_DEFAULT_LEVEL);

#define SHORT_PRESS_MS     500
#define DVS_STATE_LDSW_ON  1
#define DVS_STATE_LDSW_OFF 0

static const struct device *npm2100 = DEVICE_DT_GET_ANY(nordic_npm2100);
static const struct device *regulator = DEVICE_DT_GET_ANY(nordic_npm2100_regulator);
static const struct device *ldsw = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm2100_ldosw));
static const struct device *vbat = DEVICE_DT_GET_ANY(nordic_npm2100_vbat);

static bool short_press;
static struct gpio_callback shphld_cb;
static struct k_work shphld_work;

static int ldsw_pulse(int32_t on_off_dur, size_t cnt) {
	int ret;

	while (cnt--) {
		ret = regulator_parent_dvs_state_set(regulator, DVS_STATE_LDSW_ON);
		if (ret < 0) {
			return ret;
		}
		k_msleep(on_off_dur);

		ret = regulator_parent_dvs_state_set(regulator, DVS_STATE_LDSW_OFF);
		if (ret < 0) {
			return ret;
		}
		k_msleep(on_off_dur);
	}

	return 0;
}

void shphld_handler(struct k_work *work) {
	int ret;

	if (short_press) {
		LOG_INF("short press");
		ret = ldsw_pulse(500, 1);
	} else {
		LOG_INF("long press");
		ret = ldsw_pulse(250, 2);
	}

	if (ret < 0) {
		LOG_ERR("error while turning LDSW on/off (%d)", ret);
	}
}

void events_handler(const struct device *dev, struct gpio_callback *cb, uint32_t events) {
	static int64_t ref_time;

	if (events & BIT(NPM2100_EVENT_SYS_SHIPHOLD_FALL)) {
		ref_time = k_uptime_get();
	}

	if (events & BIT(NPM2100_EVENT_SYS_SHIPHOLD_RISE)) {
		short_press = k_uptime_delta(&ref_time) < SHORT_PRESS_MS;
		k_work_submit(&shphld_work);
	}
}

void display_vbat(void) {
	int ret;
	struct sensor_value val;

	/* STEP 3.1 - Fetch a new measurement */

	/* STEP 3.2 - Get the SENSOR_CHAN_GAUGE_VOLTAGE channel result */

	LOG_INF("VBAT: %d.%03d V", val.val1, val.val2 / 1000);
}

int main(void) {
	int ret;

	ret = regulator_parent_dvs_state_set(regulator, DVS_STATE_LDSW_OFF);
	if (ret < 0) {
		LOG_ERR("failed to change DVS state (%d)", ret);
		return ret;
	}
	regulator_enable(ldsw);
	if (ret < 0) {
		LOG_ERR("failed to enable LDSW (%d)", ret);
		return ret;
	}

	k_work_init(&shphld_work, shphld_handler);

	/* STEP 2.1 - Initialize the npm2100 callback  */

	/* STEP 2.2 - Add the npm2100 callback `shphld_cb` to the mfd driver. */

	LOG_INF("PMIC device ok");

	while (true) {
		display_vbat();
		k_msleep(2000);
	}

	return 0;
}
