#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"


#define PWM_FREQUENCY 55

/* Global variables for storing the speed of LED colour change and relative intensities. */
uint32_t rg = 1, br = 0, gb = 0, speed = 1;

int main(void)
{

	// Variables related to PWM and RGB intensities.
	volatile uint32_t ui32Load;
	volatile uint32_t ui32PWMClock;
	volatile uint32_t ui8AdjustR, ui8AdjustG, ui8AdjustB;

	// Intensities of R,G and B LEDs
	ui8AdjustR = 254;
	ui8AdjustG = 10;
	ui8AdjustB = 10;

	// System Clock and Peripheral setup.
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// PWM configuration.
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);

	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8AdjustR * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8AdjustB * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8AdjustG * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);

	PWMGenEnable(PWM1_BASE, PWM_GEN_3);

	// State variables for knowing the switch combination pressed.
	int sw2Count = 0, longPressThreshold = 20;
	int sw1Count = 0, sw1PressCount = 0;
	int manualMode = 0;
	int timeOutThreshold = 20, timeOut = 0;

	// Code to continually change the LED intensities based on the current state.
	// The current state is determined by the previous state and the key combination pressed.
	while(1)
	{
		if (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00){
			if (sw2Count == 0) {
				// If this was pressed first time.
				sw1Count = 0;
				sw1PressCount = 0;
				timeOut = 0;
			}
			sw2Count++;
			if (sw2Count >= longPressThreshold){
				// This is case for long press.
				sw2Count = longPressThreshold;
			}
		}
		else{
			// Reset all
			sw2Count = 0;
		}


		if (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00){
			sw1Count++;
			timeOut = 0;
			if (sw1PressCount == 0){
				if (sw1Count >= longPressThreshold){
					sw1Count = longPressThreshold;
				}
			}
		}
		else {
			timeOut++;
			if (timeOut >= timeOutThreshold){
				timeOut = timeOutThreshold;
			}

			if (sw1Count < longPressThreshold && sw1Count > 0){
				sw1PressCount++;
			}
			sw1Count = 0;
		}

		if (sw2Count == longPressThreshold){
			// sw2 is pressed long.
			if (sw1PressCount == 2){
				manualMode = 2;
				ui8AdjustR = ui8AdjustB = ui8AdjustG = 10;
				ui8AdjustG = 254;
				sw2Count = 0; sw1Count = 0; sw1PressCount = 0; timeOut = 0;
			}
			else if (sw1PressCount == 1 && timeOut == timeOutThreshold){
				manualMode = 1;
				ui8AdjustR = ui8AdjustB = ui8AdjustG = 10;
				ui8AdjustR = 254;
				sw2Count = 0; sw1Count = 0; sw1PressCount = 0; timeOut = 0;
			}
			else if (sw1Count == longPressThreshold){
				manualMode = 3;
				ui8AdjustR = ui8AdjustB = ui8AdjustG = 10;
				ui8AdjustB = 254;
				sw2Count = 0; sw1Count = 0; sw1PressCount = 0; timeOut = 0;
			}
		}



		if (manualMode==0){

			if (rg) {
				ui8AdjustR = ui8AdjustR - speed/10;
				ui8AdjustG = ui8AdjustG + speed/10;
			} else if (gb) {
				ui8AdjustG = ui8AdjustG - speed/10;
				ui8AdjustB = ui8AdjustB + speed/10;
			} else if (br) {
				ui8AdjustB = ui8AdjustB - speed/10;
				ui8AdjustR = ui8AdjustR + speed/10;
			}

			if (ui8AdjustR < 10)
			{
				ui8AdjustR = 10;
			}
			if (ui8AdjustG < 10)
			{
				ui8AdjustG = 10;
			}
			if (ui8AdjustB < 10)
			{
				ui8AdjustB = 10;
			}

			if (ui8AdjustR > 254)
			{
				ui8AdjustR = 254;
				br = 0;
				rg = 1;
			}
			if (ui8AdjustG > 254)
			{
				ui8AdjustG = 254;
				rg = 0;
				gb = 1;
			}
			if (ui8AdjustB > 254)
			{
				ui8AdjustB = 254;
				gb = 0;
				br = 1;
			}

			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
			{
				speed--;
				if (speed < 1) speed = 1;
			}

			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
			{
				speed++;
				if (speed > 100) speed = 100;
			}

		}
		else {
			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
			{
				if (manualMode == 1){
					ui8AdjustR++;
					if (ui8AdjustR > 254){
						ui8AdjustR = 254;
					}
				}
				else if (manualMode == 2){
					ui8AdjustG++;
					if (ui8AdjustG > 254){
						ui8AdjustG = 254;
					}
				}
				else if (manualMode == 3){
					ui8AdjustB++;
					if (ui8AdjustB > 254){
						ui8AdjustB = 254;
					}
				}
			}

			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
			{
				if (manualMode == 1){
					ui8AdjustR--;
					if (ui8AdjustR < 10){
						ui8AdjustR = 10;
					}
				}
				else if (manualMode == 2){
					ui8AdjustG--;
					if (ui8AdjustG < 10){
						ui8AdjustG = 10;
					}
				}
				else if (manualMode == 3){
					ui8AdjustB--;
					if (ui8AdjustB < 10){
						ui8AdjustB = 10;
					}
				}
			}

		}


		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8AdjustR * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8AdjustB * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8AdjustG * ui32Load / 1000);

		SysCtlDelay(200000);

	}

}
