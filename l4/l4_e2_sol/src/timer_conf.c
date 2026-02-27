#include <nrfx_timer.h>
#include <helpers.h>



#if defined(CONFIG_NRFX_TIMER00_1MHz) || defined(CONFIG_NRFX_TIMER00_128MHz)
#define TIMER00_INST_IDX 00
static nrfx_timer_t timer00_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER00_INST_IDX));
#endif

#if defined(CONFIG_NRFX_TIMER20_1MHz)
#define TIMER20_INST_IDX 20
static nrfx_timer_t timer20_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER20_INST_IDX));
#endif


static void init_timer(nrfx_timer_t *timer, uint32_t frequency_mhz)
{
    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(NRFX_MHZ_TO_HZ(frequency_mhz));
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";

    if(nrfx_timer_init(timer, &config, NULL) != 0)
    {
        return;
    }

    nrfx_timer_clear(timer);
    nrfx_timer_enable(timer);
}


#if defined(CONFIG_NRFX_TIMER00_1MHz) || defined(CONFIG_NRFX_TIMER00_128MHz)
void enable_timer00( uint32_t frequency_mhz)
{

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER00_INST_IDX)), IRQ_PRIO_LOWEST,
    nrfx_timer_irq_handler, &timer00_inst, 0);
    
    init_timer(&timer00_inst, frequency_mhz);
}
#endif

#if defined(CONFIG_NRFX_TIMER20_1MHz)
void enable_timer20( uint32_t frequency_mhz)
{
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER20_INST_IDX)), IRQ_PRIO_LOWEST,
    nrfx_timer_irq_handler, &timer20_inst, 20);
    
    init_timer(&timer20_inst, frequency_mhz);
}
#endif