/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(Lesson2_Exercise1, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Define the advertising parameter for connectable advertising */
#define APP_BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL);

/* Define the advertising parameter for non-connectable advertising */
#define APP_BT_LE_ADV_NONCONN BT_LE_ADV_PARAM(0, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL);

/* Declare the advertising packet */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Variable to keep track of current advertising type */
enum adv_type {
	ADV_CONN_SCAN = 0, //connectable and scannable
	ADV_CONN = 1, //directed connectable
	ADV_NONCONN_SCAN = 2, //non-connectable, scannable
	ADV_NONCONN = 3, //non-connectable
	NONE = 4
};

static int current_adv_type = NONE;

static unsigned char url_data[] = { 0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm',
				    'y',  '.', 'n', 'o', 'r', 'd', 'i', 'c', 's',
				    'e',  'm', 'i', '.', 'c', 'o', 'm' };

/* Declare the scan response packet */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t button = button_state & has_changed;

	if (button & DK_BTN1_MSK) {
		if (current_adv_type == ADV_CONN_SCAN) {
			LOG_INF("Advertising type already selected");
			return;
		}
		LOG_INF("Starting connectable, scannable advertising");
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			LOG_ERR("Advertising failed to start (err %d)", err);
			current_adv_type = NONE;
			return;
		}
		LOG_INF("Advertising started succesfully");
		current_adv_type = ADV_CONN_SCAN;
	}

	if (button & DK_BTN2_MSK) {
		if (current_adv_type == ADV_CONN) {
			LOG_INF("Advertising type already selected");
			return;
		}
		LOG_INF("Starting connectable advertising");
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			LOG_ERR("Advertising failed to start (err %d)", err);
			current_adv_type = NONE;
			return;
		} 
		LOG_INF("Advertising started succesfully");
		current_adv_type = ADV_CONN;
	}

	if (button & DK_BTN3_MSK) {
		if (current_adv_type == ADV_NONCONN_SCAN) {
			LOG_INF("Advertising type already selected");
			return;
		}
		LOG_INF("Starting non-connectable, scannable advertising");
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_NONCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			LOG_ERR("Advertising failed to start (err %d)", err);
			current_adv_type = NONE;
			return;
		}
		LOG_INF("Advertising started succesfully");
		current_adv_type = ADV_NONCONN_SCAN;
	}

	if (button & DK_BTN4_MSK) {
		if (current_adv_type == ADV_NONCONN) {
			LOG_INF("Advertising type already selected");
			return;
		}
		LOG_INF("Starting non-connectable advertising");
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_NONCONN, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			LOG_ERR("Advertising failed to start (err %d)", err);
			current_adv_type = NONE;
			return;
		} 
		LOG_INF("Advertising started succesfully");
		current_adv_type = ADV_NONCONN;
	}
}


int main(void)
{
	int err;

	LOG_INF("Starting Lesson 3 - Exercise 1 ");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return -1;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Buttons init failed (err %d)", err);
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized");

	return 0;
}
