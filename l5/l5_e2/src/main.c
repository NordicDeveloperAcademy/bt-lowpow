/*
 * Copyright (c) 2022 - 2025, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx_example.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>


#define INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios)
#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_out0));

/**
 * @defgroup nrfx_gppi_one_to_one_example One-to-one GPPI example
 * @{
 * @ingroup nrfx_gppi_examples
 *
 * @brief Example showing basic functionality of a nrfx_gppi helper.
 *
 * @details Application initializes nrfx_gpiote, nrfx_timer drivers and nrfx_gppi helper in a way that
 *          TIMER compare event is set up to be forwarded via PPI/DPPI to GPIOTE and toggle a pin.
 */

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_MS 2UL

/** @brief TIMER instance used in the example. */
static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(GPPI_TIMER_INST_IDX));
static nrfx_timer_t timer_inst2 = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(22));

/** @brief GPIOTE instance used in the example. */
static nrfx_gpiote_t gpiote_inst = NRFX_GPIOTE_INSTANCE(NRF_GPIOTE_INST_GET(GPPI_GPIOTE_INST_IDX));

#if !defined(__ZEPHYR__)
/* Define an IRQ handler named nrfx_timer_<GPPI_TIMER_INST_IDX>_irq_handler. */
NRFX_INSTANCE_IRQ_HANDLER_DEFINE(timer, GPPI_TIMER_INST_IDX, &timer_inst);

/* Define an IRQ handler named nrfx_gpiote_<GPPI_GPIOTE_INST_IDX>_irq_handler. */
NRFX_INSTANCE_IRQ_HANDLER_DEFINE(gpiote, GPPI_GPIOTE_INST_IDX, &gpiote_inst);
#endif

/**
 * @brief Function for handling TIMER driver events.
 *
 * @param[in] event_type Timer event.
 * @param[in] p_context  General purpose parameter set during initialization of the timer.
 *                       This parameter can be used to pass additional information to the handler
 *                       function for example the timer ID.
 */
static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    if (event_type == NRF_TIMER_EVENT_COMPARE0)
    {
        char * p_msg = p_context;
        NRFX_LOG_INFO("Timer finished. Context passed to the handler: >%s<", p_msg);
        NRFX_LOG_INFO("GPIOTE output pin: %d is %s", GPPI_OUTPUT_PIN_PRIMARY,
                      nrfx_gpiote_in_is_set(GPPI_OUTPUT_PIN_PRIMARY) ? "high" : "low");
    }
}

int enabled = false;
static void timer_handler2(nrf_timer_event_t event_type, void * p_context)
{
    if (event_type == NRF_TIMER_EVENT_COMPARE0)
    {   if (enabled) 
        {
            nrfx_timer_disable(&timer_inst);
            nrfx_timer_disable(&timer_inst2);
            enabled = false;
        }
    }
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if(!enabled) {
        enabled = true;
        nrfx_timer_enable(&timer_inst);
        nrfx_timer_enable(&timer_inst2);
    } 

}


#define MAX_REC_COUNT		1
#define NDEF_MSG_BUF_SIZE	128

#define NFC_FIELD_LED		DK_LED1
#define SYSTEM_ON_LED		DK_LED2
/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    int status;
    (void)status;

    uint8_t in_channel, out_channel;
    nrfx_gppi_handle_t input_h, gppi_h, timer2_h;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(GPPI_TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_inst, 20);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(22)), IRQ_PRIO_LOWEST,
            nrfx_timer_irq_handler, &timer_inst2, 22);
    // IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE_INST_GET(GPPI_GPIOTE_INST_IDX)), IRQ_PRIO_LOWEST,
    //             nrfx_gpiote_irq_handler, &gpiote_inst, 0);
#endif

#if 0
        status = nrfx_gpiote_channel_alloc(&gpiote_inst, &in_channel);
    NRFX_ASSERT(status == 0);
        	static const nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_NOPULL;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &in_channel,
	};

    	static const nrfx_gpiote_handler_config_t handler_config = {
		.handler = NULL,
	};
    	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};
    nrfx_gpiote_input_configure(&gpiote_inst, INPUT_PIN, &input_config);

    nrfx_gpiote_trigger_enable(&gpiote_inst, INPUT_PIN, false);

    nrfx_gppi_conn_alloc(nrfx_gpiote_in_event_address_get(&gpiote_inst, INPUT_PIN),
				   nrfx_timer_task_address_get(&timer_inst, NRF_TIMER_TASK_START), 
				   &input_h);
    
    nrfx_gppi_ep_attach(nrfx_timer_task_address_get(&timer_inst2, NRF_TIMER_TASK_START), input_h);

    nrfx_gppi_conn_enable(input_h);

#else
	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
#endif

    status = nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == 0);
    NRFX_LOG_INFO("GPIOTE status: %s",
                  nrfx_gpiote_init_check(&gpiote_inst) ? "initialized" : "not initialized");

    status = nrfx_gpiote_channel_alloc(&gpiote_inst, &out_channel);
    NRFX_ASSERT(status == 0);


    /*
     * Initialize output pin. The SET task will turn the LED on,
     * CLR will turn it off and OUT will toggle it.
     */
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

    status = nrfx_gpiote_output_configure(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY, &output_config, &task_config);
    NRFX_ASSERT(status == 0);

    nrfx_gpiote_out_task_enable(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY);

    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_config.p_context = "Some context";

    status = nrfx_timer_init(&timer_inst, &timer_config,  timer_handler);
    NRFX_ASSERT(status == 0);

    nrfx_timer_clear(&timer_inst);


    status = nrfx_timer_init(&timer_inst2, &timer_config,  timer_handler2);
    NRFX_ASSERT(status == 0);

  
    nrfx_timer_clear(&timer_inst);
    nrfx_timer_clear(&timer_inst2);


    /* Creating variable desired_ticks to store the output of nrfx_timer_ms_to_ticks function. */
    uint32_t desired_ticks = nrfx_timer_ms_to_ticks(&timer_inst, TIME_TO_WAIT_MS);
    NRFX_LOG_INFO("Time to wait: %lu ms", TIME_TO_WAIT_MS);

    /*
     * Setting the timer channel NRF_TIMER_CC_CHANNEL0 in the extended compare mode to clear
     * the timer and to trigger an interrupt if the internal counter register is equal to
     * desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    desired_ticks  = nrfx_timer_ms_to_ticks(&timer_inst2, TIME_TO_WAIT_MS*10); 
    

    nrfx_timer_extended_compare(&timer_inst2, NRF_TIMER_CC_CHANNEL0, desired_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                 true);
    /*
     * Configure endpoints of the channel so that the input timer event is connected with the output
     * pin OUT task. This means that each time the timer interrupt occurs, the LED pin will be toggled.
     */
    status = nrfx_gppi_conn_alloc(
        nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
        nrfx_gpiote_out_task_address_get(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY),
	&gppi_h);
    NRFX_ASSERT(status == 0);
    
   // nrfx_gppi_conn_enable(gppi_h);

    status = nrfx_gppi_conn_alloc(
    nrfx_timer_compare_event_address_get(&timer_inst2, NRF_TIMER_CC_CHANNEL0),
    nrfx_timer_task_address_get(&timer_inst, NRF_TIMER_TASK_STOP), 
    &timer2_h);

    nrfx_gppi_ep_attach(nrfx_timer_task_address_get(&timer_inst2, NRF_TIMER_TASK_STOP), timer2_h);

    NRFX_ASSERT(status == 0);

    //nrfx_gppi_conn_enable(timer2_h);


        int err;
    if (!pwm_is_ready_dt(&pwm_led0)) {
        printk("PWM device is not ready\n");
        return -ENODEV;
    }

    err = pwm_set_dt(&pwm_led0, PWM_MSEC(30), PWM_MSEC(15));
    if (err) {
        printk("Error %d: failed to set PWM\n", err);
        return err;
    }

    NRFX_LOG_INFO("Timer status: %s", nrfx_timer_is_enabled(&timer_inst2) ? "enabled" : "disabled");
    
    return 0;

}

/** @} */
