
#include <zephyr/drivers/watchdog.h>
#include <helpers.h>

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

static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
int wdt_channel_id;

int init_watchdog()
{

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


void watchdog_feed()
{
     wdt_feed(wdt, wdt_channel_id);
}



