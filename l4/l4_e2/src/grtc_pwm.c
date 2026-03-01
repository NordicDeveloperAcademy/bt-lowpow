#include <nrfx_grtc.h>
#include <hal/nrf_gpio.h>
#include <helpers.h>

#define GRTC_PWM_PIN 0x03

void set_grtc_pwm(uint8_t duty_cycle)
{
    nrf_gpio_pin_control_select(GRTC_PWM_PIN, NRF_GPIO_PIN_SEL_GRTC);   
    nrf_grtc_pwm_compare_set(NRF_GRTC, ( 255 * duty_cycle ) / 100);
    nrf_grtc_task_trigger(NRF_GRTC, NRF_GRTC_TASK_PWM_START);
}