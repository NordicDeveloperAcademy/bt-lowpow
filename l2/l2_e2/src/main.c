/* STEP 1.2 - Add advertising parameters, enable the Bluetooth stack and start advertising */
#include <zephyr/bluetooth/bluetooth.h>

#define DEVICE_NAME "power" 
/* STEP 2.1 - Set the advertising interval to 100 ms */
#define ADV_INTERVAL_MS 100

const struct bt_le_adv_param *adv_params =
	BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_NONE,
		BT_GAP_MS_TO_ADV_INTERVAL(ADV_INTERVAL_MS),
		BT_GAP_MS_TO_ADV_INTERVAL(ADV_INTERVAL_MS),
		NULL);

const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME)),
};
int main(void)
{
	bt_enable(NULL);
	bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
	return 0;
}
