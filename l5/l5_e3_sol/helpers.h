
#ifndef HELPERS_H
#define HELPERS_H

#include <nrfx_timer.h>
#include <hal/nrf_gpio.h>
#include <zephyr/drivers/gpio.h>

/** @brief Number of entries in each pattern lookup table. */
#define PATTERN_SIZE 5000

/** @brief Linear triangle wave pattern (100–900), rising then falling. */
extern uint16_t pattern[PATTERN_SIZE];

/** @brief Sine wave pattern (100–900), 10 full periods. */
extern uint16_t pattern2[PATTERN_SIZE];

/**
 * @brief Initialize PWM with a single repeating pattern.
 *
 * @param[in] pattern_ptr  Pointer to the pattern lookup table.
 * @param[in] pattern_size Number of entries in the pattern.
 *
 * @retval 0 on success.
 * @retval Negative errno on failure.
 */
int init_pwm_simple(const uint16_t * pattern_ptr, uint32_t pattern_size);

/**
 * @brief Initialize PWM with a three-phase pattern (rising, blink, falling).
 *
 * @param[in] pattern_rising_ptr   Pointer to the rising phase pattern.
 * @param[in] pattern_rising_size  Number of entries in the rising pattern.
 * @param[in] pattern_blink_ptr    Pointer to the blink phase pattern.
 * @param[in] pattern_blink_size   Number of entries in the blink pattern.
 * @param[in] pattern_falling_ptr  Pointer to the falling phase pattern.
 * @param[in] pattern_falling_size Number of entries in the falling pattern.
 *
 * @retval 0 on success.
 * @retval Negative errno on failure.
 */
int init_pwm_complex( const uint16_t * pattern_rising_ptr, uint32_t pattern_rising_size,
                      const uint16_t * pattern_blink_ptr, uint32_t pattern_blink_size, 
                      const uint16_t * pattern_falling_ptr, uint32_t pattern_falling_size);

/**
 * @brief Initialize a hardware timer to drive an LED using the given pattern.
 *
 * @param[in] led_spec       Pointer to the GPIO device-tree spec for the LED.
 * @param[in] pattern_ptr    Pointer to the pattern lookup table.
 * @param[in] pattern_size   Number of entries in the pattern.
 * @param[in] enable_pattern If true, start pattern playback immediately.
 *
 * @retval 0 on success.
 * @retval Negative errno on failure.
 */
int init_timer(struct gpio_dt_spec * led_spec, const uint16_t * pattern_ptr, uint32_t pattern_size, bool enable_pattern);   


#endif /* HELPERS_H */