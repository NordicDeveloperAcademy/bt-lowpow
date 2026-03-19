
#include <nrfx_timer.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include "helpers.h"


/** @brief TIMER instance used in the example. */
#define TIMER_INST_IDX              20
#define TIMER_COMPARE_CH0_VALUE     (1000)
#define TIMER_FREQUENCY_MHZ         (16)

static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER_INST_IDX));

static uint32_t _pattern_size;
static const uint16_t * _pattern_ptr;
static struct gpio_dt_spec * _led;

/**
 * @brief Function for handling TIMER driver events. 
 *
 * @param[in] event_type Timer event.
 * @param[in] p_context  General purpose parameter set during initialization of
 *                       the timer. This parameter can be used to pass
 *                       additional information to the handler function, for
 *                       example, the timer ID.
 */
static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    if(event_type == NRF_TIMER_EVENT_COMPARE0)
    {
        static uint32_t index =1;
        gpio_pin_set_dt(_led, 0);
        nrfy_timer_task_trigger(timer_inst.p_reg, NRF_TIMER_TASK_CLEAR);
        timer_inst.p_reg->CC[NRF_TIMER_CC_CHANNEL1] = _pattern_ptr[index];
        index++;

        if(index >= _pattern_size )
        {
            index = 0;
        }
    }
    else if(event_type == NRF_TIMER_EVENT_COMPARE1)
    {
        gpio_pin_set_dt(_led, 1);
    }
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int init_timer(struct gpio_dt_spec * led_spec, const uint16_t * pattern_ptr, uint32_t pattern_size, bool enable_pattern)
{
    int status;
    (void)status;
    NRFX_ASSERT(pattern_ptr != NULL);
    NRFX_ASSERT(pattern_size > 0);
    _pattern_ptr =  pattern_ptr;
    _pattern_size = pattern_size;
    _led = led_spec;

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_inst, TIMER_INST_IDX);


    gpio_pin_configure_dt(_led, GPIO_OUTPUT | GPIO_ACTIVE_LOW);

    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(NRFX_MHZ_TO_HZ(TIMER_FREQUENCY_MHZ));
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";

    status = nrfx_timer_init(&timer_inst, &config, enable_pattern ? timer_handler : NULL);
    NRFX_ASSERT(status == 0);

    nrfx_timer_clear(&timer_inst);

    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, TIMER_COMPARE_CH0_VALUE, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    nrfx_timer_compare(&timer_inst, NRF_TIMER_CC_CHANNEL1, _pattern_ptr[0], true);

    nrfx_timer_enable(&timer_inst);

    return 0;
}

/** @} */