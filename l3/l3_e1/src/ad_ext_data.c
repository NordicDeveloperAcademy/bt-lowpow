/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
/*
 * Dummy data to demonstrate extended advertising packet.
 * In a real application, replace with actual sensor readings and device info.
 */
/*
 * Extended advertising data (192 bytes total, 10 AD structures)
 *
 *  #  AD Type                Code   Bytes  Description
 *  -  ---------------------  -----  -----  ----------------------------
 *  1  Flags                  0x01       3  BLE-only (no BR/EDR)
 *  2  Complete Local Name    0x09      15  "Nordic_Beacon"
 *  3  URI                    0x24      23  https://www.nordicsemi.com
 *  4  Manufacturer Data      0xFF     103  Nordic (0x0059) sensor payload
 *  5  Service Data (16-bit)  0x16      15  Device Information
 *  6  Service Data (16-bit)  0x16       5  Battery Service
 *  7  Appearance             0x19       4  Generic Sensor
 *  8  TX Power Level         0x0A       3  0 dBm
 *  9  UUID128 Complete       0x07      18  Custom vendor service
 * 10  LE Role                0x1C       3  Peripheral only
 *                                   -----
 *                                     192
 */


static const uint8_t manuf_data[] = {
	/* Company ID */
	0x59, 0x00,

	/* Device info */
	0x02,                   /* Device type: nRF54L15 beacon */
	0x01, 0x00, 0x00,       /* Firmware: 1.0.0 */

	/* Environmental sensors */
	0xE8, 0x00,             /* Temperature: 23.2 C */
	0xF4, 0x01,             /* Humidity: 50.0 % */
	0x5F,                   /* Battery: 95 % */
	0x00, 0x01, 0x51, 0x80, /* Uptime: 86400 s (1 day) */
	0x00, 0x01, 0x8C, 0xA5, /* Pressure: 1013.25 hPa */
	0x02, 0x1C,             /* Light: 540 lux */
	0x03,                   /* UV index: 3 */
	0x00, 0x2A,             /* AQI: 42 */
	0x01, 0x9F,             /* CO2: 415 ppm */
	0x00, 0x0C,             /* PM2.5: 12 ug/m3 */
	0x00, 0x19,             /* PM10: 25 ug/m3 */
	0x2D,                   /* Noise: 45 dB */

	/* Location */
	0x03, 0xBA, 0x5A, 0x40, /* Latitude: 63.430208 N */
	0x00, 0x63, 0xB3, 0x00, /* Longitude: 10.395392 E */
	0x00, 0x2A,             /* Altitude: 42 m */

	/* IMU - Accelerometer (mg) */
	0x00, 0x10,             /* X:  16 */
	0xFF, 0xF0,             /* Y: -16 */
	0x03, 0xE8,             /* Z: 1000 (1 g) */

	/* IMU - Gyroscope (0.01 dps) */
	0x00, 0x05,             /* X:  0.05 */
	0xFF, 0xFB,             /* Y: -0.05 */
	0x00, 0x00,             /* Z:  0.00 */

	/* IMU - Magnetometer (0.1 uT) */
	0x00, 0xC8,             /* X:  20.0 */
	0xFF, 0x38,             /* Y: -20.0 */
	0x01, 0xF4,             /* Z:  50.0 */

	/* System */
	0x01, 0x4A,             /* Supply voltage: 3.30 V */
	0x01, 0x1D,             /* MCU temperature: 28.5 C */
	0x00, 0x00, 0x27, 0x10, /* TX count: 10000 */
	0x00, 0x00,             /* Error/reset count: 0 */
	0x00, 0x07,             /* Status: sensors OK | GPS fix | BLE active */
	0x01,                   /* Configuration ID */
	0x03,                   /* Hardware revision */
	'N','R','F','5','4','L','1','5', /* Serial number */

	/* Reserved (20 bytes) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/* Service data: Device Information (UUID 0x180A) */
static const uint8_t svc_data_dev_info[] = {
	0x0A, 0x18,
	'n', 'R', 'F', '5', '4', 'L', '1', '5', '-', 'D', 'K',
};

/* Service data: Battery Service (UUID 0x180F) */
static const uint8_t svc_data_battery[] = {
	0x0F, 0x18,
	0x5F, /* 95 % */
};

/* Appearance: Generic Sensor (0x0540) */
static const uint8_t appearance_data[] = {
	0x40, 0x05,
};

/* Custom vendor UUID: a3c87b14-d29e-4f61-b8a5-6e3d0fc218e7 */
static const uint8_t custom_uuid[] = {
	0xE7, 0x18, 0xC2, 0x0F, 0x3D, 0x6E, 0xA5, 0xB8,
	0x61, 0x4F, 0x9E, 0xD2, 0x14, 0x7B, 0xC8, 0xA3,
};


const struct bt_data ad_ext[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_URI,
		      0x17, '/', '/', 'w', 'w', 'w', '.',
		      'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
		      'c', 'o', 'm'),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, manuf_data, sizeof(manuf_data)),
	BT_DATA(BT_DATA_SVC_DATA16, svc_data_dev_info, sizeof(svc_data_dev_info)),
	BT_DATA(BT_DATA_SVC_DATA16, svc_data_battery, sizeof(svc_data_battery)),
	BT_DATA(BT_DATA_GAP_APPEARANCE, appearance_data, sizeof(appearance_data)),
	BT_DATA_BYTES(BT_DATA_TX_POWER, 0x00),
	BT_DATA(BT_DATA_UUID128_ALL, custom_uuid, sizeof(custom_uuid)),
	BT_DATA_BYTES(BT_DATA_LE_ROLE, 0x00),
};

const size_t ad_ext_size = ARRAY_SIZE(ad_ext);
