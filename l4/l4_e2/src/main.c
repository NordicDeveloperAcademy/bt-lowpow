/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/lbs.h>
#include <zephyr/settings/settings.h>
#include <helpers.h>


#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RANDOM_BYTES_COUNT      16

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

#define USER_LED                DK_LED3

#define USER_BUTTON             DK_BTN1_MSK

#define BT_LE_ADV_CONN_1000                                                                      \
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MIN,  \
            NULL)

static bool app_button_state;
static struct k_work adv_work;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN_1000, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
        return;
    }

    printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}


static void recycled_cb(void)
{
    printk("Connection object available from previous conn. Disconnect is complete!\n");
    advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
    .recycled         = recycled_cb,
};

static void app_led_cb(bool led_state)
{
    (void) led_state;
}

static bool app_button_cb(void)
{
    return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
    .led_cb    = app_led_cb,
    .button_cb = app_button_cb,
};


int main(void)
{
    int err;

    printk("Starting Bluetooth Peripheral LBS sample\n");

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Bluetooth initialized\n");

    err = bt_lbs_init(&lbs_callbacs);
    if (err) {
        printk("Failed to init LBS (err:%d)\n", err);
        return 0;
    }

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    #if CONFIG_WATCHDOG
        if( init_watchdog() <0) {
            printk("Failed to initialize watchdog \n");
            return 0;
        }
    #endif

    #if defined(CONFIG_NRFX_TIMER00_1MHz)
        enable_timer00(TIMER_1MHZ);
    #endif

    #if defined(CONFIG_NRFX_TIMER00_128MHz)
        enable_timer00(TIMER_128MHZ);
    #endif

    #if defined(CONFIG_NRFX_TIMER20_1MHz)
        enable_timer20(TIMER_1MHZ);
    #endif

    #if defined(CONFIG_NRFX_PWM_GRTC)   
        set_grtc_pwm(CONFIG_PWM_DUTY_CYCLE);   
    #endif    

    #if defined(CONFIG_PWM) 
    if ( set_pwm_out(CONFIG_PWM_DUTY_CYCLE ) < 0) {
        printk("Failed to set PWM output \n");
        return 0;
    }
    #endif

    while (1) 
    {
        #if defined(CONFIG_WATCHDOG)
           watchdog_feed();
        #endif
            k_sleep(K_MSEC(1000));
    }
}
