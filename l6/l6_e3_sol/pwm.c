#include "helpers.h"
#include <nrfx_pwm.h>

#define PWM_INST_IDX 20

static nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(NRF_PWM_INST_GET(PWM_INST_IDX));

static nrf_pwm_sequence_t simple_seq =
{
    .values = {0},
    .length = 1,
    .repeats = 1,
    .end_delay = 0
};


static nrf_pwm_sequence_t blink =
{
    .values = {0},
    .length = 0,
    .repeats = 1,
    .end_delay = 0
};

static nrf_pwm_sequence_t falling =
{
    .values = {0},
    .length = 0,
    .repeats = 1,
    .end_delay = 0
};


static nrf_pwm_sequence_t rising =
{
    .values = {0},
    .length = 0,
    .repeats = 1,
    .end_delay = 0
};

typedef enum 
{
    SEQ_RISING = 0,
    SEQ_BLINK ,
    SEQ_FALLING ,
    SEQ_TOP
} pattern_phase_t;

static pattern_phase_t next_phase(pattern_phase_t  phase)
{
    switch(phase)
    {
        case SEQ_RISING:
            return SEQ_BLINK;
        case SEQ_BLINK:
            return SEQ_FALLING;  
        case SEQ_FALLING:
            return SEQ_RISING;
        default:
            return SEQ_RISING;
    }
}

nrf_pwm_sequence_t * seq_array[SEQ_TOP];
pattern_phase_t current_phase = SEQ_RISING;
/**
 * @brief Function for handling PWM driver events.
 *
 * @param[in] event_type PWM event.
 * @param[in] p_context  General purpose parameter set during initialization of
 *                       the timer. This parameter can be used to pass
 *                       additional information to the handler function.
 */
static void pwm_handler(nrfx_pwm_event_type_t event_type, void * p_context)
{
    int next = false;
    next = !next;
    switch(event_type)
    {
        case NRFX_PWM_EVENT_END_SEQ0:
            current_phase = next_phase(current_phase);
            nrfx_pwm_sequence_update(&pwm_instance, 0, seq_array[next_phase(current_phase)]);
            break;
        case NRFX_PWM_EVENT_END_SEQ1:
            current_phase = next_phase(current_phase);
            nrfx_pwm_sequence_update(&pwm_instance, 1, seq_array[next_phase(current_phase)]);
            break;
        default:
            break;
    }
}


static int init_pwm( bool run_complex_pattern)
{
    int status;
    (void)status;
 
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_PWM_INST_GET(PWM_INST_IDX)), IRQ_PRIO_LOWEST,
            nrfx_pwm_irq_handler, &pwm_instance, 20);

            
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG( NRF_GPIO_PIN_MAP(1,10), NRF_PWM_PIN_NOT_CONNECTED,
                                                    NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

    config.base_clock = NRF_PWM_CLK_16MHz;
    config.top_value = 1000;

    status = nrfx_pwm_init(&pwm_instance, &config, run_complex_pattern ? pwm_handler:NULL, NULL);
    NRFX_ASSERT(status == 0);

    if(run_complex_pattern)
    {
        nrfx_pwm_complex_playback(&pwm_instance, &rising, &blink, 1, NRFX_PWM_FLAG_LOOP |NRFX_PWM_FLAG_SIGNAL_END_SEQ0 | NRFX_PWM_FLAG_SIGNAL_END_SEQ1 );
    }
    else
    {
        nrfx_pwm_simple_playback(&pwm_instance, &simple_seq, 1, NRFX_PWM_FLAG_LOOP);
    }

    return 0;
}

int init_pwm_simple(const uint16_t * pattern_ptr, uint32_t pattern_size)
{
    simple_seq.values.p_raw = pattern_ptr;
    simple_seq.length = pattern_size;

    init_pwm(false);
    return 0;
}

int init_pwm_complex( const uint16_t * pattern_rising_ptr, uint32_t pattern_rising_size, const uint16_t * pattern_blink_ptr, 
                     uint32_t pattern_blink_size, const uint16_t * pattern_falling_ptr, uint32_t pattern_falling_size )
{
    rising.values.p_raw = pattern_rising_ptr;
    rising.length = pattern_rising_size;

    blink.values.p_raw = pattern_blink_ptr;
    blink.length = pattern_blink_size;

    falling.values.p_raw = pattern_falling_ptr;
    falling.length = pattern_falling_size;

    seq_array[SEQ_RISING] = &rising;
    seq_array[SEQ_BLINK] = &blink;
    seq_array[SEQ_FALLING] = &falling;

    init_pwm(true);
    return 0;
} 
