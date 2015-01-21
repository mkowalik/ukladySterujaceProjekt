#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "segmentDisplay.h"

static uint8_t DIGIT[10] = {0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};
static uint8_t DOT = 0b01111111;

static void displayDigit(uint8_t displayNr, bool dot, uint8_t value){
    if (displayNr > 3) return;

    value %= 10;

	DISPLAY_GRD_PORT |= 0x0f;
	DISPLAY_PORT = DIGIT[value];
	if (dot) DISPLAY_PORT |= DOT;

	DISPLAY_GRD_PORT &= (0xF0 | (~(1<<displayNr)));
	
}

static uint32_t currentValue = 0;
static uint8_t currentDigitsAfterDot = 0;

static uint8_t currentDisplay = 0;

void changeDisplay(){
    currentDisplay++;

    uint8_t dot = 0;
    if (currentDisplay>0 && currentDisplay==currentDigitsAfterDot) dot = 1;

	if (currentDisplay==0) displayDigit(currentDisplay, dot, currentValue/10)
	else if (currentDisplay==1) displayDigit(currentDisplay, dot, currentValue/100);
	else if (currentDisplay==2) displayDigit(currentDisplay, dot, currentValue/1000);
	else if (currentDisplay==3) displayDigit(currentDisplay, dot, currentValue/10000);

}

void setValueToDisplay(uint32_t value, uint8_t digitsAfterDot){
    currentValue = value;
    currentDigitsAfterDot = digitsAfterDot;
}

