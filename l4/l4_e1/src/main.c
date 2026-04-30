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


/* STEP 6 - Advertise only on channels 37 and 38 */


/* STEP 8.1 - Define the advertising parameter for connectable and non-connectable advertising */


/* STEP 9.2 - Define the advertising parameter for extended advertising */


/* STEP 2 - Define the advertising packet */


/* STEP 7.1 - Define the minimal advertising packet */


/* STEP 8.2 - Define variable to keep track of advertising types */


/* STEP 8.3 -  Define variable to keep track of the current advertising type */


/* STEP 8.4 - Define the scan response packet */


/* STEP 9.3 - Declare the callback struct */

/* STEP 8.5 - Enable different advertising types upon button presses */

int main(void)
{
	int err;

	/* STEP 8.6 - Register the button handler */


	err = bt_enable(NULL);
	if (err) {
		return -1;
	}

	/* STEP 9.4 - Create the advertising set and start extended advertising */


	/* STEP 7.2 - Start advertising with the minimal advertising packet */
	err = bt_le_adv_start(param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		return -1;
	}

	return 0;
}