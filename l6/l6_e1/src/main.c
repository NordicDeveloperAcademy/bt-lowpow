/* 
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/npm2100.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

LOG_MODULE_REGISTER(l6_e1_main, CONFIG_LOG_DEFAULT_LEVEL);

#define DEVICE_NAME           CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN       (sizeof(DEVICE_NAME) - 1)
#define APP_BT_LE_ADV_NONCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, \
											  BT_GAP_ADV_SLOW_INT_MIN, \
											  BT_GAP_ADV_SLOW_INT_MAX, NULL)
#define APP_ADV_UPD_PERIOD_MS 20000

static uint8_t mfg_data[] = "\x59\x00VBAT:0.00 VDD:0.00";

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data) - 1),
};

#define SHORT_PRESS_MS     500
#define DVS_STATE_LDSW_ON  1
#define DVS_STATE_LDSW_OFF 0

static const struct device *npm2100 = DEVICE_DT_GET_ANY(nordic_npm2100);
static const struct device *npm2100_reg = DEVICE_DT_GET_ANY(nordic_npm2100_regulator);
static const struct device *npm2100_ldsw = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm2100_ldosw));
static const struct device *npm2100_adc = DEVICE_DT_GET_ANY(nordic_npm2100_vbat);

static bool short_press;
static struct gpio_callback shphld_cb;
static struct k_work shphld_work;

static int ldsw_pulse(int32_t on_off_dur, size_t cnt) {
	int ret;

	while (cnt--) {
		ret = regulator_parent_dvs_state_set(npm2100_reg, DVS_STATE_LDSW_ON);
		if (ret < 0) {
			return ret;
		}
		k_msleep(on_off_dur);

		ret = regulator_parent_dvs_state_set(npm2100_reg, DVS_STATE_LDSW_OFF);
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

int get_voltages(struct sensor_value *vbat, struct sensor_value *vout) {
	int ret;

	/* STEP 3.1 - Fetch a new measurement */

	/* STEP 3.2 - Get the SENSOR_CHAN_GAUGE_VOLTAGE channel result into vbat */

	/* STEP 3.3 - Get the SENSOR_CHAN_VOLTAGE channel result into vout */

	return 0;
}

int main(void) {
	int ret;
	struct sensor_value vbat, vout;

	/* Init BT and start advertising */
	ret = bt_enable(NULL);
	if (ret) {
		LOG_ERR("bluetooth init failed (%d)\n", ret);
		return ret;
	}
	ret = bt_le_adv_start(APP_BT_LE_ADV_NONCONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (ret) {
		LOG_ERR("advertising failed to start (%d)\n", ret);
		return ret;
	}

	/* Configure PMIC */
	ret = regulator_parent_dvs_state_set(npm2100_reg, DVS_STATE_LDSW_OFF);
	if (ret < 0) {
		LOG_ERR("failed to change DVS state (%d)", ret);
		return ret;
	}
	regulator_enable(npm2100_ldsw);
	if (ret < 0) {
		LOG_ERR("failed to enable LDSW (%d)", ret);
		return ret;
	}

	k_work_init(&shphld_work, shphld_handler);

	/* STEP 2.1 - Initialize the npm2100 callback  */

	/* STEP 2.2 - Add the npm2100 callback `shphld_cb` to the mfd driver. */

	LOG_INF("PMIC device ok");

	while (true) {
		/* Get ADC measurements */
		get_voltages(&vbat, &vout);

		/* Change advertising data to include the new measurements */
		snprintf(&mfg_data[2], sizeof(mfg_data) - 2, "VBAT:%01d.%02d VDD:%01d.%02d",
				 abs(vbat.val1) % 10, abs(vbat.val2 / 10000) % 100,
				 abs(vout.val1) % 10, abs(vout.val2 / 10000) % 100);
		LOG_INF("%s", &mfg_data[2]);
		bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);

		k_msleep(APP_ADV_UPD_PERIOD_MS);
	}

	return 0;
}
