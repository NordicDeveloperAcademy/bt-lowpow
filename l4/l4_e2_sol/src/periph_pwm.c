#include <zephyr/drivers/pwm.h>
#include <helpers.h>

#define PWM_PERIOD  PWM_USEC(7812)

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_out0));

int set_pwm_out(uint8_t duty_cycle)
{
    int err;
    if (!pwm_is_ready_dt(&pwm_led0)) {
        printk("PWM device is not ready\n");
        return -ENODEV;
    }

    err = pwm_set_dt(&pwm_led0, PWM_PERIOD, ( duty_cycle * PWM_PERIOD ) / 100);
    if (err) {
        printk("Error %d: failed to set PWM\n", err);
        return err;
    }

    return 0;
}