#ifndef L4_E2_SOL_INC_HELPERS_H
#define L4_E2_SOL_INC_HELPERS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#if defined(CONFIG_WATCHDOG)

/* @brief Initialize the watchdog timer */
int init_watchdog();

/* @brief Feed the watchdog timer */    
void watchdog_feed();
#endif

#if defined(CONFIG_NRFX_PWM_GRTC)

/* @brief Set the GRTC PWM duty cycle   
    @param duty_cycle The duty cycle value in %
***/
void set_grtc_pwm(uint8_t duty_cycle);  
#endif


#if defined(CONFIG_NRFX_TIMER) 

#define TIMER_1MHZ (1)
#define TIMER_128MHZ (128)

#endif

#if defined(CONFIG_NRFX_TIMER00_1MHz) || defined(CONFIG_NRFX_TIMER00_128MHz)

/* @brief Enable TIMER00 with the specified frequency
    @param frequency_mhz The frequency in MHz
***/
void enable_timer00( uint32_t frequency_mhz);
#endif

#if defined(CONFIG_NRFX_TIMER20_1MHz)

/* @brief Enable TIMER20 with the specified frequency
    @param frequency_mhz The frequency in MHz
***/
void enable_timer20( uint32_t frequency_mhz);
#endif

#if defined(CONFIG_PWM)

/* @brief Set the PWM output duty cycle
    @param duty_cycle The duty cycle value in %
***/
int set_pwm_out(uint8_t duty_cycle);
#endif

#endif      /* L4_E2_SOL_INC_HELPERS_H */