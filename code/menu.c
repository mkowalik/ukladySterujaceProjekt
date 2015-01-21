/*
 * 
 * 
 * menu.c
 * 
 */

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "HD44780.h"
#include "menu.h"
 
uint16_t dist_value = 1000; //1000/100 = 10cm
uint16_t velocity_value = 100;
uint8_t tempButtonVal = 0;
uint8_t actual_unit = kph_button;
static uint8_t rightarrow = 0b01111110;
static uint8_t leftarrow = 0b01111111;
uint8_t distChange = 0;


uint16_t toMph(uint16_t kph_val){
	return 62 * kph_val / 100;
}

void text1(uint16_t value, uint8_t unit, char * text){
	if(unit == kph_button){
		sprintf(text, "%u.%u %u kph   mph", value/100, value%100, rightarrow);
	}
	else{
		value = toMph(value);
		sprintf(text, "%u.%u  kph %u mph", value/100, value%100, rightarrow);
	}		
}
<<<<<<< HEAD
void text2(uint16_t value, char * text){
	if(value%100<10) sprintf(text, "distance: %u %u.0%u %u", leftarrow, value/100, value%100, rightarrow);
	else sprintf(text, "distance: %u %u.%u %u", leftarrow, value/100, value%100, rightarrow);
	
=======

void text2(uint16_t value, char * text){
	if(value%100<10) sprintf(text, "distance: %u %u.0%u %u", leftarrow, value/100, value%100, rightarrow);
	else sprintf(text, "distance: %u %u.%u %u", leftarrow, value/100, value%100, rightarrow);
>>>>>>> 9d5a5a97b7d38c722997027fe37dbe58ed0abfb7
}

void displayMenu(){
	char text_1[40];
	char text_2[40];
<<<<<<< HEAD
	
=======
	LCD_GoTo('0','0');
>>>>>>> 9d5a5a97b7d38c722997027fe37dbe58ed0abfb7
	//reading velocity_value velocity_value = read();
	tempButtonVal = getButtonValue(NULL);
	if(tempButtonVal == kph_button || tempButtonVal == mph_button) actual_unit = tempButtonVal;
	text1(velocity_value, actual_unit, text_1)
	
	if(tempButtonVal == up_dist || tempButtonVal == down_dist) distChange = tempButtonVal;
	if(distChange == up_dist){
		dist_value += 1;
		distChange = 0;
	}
	else if(distChange == down_dist){
		dist_value -= 1;
		distChange = 0;
	}
	text2(dist_value,text_2);
	
	LCD_GoTo('0','0');
	LCD_WriteText(text_1);
	LCD_GoTo('0','1');
	LCD_WriteText(text_2);

}
