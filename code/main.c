#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>
#include "taskManager.h"
#include "menu.h"
#include "externalInterrupt.h"
#include "segmentDisplay.h"
#include "keyboard.h"

#define IR_LED_0_PIN PD5
#define IR_LED_1_PIN PD6
#define IR_LED_PORT PORTD
#define IR_LED_DDR DDRD

#define OC2 PD7

#define GAP_TIME 10 //gap time between external interrupts
static uint32_t actualTime = GAP_TIME + 1;

static uint32_t startTimeTSOP = 0;
static uint32_t stopTimeTSOP = 0;

static bool freezeDisplayTime = false;

#define DEB0 PA0
#define DEB1 PA1
#define DEB2 PD4
#define DEB3 PD1

/**
 * Timer0 interrupt handler - for task manager. Called every 1ms.
 * */

ISR(TIMER0_COMP_vect){
	schedule();
}

/**
 * Setup Atmega32's Timer0 (8-bit) for interrupts every 1ms
 * */

void setupTimer0(){

	TCCR0 |= (1<<WGM01) | (0<<WGM00);	// set clock type as CTC
	OCR0 = 125;							// set Output Compare Register - together with prescaler this will give us
	// interrupt every 1ms
	TIMSK |= (1<<OCIE0);				// set interrupts co compare
	TCCR0 |= (1<<CS00) | (1<<CS01 );	// set clock prescaler 64. Frequency 1kHz

}

/**
 * Setup Atmega32's Timer2 (8-bit) for generating quadrature signal with frequency 38kHz on OC2 pin.
 * */

void setupTimer2(){
    
    TCCR2 |= (1<<WGM21) | (0<<WGM20); //set clock type as CTC
    TCCR2 |= (0<<COM21) | (1<<COM20); //set toggle OC2 on compare match
    TCCR2 |= (0<<CS22) | (0<<CS21) | (1<<CS20); //set clock prescaler at 1

    OCR2 = 106; //value to compare, value on OC2 should be toggled with frequenct 19kHz
    //no interrupts needed

    DDRD |= _BV(PD7);//set OC2 pin as output

}

/**
 * This function is called in task manager every 4ms which changes actual displaying segment display (multiplexing) from set of 4 segment displays.
 * */

void changeDisplayTask(void* args){
    changeDisplay();
}

/**
 * This function is called in task manager every 1ms which increments actual working controller time, and if needed calls library from segment display to change displayung value.
 * */

void incrementTimeTask(void* args){
    actualTime++;
    if (!freezeDisplayTime) setValueToDisplay((uint32_t)(actualTime - startTimeTSOP), 2); //if object is between first and second gate
}

/**
 * This function is called in task manager verey 40ms which checks if it's pushed any button and calling function from menu library.
 * */

void checkButtonTask(void* args){
    if (!isPressed()) return;
    pushedButtonEvent(read());
}

bool measurementToDisplay = false;

/**
 * This function is called every 20ms in task manager, checks if it's measured any time between cuting IR gates (marked in bool variable measurementToDisplay in external interrupts occuring when IR barier is cuted).
 * */

void TSOPCheckTask(void* args){
    if (!measurementToDisplay) return;
    refreshSpeed(stopTimeTSOP - startTimeTSOP);
    measurementToDisplay = false;
}

void TSOP0WaitForReady();
void TSOP1WaitForReady();
void TSOP0interrupt();
void TSOP1interrupt();

/**
 * This function is registerd in the begining or by the TSOP0interrupt function when interrupt occours, to call when external interrupt 0 occours on low level edge. It means that TSOP registered minimum amount of packages from IR LED and then function registers handler function and changes state of micocontroller, which should wait for raising edge on INT0 pin. and then registered function will be invoked.
 * */

void TSOP0WaitForReady(){
    PORTA ^= (_BV(DEB0));
    externalInt0funRegister(TSOP0interrupt);
    setupRisingEdgeINT0();
}

/**
 * This function is registerd in the begining or by the TSOP1interrupt function when interrupt occours, to call when external interrupt 1 occours on low level edge. It means that TSOP registered minimum amount of packages from IR LED and then function registers handler function and changes state of micocontroller, which should wait for raising edge on INT1 pin and then registered function will be invoked.
 * */

void TSOP1WaitForReady(){
    PORTD ^= (_BV(DEB2));
    externalInt1funRegister(TSOP1interrupt);
    setupRisingEdgeINT1();
}

/**
 * This function is registered to call when external interrupt 0 occours. It means the ir barier is cut. Starts measuring time between cuting IR bariers - sets startTimeTSOP variable to actual time. Turns off IR LED 0 and turn on IR LED 1, switch off external interrupt on INT0 and switch on on INT1, changes function handling interrupt to TSOP0WaitForReady and sets type of interrupt to low level edge.
 * */

void TSOP0interrupt(){
    PORTA ^= (_BV(DEB1));       //DEBUG

    IR_LED_PORT &= ~(_BV(IR_LED_0_PIN));    //switch off IR LED 0
    IR_LED_PORT |= _BV(IR_LED_1_PIN);       //switch on IR LED 1

    freezeDisplayTime = false;

    startTimeTSOP = actualTime;
    stopTimeTSOP = 0;

    setupLowLevelINT0();
    externalInt0funRegister(TSOP0WaitForReady);

    externalINT0switchOff();
    externalINT1switchOn();
}

/**
 * This function is registered to call when external interrupt 1 occours. It means the ir barier is cut. Stops measuring time between cuting IR bariers - sets stopTimeTSOP variable to actual time. Turns off IR LED 1 and turn on IR LED 0, switch off external interrupt on INT1 and switch on on INT0, changes function handling interrupt to TSOP1WaitForReady and sets type of interrupt to low level edge.
 * */

void TSOP1interrupt(){
    PORTD ^= (_BV(DEB3));       //DEBUG
    IR_LED_PORT &= ~(_BV(IR_LED_1_PIN));
    IR_LED_PORT |= _BV(IR_LED_0_PIN);

    freezeDisplayTime = true;

    stopTimeTSOP = actualTime;
    measurementToDisplay = true;

    setupLowLevelINT1();
    externalInt1funRegister(TSOP1WaitForReady);

    externalINT1switchOff();
    externalINT0switchOn();
}

/**
 * Turns on IR LED on first gate and turns off IR LED on second gate. Set ups also both pins for output.
 * */

void IrLEDinit(){
    IR_LED_DDR |= (_BV(IR_LED_0_PIN) | _BV(IR_LED_1_PIN));
    IR_LED_PORT |= (_BV(IR_LED_0_PIN));
    IR_LED_PORT &= ~(_BV(IR_LED_1_PIN));
}

/**
 * Main function.
 * */

int main(void)
{

    DDRD |= _BV(DEB2);   //DEBUG
    DDRD |= _BV(DEB3);   //DEBUG
    DDRA |= _BV(DEB0);   //DEBUG
    DDRA |= _BV(DEB1);   //DEBUG
    PORTD |= _BV(DEB2);   //DEBUG
    PORTD |= _BV(DEB3);   //DEBUG
    PORTA |= _BV(DEB0);   //DEBUG
    PORTA |= _BV(DEB1);   //DEBUG

    PORTA &= ~(_BV(DEB0));  //DEBUG
    _delay_ms(500);        //DEBUG
    PORTA &= ~(_BV(DEB1));  //DEBUG
    _delay_ms(500);        //DEBUG
	
	setupTimer0();
	setupTimer2();

	IrLEDinit();

	segmentDisplayInit();

	externalIntInit(&actualTime, GAP_TIME, true, false);
    externalInt0funRegister(TSOP0WaitForReady);
    externalInt1funRegister(TSOP1WaitForReady);

	LCD_Initalize();
    LCD_WriteText("Photocell v0.001");
	
	keyboardInit();

	addTask(0, 4, changeDisplayTask, NULL);
	addTask(1, 10, incrementTimeTask, NULL);
	addTask(2, 40, checkButtonTask, NULL);
	addTask(3, 20, TSOPCheckTask, NULL);
	
	setValueToDisplay(0, 3);    //DEBUG //dlaczego to jest w ogole potrzebne?!?!??!!?

    _delay_ms(1000);
	LCD_Clear();
	displayMenu(0,0);
	

    PORTD &= ~(_BV(DEB2));  //DEBUG
    _delay_ms(500);        //DEBUG
    PORTD &= ~(_BV(DEB3));  //DEBUG
    _delay_ms(500);        //DEBUG
    PORTD |= _BV(DEB2);   //DEBUG
    PORTD |= _BV(DEB3);   //DEBUG
    PORTA |= _BV(DEB0);   //DEBUG
    PORTA |= _BV(DEB1);   //DEBUG

	sei();								// turn interrupts on
    
	execute();

}
