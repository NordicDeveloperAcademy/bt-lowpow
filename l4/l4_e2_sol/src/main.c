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

#if CONFIG_WATCHDOG
    #include <zephyr/drivers/watchdog.h>
#endif

#if CONFIG_NRFX_TIMER
    #include <nrfx_timer.h>
    #define TIMER_INST_IDX 20
    static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER_INST_IDX));

#endif  

#if CONFIG_NRFX_PWM_GRTC
    #include <nrfx_grtc.h>
    #include <hal/nrf_gpio.h>
    #define GRTC_PWM_PIN 0x03
    #define GRTC_PWM_PULSE (255/4) 
#endif

#if CONFIG_PWM
    #include <zephyr/drivers/pwm.h>
    #define PWM_PERIOD  PWM_USEC(7812)
    #define PWM_PULSE  ( ( PWM_PERIOD) / 4 ) 
    static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_out0));
#endif

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

#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
                 enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d %s\n", addr, level, err,
               bt_security_err_to_str(err));
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
    .recycled         = recycled_cb,
#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_LBS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
           bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

static void app_led_cb(bool led_state)
{

}

static bool app_button_cb(void)
{
    return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
    .led_cb    = app_led_cb,
    .button_cb = app_button_cb,
};


#if CONFIG_WATCHDOG
 
#ifndef WDT_MAX_WINDOW
#define WDT_MAX_WINDOW  5000U
#endif

#ifndef WDT_MIN_WINDOW
#define WDT_MIN_WINDOW  0U
#endif


static struct wdt_timeout_cfg wdt_config = {
        .window = {
            .min = WDT_MIN_WINDOW,
            .max = WDT_MAX_WINDOW
        },
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC
};

static int init_watchdog(const struct device * wdt)
{
    int wdt_channel_id;
    int err = -1;
    printk("Watchdog sample application\n");

    if (!device_is_ready(wdt)) {
        printk("%s: device not ready.\n", wdt->name);
        return err;
    }

    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id == -ENOTSUP) {
        /* IWDG driver for STM32 doesn't support callback */
        printk("Callback support rejected, continuing anyway\n");
        wdt_config.callback = NULL;
        wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    }
    if (wdt_channel_id < 0) {
        printk("Watchdog install error\n");
        return wdt_channel_id;
    }

    err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (err < 0) {
        printk("Watchdog setup error\n");
        return err;
    }
    
    wdt_feed(wdt, wdt_channel_id);

    return wdt_channel_id;
}
#endif

#if CONFIG_NRFX_PWM_GRTC
static void set_grtc_pwm(uint8_t duty_cycle)
{
    nrf_gpio_pin_control_select(GRTC_PWM_PIN, NRF_GPIO_PIN_SEL_GRTC);
    nrf_grtc_pwm_compare_set(NRF_GRTC, duty_cycle);
    nrf_grtc_task_trigger(NRF_GRTC, NRF_GRTC_TASK_PWM_START);
}
#endif

#if CONFIG_NRFX_TIMER
static void enable_timer(nrfx_timer_t * timer, uint32_t frequency_mhz)
{
     nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(NRFX_MHZ_TO_HZ(frequency_mhz));
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";

    if(nrfx_timer_init(timer, &config, NULL) != 0)
    {
        return;
    }
    nrfx_timer_clear(timer);
    nrfx_timer_enable(timer);
}
#endif

int main(void)
{
    int err;

    #if CONFIG_NRFX_PWM_GRTC
        set_grtc_pwm(GRTC_PWM_PULSE);   
    #endif    

    #if CONFIG_PWM
        pwm_set_dt(&pwm_led0, PWM_PERIOD, PWM_PULSE);
    #endif

    printk("Starting Bluetooth Peripheral LBS sample\n");

    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return 0;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return 0;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Bluetooth initialized\n");

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    err = bt_lbs_init(&lbs_callbacs);
    if (err) {
        printk("Failed to init LBS (err:%d)\n", err);
        return 0;
    }

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    //WDG
#if CONFIG_WATCHDOG
    const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
    int wdt_channel_id = init_watchdog(wdt);

    if(wdt_channel_id <0) {
        printk("Failed to initialize watchdog (err %d)\n", wdt_channel_id);
        return 0;
    }
#endif


#if CONFIG_NRFX_TIMER
    //TIMER
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
    nrfx_timer_irq_handler, &timer_inst, 20);
    enable_timer(&timer_inst, 1);

#endif

    while (1) 
    {
        #if CONFIG_WATCHDOG
        wdt_feed(wdt, wdt_channel_id);
        #endif
        k_sleep(K_MSEC(1000));
    }
}
