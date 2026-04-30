/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <dk_buttons_and_leds.h>
#include "ad_ext_data.h"

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* STEP 1 - Define the advertising parameter for non-connectable advertising */
//const struct bt_le_adv_param *param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, \
													  BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL);

/* STEP 6 - Advertise only on channels 37 and 38 */
const struct bt_le_adv_param *param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_DISABLE_CHAN_39, \
                                      BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL);

/* STEP 8.1 - Define the advertising parameter for connectable and non-connectable advertising */
#define APP_BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)
#define APP_BT_LE_ADV_NONCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

/* STEP 9.2 - Define the advertising parameter for extended advertising */
#define APP_BT_LE_EXT_ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

/* STEP 2 - Define the advertising packet */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* STEP 7.1 - Define the minimal advertising packet */
static const struct bt_data ad_min[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

/* STEP 8.2 - Define variable to keep track of advertising types */
enum adv_type {
	ADV_CONN_SCAN = 0, //connectable and scannable
	ADV_CONN = 1, //connectable
	ADV_NONCONN_SCAN = 2, //non-connectable, scannable
	ADV_NONCONN = 3, //non-connectable
	NONE = 4
};

/* STEP 8.3 -  Define variable to keep track of the current advertising type */
static int current_adv_type = NONE;

/* STEP 8.4 - Define the scan response packet */
static unsigned char url_data[] = { 0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm',
				    'y',  '.', 'n', 'o', 'r', 'd', 'i', 'c', 's',
				    'e',  'm', 'i', '.', 'c', 'o', 'm' };

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};

/* STEP 9.3 - Declare the callback struct */
const static struct bt_le_ext_adv_cb adv_callbacks = {};

/* STEP 8.5 - Enable different advertising types upon button presses */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t button = button_state & has_changed;

	if (button & DK_BTN1_MSK) {
		if (current_adv_type == ADV_CONN_SCAN) {
			return;
		}

		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			current_adv_type = NONE;
			return;
		}
		current_adv_type = ADV_CONN_SCAN;
	}

	if (button & DK_BTN2_MSK) {
		if (current_adv_type == ADV_CONN) {
			return;
		}
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			current_adv_type = NONE;
			return;
		} 
		current_adv_type = ADV_CONN;
	}

	if (button & DK_BTN3_MSK) {
		if (current_adv_type == ADV_NONCONN_SCAN) {
			return;
		}
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_NONCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			current_adv_type = NONE;
			return;
		}
		current_adv_type = ADV_NONCONN_SCAN;
	}

	if (button & DK_BTN4_MSK) {
		if (current_adv_type == ADV_NONCONN) {
			return;
		}
		bt_le_adv_stop();
		err = bt_le_adv_start(APP_BT_LE_ADV_NONCONN, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			current_adv_type = NONE;
			return;
		}
		current_adv_type = ADV_NONCONN;
	}
}

int main(void)
{
	int err;

	/* STEP 8.6 - Register the button handler */
	err = dk_buttons_init(button_handler);
	if (err) {
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		return -1;
	}

	/* STEP 9.4 - Create the advertising set and start extended advertising */
	static struct bt_le_ext_adv *adv_set;

	err = bt_le_ext_adv_create(APP_BT_LE_EXT_ADV_PARAM, &adv_callbacks, &adv_set);
	if (err) {
		return 0;
	}

	err = bt_le_ext_adv_set_data(adv_set, ad_ext, ad_ext_size, NULL, 0);
	if (err) {
		return 0;
	}

	err = bt_le_ext_adv_start(adv_set, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		return 0;
	}

	/* STEP 7.2 - Start advertising with the minimal advertising packet */
	//err = bt_le_adv_start(param, ad, ARRAY_SIZE(ad), NULL, 0);
	/*err = bt_le_adv_start(param, ad_min, ARRAY_SIZE(ad_min), NULL, 0);
	if (err) {
		return -1;
	}*/

	return 0;
}