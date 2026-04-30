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

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

#define LOOP_PERIOD_MS      900

static K_SEM_DEFINE(ble_init_ok, 0, 1);

/* STEP 9.3 Define timer and expiry function */
static void send_large_data(struct k_timer *timer);
K_TIMER_DEFINE(loop_restart_timer, send_large_data, NULL);

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

/* STEP 7.3.1 - Create the variable that holds the callback for MTU negotiation */
static struct bt_gatt_exchange_params exchange_params;
static void exchange_func(struct bt_conn *conn, uint8_t att_err, struct bt_gatt_exchange_params *params){};

int i = 0;

/* STEP 9.1 - Define a large data packet */
uint8_t large_data[244] =
    "a_NordicSemiconductor_NUS_LargePacket_ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
    "abcdefghijklmnopqrstuvwxyz_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ_ab"
    "cdefghijklmno_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
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

/* STEP 7.3.2 - Request an MTU exchange */
static void request_mtu_exchange(struct bt_conn *conn)
{
    int err;
    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(current_conn, &exchange_params);
    if (err) {
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
          return;
    }
}

static void request_phy_update(struct bt_conn *conn)
{
    int err;
    const struct bt_conn_le_phy_param preferred_phy = {
        .options = BT_CONN_LE_PHY_OPT_NONE,
        /* STEP 15 - Set the preferred PHY to 2M */
        .pref_rx_phy = BT_GAP_LE_PHY_2M,
        .pref_tx_phy = BT_GAP_LE_PHY_2M,
    };
    err = bt_conn_le_phy_update(conn, &preferred_phy);
    if (err) {
    }
}


static void connected(struct bt_conn *conn, uint8_t err)
{
 char addr[BT_ADDR_LE_STR_LEN];

 if (err) {
  return;
 }

 bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

 current_conn = bt_conn_ref(conn);

struct bt_conn_info info;
err = bt_conn_get_info(conn, &info);
if (err) {
	return;
}

 request_phy_update(current_conn);

 /* STEP 8 - Request an MTU exchange */
 request_mtu_exchange(current_conn);

 /* STEP 12 - Request a data length update */
 request_data_len_update(current_conn);


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

BT_CONN_CB_DEFINE(conn_callbacks) = {
 .connected              = connected,
 .disconnected           = disconnected,
 .recycled               = recycled_cb,
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
     uint16_t len)
{
 char addr[BT_ADDR_LE_STR_LEN] = {0};

 bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

}

static void bt_notif_enabled_cb(enum bt_nus_send_status status)
{
    /* STEP 9.4 - Start the timer only when notifications are enabled */
    switch (status) 
    {
        case BT_NUS_SEND_STATUS_ENABLED:
            k_timer_start(&loop_restart_timer, K_NO_WAIT, K_MSEC(LOOP_PERIOD_MS));
            break;
        case BT_NUS_SEND_STATUS_DISABLED:
            k_timer_stop(&loop_restart_timer);
            break;
        break;
    }
};

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

/* STEP 9.2 - Send a large packet over NUS */
static void send_large_data(struct k_timer *timer)
{
     large_data[0]++;
     if (large_data[0] > 'z') {
          large_data[0] = 'a';
     }
 
     if (bt_nus_send(current_conn, large_data, sizeof(large_data))) {
     }
}

int main(void)
{
 int err = 0;

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