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


#include <hal/nrf_gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>


#define LED1_PIN NRF_GPIO_PIN_MAP(1,10)

#define INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios)
#define SW0_NODE DT_ALIAS(sw0)

#define GPPI_OUTPUT_PIN_PRIMARY LED1_PIN

#define GPPI_TIMER_INST_IDX 20
#define GPPI_TIMER2_INST_IDX 22

#define GPPI_GPIOTE_INST_IDX 20


static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_out0));

/** @brief TIMER instance used in the example. */
static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(GPPI_TIMER_INST_IDX));
static nrfx_timer_t timer_inst2 = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(22));

/** @brief GPIOTE instance used in the example. */
static nrfx_gpiote_t gpiote_inst = NRFX_GPIOTE_INSTANCE(NRF_GPIOTE_INST_GET(GPPI_GPIOTE_INST_IDX));


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
        gpio_pin_toggle_dt(&led);
    }
}


static void timer_handler2(nrf_timer_event_t event_type, void * p_context)
{
    if (event_type == NRF_TIMER_EVENT_COMPARE0)
    {   
        nrfx_timer_pause(&timer_inst);
        nrfx_timer_pause(&timer_inst2);
        gpio_pin_set_dt(&led, 1);
    }
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

    nrfx_timer_resume(&timer_inst);
    nrfx_timer_resume(&timer_inst2);

}

int gpio_input_init(bool use_ppi)
{
    int status;
    uint8_t in_channel;
    nrfx_gppi_handle_t input_h;

    if(use_ppi)
    {
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
        status = nrfx_gpiote_input_configure(&gpiote_inst, INPUT_PIN, &input_config);
        NRFX_ASSERT(status == 0);

        nrfx_gpiote_trigger_enable(&gpiote_inst, INPUT_PIN, false);

        status = nrfx_gppi_conn_alloc(nrfx_gpiote_in_event_address_get(&gpiote_inst, INPUT_PIN),
                    nrfx_timer_task_address_get(&timer_inst, NRF_TIMER_TASK_START), 
                    &input_h);
        NRFX_ASSERT(status == 0);

        status = nrfx_gppi_ep_attach(nrfx_timer_task_address_get(&timer_inst2, NRF_TIMER_TASK_START), input_h);
        NRFX_ASSERT(status == 0);

        nrfx_gppi_conn_enable(input_h);
        return status;
    }

    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

    return ret;
}

int timer_workflow_init(bool use_ppi)
{
    int status;
    nrfx_gppi_handle_t  gppi_h, timer2_h;


    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_config.p_context = "Some context";

    status = nrfx_timer_init(&timer_inst, &timer_config,  use_ppi ? NULL : timer_handler);
    NRFX_ASSERT(status == 0);

  
    status = nrfx_timer_init(&timer_inst2, &timer_config,  use_ppi ? NULL : timer_handler2);
    NRFX_ASSERT(status == 0);

  

    uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer_inst, CONFIG_GPIO_EVENT_PERIOD_US);

    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, !use_ppi);

   

    nrfx_timer_extended_compare(&timer_inst2, NRF_TIMER_CC_CHANNEL0, desired_ticks * CONFIG_GPIO_EVENTS_CNT, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                 !use_ppi);

    nrfx_timer_clear(&timer_inst);
    nrfx_timer_clear(&timer_inst2);

    nrfx_timer_enable(&timer_inst);
    nrfx_timer_enable(&timer_inst2);
    nrfx_timer_pause(&timer_inst);
    nrfx_timer_pause(&timer_inst2); 


    if(use_ppi)
    {
        status = nrfx_gppi_conn_alloc(
            nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
            nrfx_gpiote_out_task_address_get(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY),
        &gppi_h);
        NRFX_ASSERT(status == 0);

        nrfx_gppi_conn_enable(gppi_h);

        status = nrfx_gppi_conn_alloc(
        nrfx_timer_compare_event_address_get(&timer_inst2, NRF_TIMER_CC_CHANNEL0),
        nrfx_timer_task_address_get(&timer_inst, NRF_TIMER_TASK_STOP), 
        &timer2_h);
        NRFX_ASSERT(status == 0);

        status = nrfx_gppi_ep_attach(nrfx_timer_task_address_get(&timer_inst2, NRF_TIMER_TASK_STOP), timer2_h);
        NRFX_ASSERT(status == 0);

        nrfx_gppi_conn_enable(timer2_h);
        return status;

    }

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(GPPI_TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_inst, GPPI_TIMER_INST_IDX);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(GPPI_TIMER2_INST_IDX)), IRQ_PRIO_LOWEST,
            nrfx_timer_irq_handler, &timer_inst2, GPPI_TIMER2_INST_IDX);
   
    return status;
}


int gpio_output_init(bool use_ppi)
{
    int status;
    uint8_t out_channel;

    if(use_ppi)
    {
        status =nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
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

        status = nrfx_gpiote_output_configure(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY, &output_config, &task_config);
        NRFX_ASSERT(status == 0);

        nrfx_gpiote_out_task_enable(&gpiote_inst, GPPI_OUTPUT_PIN_PRIMARY);

        return status;
    }

    status = gpio_pin_configure_dt(&led, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
    NRFX_ASSERT(status == 0);

    status = gpio_pin_set_dt(&led, 1);
    NRFX_ASSERT(status == 0);

    return status;
}


int main(void)
{


gpio_output_init(IS_ENABLED(CONFIG_USE_PPI_WORKFLOW));  
gpio_input_init(IS_ENABLED(CONFIG_USE_PPI_WORKFLOW));
timer_workflow_init(IS_ENABLED(CONFIG_USE_PPI_WORKFLOW));



int err;

if (!pwm_is_ready_dt(&pwm_led0)) {
    printk("PWM device is not ready\n");
    return -ENODEV;
}

err = pwm_set_dt(&pwm_led0, PWM_USEC(CONFIG_PWM_OUTPUT_PERIOD_US), PWM_USEC(CONFIG_PWM_OUTPUT_PERIOD_US/2));
if (err) {
    printk("Error %d: failed to set PWM\n", err);
    return err;
}
    
return 0;

}

/** @} */
