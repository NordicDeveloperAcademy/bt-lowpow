/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <soc.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <dk_buttons_and_leds.h>

#if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(Lesson3_Exercise2, LOG_LEVEL_INF);
#endif //CONFIG_DEVACADEMY_APP_DEBUGGING

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

#define LOOP_PERIOD_MS      2000

static void timer_expiry_handler(struct k_timer *timer);

static K_SEM_DEFINE(ble_init_ok, 0, 1);

K_TIMER_DEFINE(loop_restart_timer, timer_expiry_handler, NULL);

static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;
static struct k_work adv_work;

struct uart_data_t {
 void *fifo_reserved;
 uint8_t data[CONFIG_BT_NUS_UART_BUFFER_SIZE];
 uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static const struct bt_data ad[] = {
 BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
 BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
 BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/* STEP 8.1 - Create the variable that holds the callback for MTU negotiation */
static struct bt_gatt_exchange_params exchange_params;
#if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
static void exchange_func(struct bt_conn *conn, uint8_t att_err, struct bt_gatt_exchange_params *params);
#else
static void exchange_func(struct bt_conn *conn, uint8_t att_err, struct bt_gatt_exchange_params *params){};
#endif //CONFIG_DEVACADEMY_APP_DEBUGGING

int i = 0;

/* STEP 10.1 - Define a large data packet */
uint8_t large_data[233] =
    "a_NordicSemiconductor_NUS_LargePacket_ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
    "abcdefghijklmnopqrstuvwxyz_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ_ab"
    "cdefghijklmno_ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
    "Nordic_Developer_Academy_NordicSemiconductor_EndOfPacket";

static void adv_work_handler(struct k_work *work)
{
 int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

 if (err) {
  return;
 }

}

static void advertising_start(void)
{
 k_work_submit(&adv_work);
}

/* STEP 8.2 - Request an MTU exchange */
static void request_mtu_exchange(struct bt_conn *conn)
{
    int err;
    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(current_conn, &exchange_params);
    if (err) {
          #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
          LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
          #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
          return;
    }
}

/* STEP 7.2 - Request a data length update */
static void request_data_len_update(struct bt_conn *conn)
{
    int err;
    struct bt_conn_le_data_len_param my_data_len = {
        .tx_max_len = BT_GAP_DATA_LEN_MAX,
        .tx_max_time = BT_GAP_DATA_TIME_MAX,
    };

    err = bt_conn_le_data_len_update(current_conn, &my_data_len);
    if (err) {
          #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
          LOG_ERR("data_len_update failed (err %d)", err);
          #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
          return;
    }
}

/* STEP 12.2 Request a PHY update */
/*static void request_phy_update(struct bt_conn *conn)
{
    int err;
    const struct bt_conn_le_phy_param preferred_phy = {
        .options = BT_CONN_LE_PHY_OPT_NONE,
        .pref_rx_phy = BT_GAP_LE_PHY_2M,
        .pref_tx_phy = BT_GAP_LE_PHY_2M,
    };
    err = bt_conn_le_phy_update(conn, &preferred_phy);
    if (err) {
        #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
        LOG_ERR("bt_conn_le_phy_update() returned %d", err);
        #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    }
}*/


static void connected(struct bt_conn *conn, uint8_t err)
{
 char addr[BT_ADDR_LE_STR_LEN];

 if (err) {
  return;
 }

 bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

 current_conn = bt_conn_ref(conn);

#if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
struct bt_conn_info info;
err = bt_conn_get_info(conn, &info);
if (err) {
	LOG_ERR("bt_conn_get_info() returned %d", err);
	return;
}

double connection_interval = BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us) *1.25; // in ms
uint16_t supervision_timeout = info.le.timeout*10; // in ms
LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, info.le.latency, supervision_timeout);
#endif //CONFIG_DEVACADEMY_APP_DEBUGGING

 /* STEP 9 - Request an MTU exchange and data length update */
 request_mtu_exchange(current_conn);
 //request_data_len_update(current_conn);

 /* STEP 12.3 - Request a PHY update*/
 //request_phy_update(current_conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
 char addr[BT_ADDR_LE_STR_LEN];

 bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

 if (auth_conn) {
  bt_conn_unref(auth_conn);
  auth_conn = NULL;
 }

 if (current_conn) {
  bt_conn_unref(current_conn);
  current_conn = NULL;
 }
}

static void recycled_cb(void)
{
 advertising_start();
}

void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    double connection_interval = interval*1.25;         // in ms
    uint16_t supervision_timeout = timeout*10;          // in ms
    #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
    LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, latency, supervision_timeout);
    #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
}

void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
    uint16_t tx_len     = info->tx_max_len; 
    uint16_t tx_time    = info->tx_max_time;
    uint16_t rx_len     = info->rx_max_len;
    uint16_t rx_time    = info->rx_max_time;
    #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
    LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time, rx_time);
    #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    
}

void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
    // PHY Updated
    if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
        #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
        LOG_INF("PHY updated. New PHY: 1M");
        #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
        #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
        LOG_INF("PHY updated. New PHY: 2M");
        #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
        #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
        LOG_INF("PHY updated. New PHY: Long Range");
        #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
 .connected              = connected,
 .disconnected           = disconnected,
 .recycled               = recycled_cb,
 .le_param_updated       = on_le_param_updated,
 //.le_data_len_updated    = on_le_data_len_updated,
 //.le_phy_updated     = on_le_phy_updated,
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
     uint16_t len)
{
 char addr[BT_ADDR_LE_STR_LEN] = {0};

 bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

}

static void bt_notif_enabled_cb(enum bt_nus_send_status status)
{
    #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
    LOG_INF("Notifications enabled, starting to send packets");
    #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
    /* Start timer for sending data */
    /* STEP 10.4 - Start the timer to begin sending data packets */
    k_timer_start(&loop_restart_timer, K_NO_WAIT, K_MSEC(LOOP_PERIOD_MS));
}

static struct bt_nus_cb nus_cb = {
 .received = bt_receive_cb,
 .send_enabled = bt_notif_enabled_cb,
};

void error(void)
{
 while (true) {
  /* Spin for ever */
  k_sleep(K_MSEC(1000));
 }
}

#if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
static void exchange_func(struct bt_conn *conn, uint8_t att_err,
              struct bt_gatt_exchange_params *params)
{
    LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
    
    if (!att_err) {
        uint16_t payload_mtu = bt_gatt_get_mtu(conn) - 3;   // 3 bytes used for Attribute headers.
        LOG_INF("New MTU: %d bytes", payload_mtu);
    }
}
#endif //CONFIG_DEVACADEMY_APP_DEBUGGING

/* STEP 10.2 - Send a large packet over NUS */
static void timer_expiry_handler(struct k_timer *timer)
{
     #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
     LOG_INF("Sending data packet");
     #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
        
     large_data[0]++;
     if (large_data[0] > 'z') {
          large_data[0] = 'a';
     }
 
     if (bt_nus_send(current_conn, large_data, sizeof(large_data))) {
     #if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
          LOG_ERR("Couldn't send notification");
          #endif //CONFIG_DEVACADEMY_APP_DEBUGGING
     }
}

int main(void)
{
 int err = 0;

#if defined(CONFIG_DEVACADEMY_APP_DEBUGGING)
LOG_INF("Starting Lesson 3 Exercise 2");
#endif //CONFIG_DEVACADEMY_APP_DEBUGGING

/* STEP 10.3 - Initialize the button handler */
 /*err = dk_buttons_init(button_handler);
 if (err) {
  return 0;
 }*/

 err = bt_enable(NULL);
 if (err) {
  error();
 }

 k_sem_give(&ble_init_ok);

 if (IS_ENABLED(CONFIG_SETTINGS)) {
  settings_load();
 }

 err = bt_nus_init(&nus_cb);
 if (err) {
  return 0;
 }

 k_work_init(&adv_work, adv_work_handler);
 advertising_start();

}