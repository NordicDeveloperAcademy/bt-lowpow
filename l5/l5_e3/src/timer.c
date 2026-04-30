
#include <nrfx_timer.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>
#include "helpers.h"


/** @brief TIMER instance used in the example. */
#define TIMER_INST_IDX              20
#define TIMER_COMPARE_CH0_VALUE     (1000)
#define TIMER_FREQUENCY_MHZ         (16)

static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER_INST_IDX));

static uint32_t _pattern_size;
static const uint16_t * _pattern_ptr;

#define GPPI_GPIOTE_INST_IDX 20
static nrfx_gpiote_t gpiote_inst = NRFX_GPIOTE_INSTANCE(NRF_GPIOTE_INST_GET(GPPI_GPIOTE_INST_IDX));

#define LED1_PIN NRF_GPIO_PIN_MAP(1,10)

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
        timer_inst.p_reg->CC[NRF_TIMER_CC_CHANNEL1] = _pattern_ptr[index];
        index++;

        if(index >= _pattern_size )
        {
            index = 0;
        }
    }
}

static void init_timer_ddpi_output( void )
{
    int status =nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    uint8_t out_channel;
    nrfx_gppi_handle_t led_set, led_clr;

    NRFX_ASSERT(status == 0);

    status = nrfx_gpiote_channel_alloc(&gpiote_inst, &out_channel);
    NRFX_ASSERT(status == 0);

    static const nrfx_gpiote_output_config_t output_config =
    {
    .drive = NRF_GPIO_PIN_S0S1,
    .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
    .pull = NRF_GPIO_PIN_NOPULL,
    };

    const nrfx_gpiote_task_config_t task_config =
    {
    .task_ch = out_channel,
    .polarity = NRF_GPIOTE_POLARITY_TOGGLE,
    .init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
    };

    status = nrfx_gpiote_output_configure(&gpiote_inst, LED1_PIN, &output_config, &task_config);
    NRFX_ASSERT(status == 0);

    nrfx_gpiote_out_task_enable(&gpiote_inst, LED1_PIN);

    status = nrfx_gppi_conn_alloc(
            nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL1),
            nrfx_gpiote_set_task_address_get(&gpiote_inst, LED1_PIN),
        &led_set);
        NRFX_ASSERT(status == 0);

        status = nrfx_gppi_conn_alloc(
            nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
            nrfx_gpiote_clr_task_address_get(&gpiote_inst, LED1_PIN),
        &led_clr);
        NRFX_ASSERT(status == 0);

    nrfx_gppi_conn_enable(led_set);
    nrfx_gppi_conn_enable(led_clr);
    
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int init_timer( const uint16_t * pattern_ptr, uint32_t pattern_size, bool enable_pattern)
{
    int status;
    (void)status;
    NRFX_ASSERT(pattern_ptr != NULL);
    NRFX_ASSERT(pattern_size > 0);
    _pattern_ptr =  pattern_ptr;
    _pattern_size = pattern_size;

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_inst, TIMER_INST_IDX);

    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(NRFX_MHZ_TO_HZ(TIMER_FREQUENCY_MHZ));
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";

    status = nrfx_timer_init(&timer_inst, &config, enable_pattern ? timer_handler : NULL);
    NRFX_ASSERT(status == 0);

    nrfx_timer_clear(&timer_inst);

    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, TIMER_COMPARE_CH0_VALUE, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, enable_pattern? true : false);

    nrfx_timer_compare(&timer_inst, NRF_TIMER_CC_CHANNEL1, _pattern_ptr[0], enable_pattern? true : false);

    if(enable_pattern)
    {
       init_timer_ddpi_output();
    }

    nrfx_timer_enable(&timer_inst);

    return 0;
}

/** @} */