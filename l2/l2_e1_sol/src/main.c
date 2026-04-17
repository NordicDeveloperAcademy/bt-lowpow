/* STEP 4 - Turn off the low frequency clock, RAM sections for CRACKEN and ICACHE and enter system off */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/timer/system_timer.h>

int main(void)
{
	//Turn off GRTC and LF clock:
	//sys_clock_disable();

	//Turn off RAM sections for CRACKEN and ICACHE:
	//NRF_MEMCONF->POWER[1].RET &= ~0xE;

	/* STEP 5 - Increase VDD leakage */
	//NRF_P0->PIN_CNF[4] = 0xD;  // Set P0.04 as output with a pullup to VDD
	//NRF_P0->OUTCLR = 1<<4;     // Set P0.04 output to low (GND)

	//Enter system OFF:
	//sys_poweroff();

	return 0;
}
