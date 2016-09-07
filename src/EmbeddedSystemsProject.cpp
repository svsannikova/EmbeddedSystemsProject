/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include <cr_section_macros.h>
//new comment
// TODO: insert other include files here

static void gCodeParser(void *pvParameters) {
	int character;
	int clause[61];

	int i = 0;

	while (1){

		// Read character from debug serial port
		character = Board_UARTGetChar();

		// ASCII CODE TABLE

		// code char
		//	0   NUL	16	DLE	32	SP	48	0	64	@	80	P	96	`	112	p
		//	1	SOH	17	DC1	33	!	49	1	65	A	81	Q	97	a	113	q
		//	2	STX	18	DC2	34	"	50	2	66	B	82	R	98	b	114	r
		//	3	ETX	19	DC3	35	#	51	3	67	C	83	S	99	c	115	s
		//	4	EOT	20	DC4	36	$	52	4	68	D	84	T	100	d	116	t
		//	5	ENQ	21	NAK	37	%	53	5	69	E	85	U	101	e	117	u
		//	6	ACK	22	SYN	38	&	54	6	70	F	86	V	102	f	118	v
		//	7	BEL	23	ETB	39	'	55	7	71	G	87	W	103	g	119	w
		//	8	BS	24	CAN	40	(	56	8	72	H	88	X	104	h	120	x
		//	9	HT	25	EM	41	)	57	9	73	I	89	Y	105	i	121	y
		//	10	LF	26	SUB	42	*	58	:	74	J	90	Z	106	j	122	z
		//	11	VT	27	ESC	43	+	59	;	75	K	91	[	107	k	123	{
		//	12	FF	28	FS	44	,	60	<	76	L	92	\	108	l	124	|
		//	13	CR	29	GS	45	-	61	=	77	M	93	]	109	m	125	}
		//	14	SO	30	RS	46	.	62	>	78	N	94	^	110	n	126	~
		//	15	SI	31	US	47	/	63	?	79	O	95	_	111	o	127	DEL


		// If there is character
		if (character != -1){
			if (character != 10 && character != 13){
				// Print character to UART
				clause[i] = character;
				i++;
				//Board_UARTPutChar (character);
			}else{
				i=0;

				// M10
				if (clause[0] == 77 && clause[1] == 49 && clause[2] == 48){
					DEBUGOUT("OK");
					vTaskDelay(configTICK_RATE_HZ);
				}

				// M4
				else if (clause[0] == 77 && clause[1] == 52){
					Board_UARTPutSTR("\r\nOK\r\n");
				}

				// G1
				else if (clause[0] == 71 && clause[1] == 49){
					Board_UARTPutSTR("\r\nOK\r\n");
				}

				// G28
				else if (clause[0] == 50 && clause[1] == 50 && clause[2] == 56){
					Board_UARTPutSTR("\r\nOK\r\n");
				}

				else{
					Board_UARTPutSTR("\r\nNOT OK\r\n");
				}

				// Empty the clause
				memset (clause, 0, sizeof clause);
			}
		}
	}
}

static bool doStep(){

}

/* UART (or output) thread */
static void vUARTTask(void *pvParameters) {
	int tickCnt = 0;

	while (1) {
		tickCnt++;
		if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
			Board_LED_Set(0, true);
		}else{
			Board_LED_Set(0, false);
		}

		//DEBUGOUT("Tick: %d \r\n", tickCnt);

		/* About a 1s delay here */
		vTaskDelay(configTICK_RATE_HZ/6);
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

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);

	//	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_DIGMODE_EN);
	//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);
	//
	//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, True);

	/* UART output thread, simply counts seconds */
	xTaskCreate(gCodeParser, "gCodeParser",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(vUARTTask, "vUARTTask",
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
