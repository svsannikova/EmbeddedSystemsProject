/*
===============================================================================
 Name    	: main.c
 Author  	: $(author)
 Version 	:
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string>
#include "string.h"
#include <cstdlib>
#include "ITM_write.h"
#include <cr_section_macros.h>
#include <XYPlotter.h>
#include "RIT.h"
#include "GCode.h"
#include "math.h"
#include "Pin.h"

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

int cmp_value;
int tempus;

int normSpeed;
int maxSpeed;

// SCT for the pen
#define SCT_PWM            LPC_SCT0 /* Use SCT0 for PWM */
#define SCT_PEN_PIN   		1       /* COUT0 [index 2] Controls LED */
#define SCT_PEN_OUT    		1       /* COUT0 [index 2] Controls LED */
#define SCT_PWM_RATE   	    50        /* PWM frequency 50 Hz */
#define SCT_PWM_RATE1   	1000        /* PWM frequency 50 Hz */
/* Systick timer tick rate, to change duty cycle */
#define TICKRATE_HZ     1000        /* 1 ms Tick rate */

#define SCT_PWM1            LPC_SCT1 /* Use SCT0 for PWM */
#define SCT_LASER_PIN   		1       /* COUT0 [index 2] Controls LED */
#define SCT_LASER_OUT    		1       /* COUT0 [index 2] Controls LED */
/* Systick timer tick rate, to change duty cycle */

//static void app_setup_pin(void)
//{
//	/* Enable SWM clock before altering SWM */
//	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
//
//#if defined(BOARD_NXP_LPCXPRESSO_1549)
//	/* Connect SCT output 1 to PIO0_29 */
//	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT1_O, 0, 10);
//#endif
//
//	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
//}

static void app_setup_pin(void)
{
	/* Enable SWM clock before altering SWM */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);

#if defined(BOARD_NXP_LPCXPRESSO_1549)
	/* Connect SCT output 1 to PIO0_29 */
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT1_O, 0, 10);
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT1_O, 0, 12);
#endif

	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
}

void SysTick_Handler(void)
{
	/* This is only used to wakeup up the device from sleep */
}


xSemaphoreHandle sbRIT;
QueueHandle_t xCommands;

bool accFlag = false;
int m4;

volatile uint32_t RIT_count;
int pps = 400;
bool direction;
int accSteps;

int counterMS;
double counterSS;
int stepsTotal;

float xDist,yDist;
bool xDirection,yDirection;

Pin xSw1 = Pin(0, 9);
Pin xSw2 = Pin(0, 29);
Pin xDir = Pin(1, 0);
Pin xMove = Pin(0, 24);

Pin ySw1 = Pin(0, 0);
Pin ySw2 = Pin(1, 3);
Pin yDir = Pin(0, 28);
Pin yMove = Pin(0, 27);

XYPlotter plotter = XYPlotter(xSw1, xSw2, xDir, xMove, ySw1, ySw2, yDir, yMove);

Gcode parser;

struct command
{
	int selected;
	float xcord;
	float ycord;
	int param;
} ncmd;

struct command *cmd;

void RIT_start(float xDis, bool xDire, float yDis, bool yDire, int us1){
	uint64_t cmp_value;

	normSpeed = us1;
	maxSpeed = 200;

	xDist = xDis;
	xDirection = xDire;

	yDist = yDis;
	yDirection = yDire;

	counterMS = 0;		// Master steps
	counterSS = 0.0;	// Slave steps
	accSteps = 0;

	if(xDist > yDist){
		// X is master
		stepsTotal = xDist;
	}else{
		// Y is master
		stepsTotal = yDist;
	}

	// Determine approximate compare value based on clock rate and passed interval
	tempus = normSpeed;
	cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) tempus / 1000000;

	// disable timer during configuration
	Chip_RIT_Disable(LPC_RITIMER);

	// enable automatic clear on when compare value==timer value
	// this makes interrupts trigger periodically
	Chip_RIT_EnableCompClear(LPC_RITIMER);
	// reset the counter
	Chip_RIT_SetCounter(LPC_RITIMER, 0);
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
	// start counting
	Chip_RIT_Enable(LPC_RITIMER);
	// Enable the interrupt signal in NVIC (the interrupt controller)
	NVIC_EnableIRQ(RITIMER_IRQn);

	// wait for ISR to tell that we're done
	if(xSemaphoreTake(sbRIT, portMAX_DELAY) == pdTRUE) {
		// Disable the interrupt signal in NVIC (the interrupt controller)
		NVIC_DisableIRQ(RITIMER_IRQn);
	}
	else
	{
		// unexpected error
	}
}

extern "C" {
void RIT_IRQHandler(void){
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	if(stepsTotal > 0){
		stepsTotal--;

		// X is master
		if(xDist > yDist){

			if(xDist > 100 && accFlag == false){
				if(stepsTotal > 0.6*xDist && tempus >= maxSpeed){
					if(stepsTotal % 10 == 0){
						tempus=tempus-8;
						cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) tempus / 1000000;
						Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
					}
					accSteps++;
				}
				if(stepsTotal < 0.5*xDist && stepsTotal <= accSteps && tempus <= normSpeed){
					if(stepsTotal % 10 == 0){
						tempus=tempus+8;
						cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) tempus / 1000000;
						Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
					}
				}
			}

			// Move X every time
			if(xDirection == true){
				if(!GPIO::readPin(xSw1)){
					GPIO::setPin(xMove, true);
					GPIO::setPin(xMove, false);
					counterMS++;
				}else{

				}
			}else{
				if(!GPIO::readPin(xSw2)){
					GPIO::setPin(xMove, true);
					GPIO::setPin(xMove, false);
					counterMS++;
				}else{

				}
			}

			// Move y only if..
			if((counterMS/(xDist/yDist) - counterSS) >= 1.0){
				if(yDirection == true){
					if(!GPIO::readPin(ySw1)){
						GPIO::setPin(yMove, true);
						GPIO::setPin(yMove, false);
						counterSS++;
					}else{

					}
				}else{
					if(!GPIO::readPin(ySw2)){
						GPIO::setPin(yMove, true);
						GPIO::setPin(yMove, false);
						counterSS++;
					}else{

					}
				}
			}

			// Y is master or equal steps
		}else{

			if(yDist > 100 && accFlag == false){
				if(stepsTotal > 0.6*xDist && tempus >= maxSpeed){
					if(stepsTotal % 10 == 0){
						tempus=tempus-8;
						cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) tempus / 1000000;
						Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
					}
					accSteps++;
				}
				if(stepsTotal < 0.5*xDist && stepsTotal <= accSteps && tempus <= normSpeed){
					if(stepsTotal % 10 == 0){
						tempus=tempus+8;
						cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) tempus / 1000000;
						Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
					}
				}
			}

			// Move Y every time
			if(yDirection == true){
				if(!GPIO::readPin(ySw1)){
					GPIO::setPin(yMove, true);
					GPIO::setPin(yMove, false);
					counterMS++;
				}else{

				}
			}else{
				if(!GPIO::readPin(ySw2)){
					GPIO::setPin(yMove, true);
					GPIO::setPin(yMove, false);
					counterMS++;
				}else{

				}
			}

			// Move X only if..
			if((counterMS/(yDist/xDist) - counterSS) >= 1.0){
				if(xDirection == true){
					if(!GPIO::readPin(xSw1)){
						GPIO::setPin(xMove, true);
						GPIO::setPin(xMove, false);
						counterSS++;
					}else{

					}
				}else{
					if(!GPIO::readPin(xSw2)){
						GPIO::setPin(xMove, true);
						GPIO::setPin(xMove, false);
						counterSS++;
					}else{

					}
				}
			}
		}
	}else{
		Chip_RIT_Disable(LPC_RITIMER);
		// disable timer
		// Give semaphore and set context switch flag if a higher priority task was woken up
		xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
	}

	// End the ISR and (possibly) do a context switch
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}
}

static void mainTask(void *pvParameters) {


	//printf("Ready \r\n");

	int selection;

	plotter.calibrate();


	while(1){

		selection = parser.readfromUart();

		cmd = & ncmd;
		cmd->selected = selection;

		if (selection == 1){
			cmd->xcord = parser.returng1().x;
			cmd->ycord = parser.returng1().y;
		}else if(selection == 4){
			cmd->param = parser.returnm1();
		}else if(selection == 5){
			cmd->param = parser.returnm4();
		}

		if( xQueueSend( xCommands,( void * ) cmd, portMAX_DELAY ) != pdPASS )
		{
			/* Failed to post the message, even after 10 ticks. */
		}

	}

}

static void readQueTask(void *pvParameters) {
	int penstuff;
	struct command rcmd;

	while(1){
		if( xQueueReceive( xCommands, &( rcmd ), portMAX_DELAY ) )
		{
			// pcRxedMessage now points to the struct AMessage variable posted
			// by vATask.

			switch(rcmd.selected){

			//we have a g1 command
			case 1:
				//read where we need to go

				if(rcmd.xcord < 0.0){
					rcmd.xcord = 0;
				}
				if(rcmd.ycord < 0.0){
					rcmd.ycord = 0;
				}

				plotter.move(rint(rcmd.xcord * (float)(plotter.xmaxPos / 310.0)), rint(rcmd.ycord * (float)(plotter.ymaxPos / 370.0)));
				parser.sendOk();
				break;
				//we have g28, return home
			case 2:

				plotter.move(0, 0);
				parser.sendOk();
				break;

				//we have m10, return the starting parameters
			case 3:
				parser.sendM10();
				parser.sendOk();
				break;

				//we have m1, pen up and down
			case 4:
				penstuff = rcmd.param;
				accFlag = false;

				if(penstuff == 90){
					//put pen down
					Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PEN_PIN, (Chip_SCTPWM_PercentageToTicks(SCT_PWM, 75)/10));
				}
				else if(penstuff == 160){
					//put the pen up
					Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PEN_PIN, (Chip_SCTPWM_PercentageToTicks(SCT_PWM, 50)/10));
				}
				parser.sendOk();
				break;

				//we have m4, we return ok from the gcode class we dont need to do anything else.
			case 5:

				m4 = rcmd.param;

				if(m4 == 0){
					accFlag = false;
				}else{
					accFlag = true;
				}

				Chip_SCTPWM_SetDutyCycle(SCT_PWM1, SCT_LASER_PIN, Chip_SCTPWM_PercentageToTicks(SCT_PWM1, int(m4 * 100/255)));

				parser.sendOk();
				break;
			}
		}
	}
}

int main(void) {

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
#endif
#endif

	sbRIT = xSemaphoreCreateBinary();
	xCommands = xQueueCreate( 50, sizeof( command ) );

	Chip_RIT_Init(LPC_RITIMER);
	NVIC_SetPriority(RITIMER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1 );

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, IOCON_DIGMODE_EN | IOCON_MODE_PULLUP);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 11, IOCON_DIGMODE_EN | IOCON_MODE_PULLUP);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 11);

	GPIO::initiate(xSw1, true);
	GPIO::initiate(xSw2, true);

	GPIO::initiate(xDir, false);
	GPIO::initiate(xMove, false);

	GPIO::initiate(ySw1, true);
	GPIO::initiate(ySw2, true);

	GPIO::initiate(yDir, false);
	GPIO::initiate(yMove, false);

	// SCT for pen
	Chip_SCTPWM_Init(SCT_PWM);
	Chip_SCTPWM_SetRate(SCT_PWM, SCT_PWM_RATE);

	Chip_SCTPWM_Init(SCT_PWM1);
	Chip_SCTPWM_SetRate(SCT_PWM1, SCT_PWM_RATE1);

	/* Setup Board specific output pin */
	app_setup_pin();

	/* Use SCT0_OUT1 pin */
	Chip_SCTPWM_SetOutPin(SCT_PWM, SCT_PEN_OUT, SCT_PEN_PIN);
	Chip_SCTPWM_SetOutPin(SCT_PWM1, SCT_LASER_OUT, SCT_LASER_PIN);

	/* Move pen up at first */
	Chip_SCTPWM_SetDutyCycle(SCT_PWM, SCT_PEN_PIN, (Chip_SCTPWM_PercentageToTicks(SCT_PWM, 50)/10));
	Chip_SCTPWM_SetDutyCycle(SCT_PWM1, SCT_LASER_PIN, Chip_SCTPWM_PercentageToTicks(SCT_PWM1, 0));

	Chip_SCTPWM_Start(SCT_PWM);
	Chip_SCTPWM_Start(SCT_PWM1);

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);
	// SCT for pen

	//yMotor = StepperMotor(ySw1, ySw2, yDir, yMove);

	//xMotor.calibrate();

	// Main task
	xTaskCreate(mainTask, "mainTask",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);


	xTaskCreate(readQueTask, "readQueue",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);


	/* Start the scheduler */
	vTaskStartScheduler();

	// Force the counter to be placed into memory
	volatile static int i = 0 ;
	// Enter an infinite loop, just incrementing a counter
	while(1) {
		i++ ;
	}
	return 0 ;
}

