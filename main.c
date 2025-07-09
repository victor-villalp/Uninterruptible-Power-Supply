/*
PWM PF2,PF3 50% 60Hz
LM34 PD0
Voltage Sensor PD1 
Serial monitor 115200
Interrupt Handler ADC for Fan
//vd means voltage divider code
i.e. change the ts in step configure of ss2 of adc 
and change the value to CHx
*/

#define PART_TM4C123GH6PM

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stdlib.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/systick.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"
#include "driverlib/pwm.h"
#include "driverlib/pwm.c"

#define GPIO_PF0_M1PWM4         0x00050005
#define GPIO_PF1_M1PWM5         0x00050405
#define GPIO_PF2_M1PWM6         0x00050805
#define GPIO_PF3_M1PWM7         0x00050C05

//LM34 temperature sensor
uint32_t ADCValues[1];
uint32_t TempValueF ;
uint32_t TempValueC ;

//Internal Temperature Sensor
uint32_t rawTS[1];
uint32_t TSc ;
uint32_t TSf ;

//Voltage Sensor
uint32_t rawvalue[4];
uint32_t voltage;
uint32_t amps;
volatile uint32_t avv;
volatile uint32_t a;
volatile uint32_t b;
volatile uint32_t c;
volatile uint32_t wavv;

void InitConsole(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);
}

void PWMWave(void)
{
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // For pwm, external circuit and fan
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // External circuit interrupt handler
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);  // The Tiva Launchpad has two modules (0 and 1). Module 1 covers the LED pins
		
    //Configure PF1 Pins as PWM
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
		GPIOPinConfigure(GPIO_PF3_M1PWM7);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);

    //Configure PWM_GEN_3 Covers M1PWM6 and M1PWM7 
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC); 
	
    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, 20800);  // 20800 60 hz system clock divided by 2.5

    //Set PWM duty-50% (Period /2)
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6,10400); // 10400 50 % duty cycle , PWM_OUT_5 = M1PWM5= PF1
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7,10400); // PWM_OUT_6 = M1PWM6 = PF2 CHECK SCOPE

    // Enable the PWM generator
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);
		
    // Turn on the Output pins
    PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true); 
		PWMOutputInvert(PWM1_BASE, PWM_OUT_7_BIT, true);
		PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true); 
}

void ADCInit(void)
{
		// Enable ADC peripheral
		SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	  SysCtlDelay(3);
		
		// Configure sequencer, set priority
	  ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 1); // lm34
		ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 2); // Internal temperature sensor
		ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // voltage sensor
		
		// Configure sequencer steps
	  ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH7 | ADC_CTL_IE |ADC_CTL_END); // External temperature sensor
		ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_TS | ADC_CTL_IE |ADC_CTL_END); // Intenal temperature sensor, TS		
		ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH6); //ADC0 SS1 Step 0
		ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH6); //ADC0 SS1 Step 1
		ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH6); //ADC0 SS1 Step 2
		ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_CH6|ADC_CTL_END);
		
		//ADC Interrupt
		IntPrioritySet(INT_ADC0SS1, 0x00);
		IntEnable(INT_ADC0SS1);
		ADCIntEnable(ADC0_BASE,1);
		ADCSequenceEnable(ADC0_BASE, 1);//voltage sensor
		
		IntPrioritySet(INT_ADC0SS3, 0x01);
		IntEnable(INT_ADC0SS3)
		ADCIntEnable(ADC0_BASE,3); 
		ADCSequenceEnable(ADC0_BASE, 3); //lm34
}

int main()
	{
		// Set system clock, intialize console(serial monitor), PWM module, set PF1 as output for FAN
	  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	  InitConsole();
		PWMWave();
		ADCInit();
		GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_4); 
		GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);
	  IntMasterEnable();
		
		while(1){
			//Software Trigger Sequencer
			ADCProcessorTrigger(ADC0_BASE, 2);
			
			//Print data
			UARTprintf("%0d,%0d,%0d\n",TSc,TempValueC,voltage);
	       SysCtlDelay(80000000 / 12);
	    }
			
}
	
void ADC0SS3_Handler() // lm34
{
	GPIOinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
	ADCIntClear(ADC0_BASE,3);
	ADCProcessorTrigger(ADC0_BASE,3);
	ADCSequenceDataGet(ADC0_BASE, 3, ADCValues);
	TempValueF = (3.3 * ADCValues[0] * 100.0)/4096.0;
	TempValueC = (TempValueF - 32) * (5.0/9.0);

	if(TempValueC>15){
		if(TempValueC>24){
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x02);//pf1
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0x10);//pf4
		}	
	}
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4,~GPIO_PIN_4);
}	

void ADC0SS2_Handler() // ts
{
	//Get data from sequencers ss2
	ADCIntClear(ADC0_BASE,2);
	ADCSequenceDataGet(ADC0_BASE,2,rawTS);
	
	//Internal Temperature sensor
	TSc = (uint32_t)(147.5 - ((75.0*3.3 *(float)rawTS[0])) / 4096.0); //Internal Temperature  ss2
	TSf = ((TempValueC * 9) + 160) / 5;//Internal Temperature Sensor ss2
	
	if(TSc>24){
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PIN_5);//pf1
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);//pf4
	} else if (TSc<23){
		SysCtlDelay(300000);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, ~GPIO_PIN_5);//pf1
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, ~GPIO_PIN_6);//pf4
	}
}	


void ADC0SS1_Handler() // voltage sensor
{
	GPIOinWrite(GPIO_PORTC_BASE, GPIO_PIN_3, GPIO_PIN_3);
	ADCIntClear(ADC0_BASE,1);
	ADCProcessorTrigger(ADC0_BASE,1);
	ADCSequenceDataGet(ADC0_BASE, 1, rawvalue);
	avv = (rawvalue[0] + rawvalue[1] + rawvalue[2] + rawvalue[3] + 2)/4;
	voltage =  ((3.3 * avv * 1000.0)/4096.0)/1;
	
	if(voltage>2100){
		if(voltage<2500){
			GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PIN_5);//pf1
			GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);//pf4
		}
	}
	GPIOinWrite(GPIO_PORTC_BASE, GPIO_PIN_3, GPIO_PIN_3);
}