#include <platform.h>
#include <gpio.h>
#include <timer.h>
#include "delay.h"
#include "lcd.h"
#include "switches.h"
#include "leds.h"
#include "project03Lib.h"

// ISR counters
volatile int getTemp_counter = 0;		// A counter that count seconds
volatile int clearLCD_counter = 0;		// A counter that count seconds
volatile int getDist_counter = 0;		// A counter that count seconds

// ISR routine
void myTimer_isr(void) {
	getTemp_counter = getTemp_counter + 1;
	clearLCD_counter = clearLCD_counter + 1;
	getDist_counter = getDist_counter + 1;
}

// Main START
int main(void) {
	// DEFINE PROPERTIES
	int tempTime = 5;					// Get temperature measurement every tempTime seconds
	int measuNum = 24;					// Number of temp measurements
	int clearTime = 10;					// LCD is open for clearTime seconds
	int distTime = 1;					// Get distance mesurement every distTime seconds
	
	float highTemp = 27;				// If temperature >= highTemp Red_LED ON
	float lowTemp = 23;					// If temperature <= lowTemp Blue_LED ON
	float maxTemp = 45;					// If temperature >= maxTemp Green_LED ON
	
	double maxDist = 8;					// If Distance <= maxDist LCD_ON
	
	// Timer Init
	timer_init(1000000); 				// Set timer to 1sec
	timer_set_callback(myTimer_isr);	// Set the callback funtion to the timer
	timer_enable();						// Enable timer
	__enable_irq();						// Enable irq
	
	// LEDs init
	leds_init();
	
	// LCD Init
	#define LCD_BackLight_Pin PB_0		// Set LCD Backlight Pin
	gpio_set_mode(LCD_BackLight_Pin, Output);
	lcd_init();
	gpio_set(LCD_BackLight_Pin, 1);
	lcd_clear();						// Clear LCD
	lcd_set_cursor(0,0);				// Set cursor
	lcd_print("MyThermometer :)");		// Print  welcome message 
	lcd_set_cursor(0,1);
	lcd_print("   8804  8805   ");
	
	// Distance Sensor - HCSR04 init
	HCSR04_init();
	
	// Deifine variables we will use
	volatile float tempArray[measuNum];	// Array with last 24 sensor's result
	volatile int iTemp = 0;				// Index of the tempArray
	float sumTemp = 0;					// Sum of the tempArray
	float avgTemp = 0;					// Average of the tempArray
	
	uint8_t presence = 0;				// DHT11_check_response result
	volatile float Temperature = 0;		// DHT11 sensor's temperature result
	volatile float Humidity = 0;		// DHT11 sensor's humidity result
	uint16_t SUM, Temp, Rh;				// DHT11_read results
	uint8_t Temp_byte1, Temp_byte2;		// DHT11_read results
	uint8_t Rh_byte1, Rh_byte2;			// DHT11_read results
	
	volatile double Distance = 100;		// HCSR04 sensor's distance result
	
	char str1[20] = {0};				// str will be printed in LCD fisrt line
	char str2[20] = {0};				// str will be printed in LCD second line
	
	// Define indexies
	int jTemp;
	
	// Define flags
	int flag_LEDs = 1;

	// While START
	while (1) {
		// Take temp measurement every 5 seconds
		if (getTemp_counter == tempTime) {
			// Get Temperature
			// DHT11 Code
			__disable_irq();
			DHT11_start();
			presence = DHT11_check_response();
			Rh_byte1 = DHT11_read();
			Rh_byte2 = DHT11_read();
			Temp_byte1 = DHT11_read();
			Temp_byte2 = DHT11_read();
			SUM = DHT11_read();
			Temp = Temp_byte2<<8|Temp_byte1;
			Rh = Rh_byte1;
			Temperature = (float) Temp;
			Humidity = (float) Rh;
			__enable_irq();
			// Save temperature to tempArray
			
			/*
			// DS1820 Code
			presence = DS1820_start();
			delay_us(1);
			DS1820_write(0xCC);
			DS1820_write(0x44);
			delay_us(800);
			presence = DS1820_start();
			DS1820_write(0xCC);
			DS1820_write(0xBE);
			Temp_byte1 = DS1820_read();
			Temp_byte2 = DS1820_read();
			Temperature = (float) ((Temp_byte2<<8)|Temp_byte1) / 16;
			*/
			
			tempArray[iTemp] = Temperature;
			
			// Set getTemp_counter to zero !!!!!!!!!!!!!!!!!! maybe in the end
			getTemp_counter = 0;
			
			// Check for highTemp, maxTemp, lowTemp
			if (tempArray[iTemp] >= highTemp){
				// If temperature >= highTemp Red_LED ON
				leds_set(1, 0, 0);
				sprintf(str1, "HighTemp Reached");
				if (tempArray[iTemp] >= maxTemp) {
					// If temperature >= maxTemp Green_LED ON
					leds_set(1, 1, 0);
					sprintf(str1, "Max Temp Reached");
				} else {
					leds_set(1, 0, 0);
				}
			} else if (tempArray[iTemp] <= lowTemp) {
				leds_set(0, 0, 1);
				sprintf(str1, "Low Temp Reached");
			} else {
				leds_set(0, 0, 0);
				flag_LEDs = 0;
			}
			
			// Display highTemp, maxTemp, lowTemp if needed
			if (flag_LEDs == 1) {
				sprintf(str2, "    %.2f  C    ", tempArray[iTemp]);
				// Set clearLCD_counter to zero 
				clearLCD_counter = 0;
				// Print to LCD
				lcd_clear();
				gpio_set(LCD_BackLight_Pin, 1);
				lcd_set_cursor(0,0);
				lcd_print(str1);
				lcd_set_cursor(0,1);
				lcd_print(str2);			
			} else {
				flag_LEDs = 1;
			}
			
			// Incrise iTemp
			iTemp = iTemp + 1;
		}
		
		// Take average temperature every 24 mesurements
		if (iTemp == measuNum) {
			sumTemp = 0;
			for (jTemp = 0; jTemp < iTemp; jTemp++) {
				sumTemp = sumTemp + tempArray[jTemp];
			}
			avgTemp = sumTemp / measuNum;
			sprintf(str1, "MyThermometer :)");
			sprintf(str2, "avgTemp: %.2f C", avgTemp);
			// Set iTemp to zero
			iTemp = 0;
			// Set clearLCD_counter to zero
			clearLCD_counter = 0;
			// Print to LCD
			lcd_clear();
			gpio_set(LCD_BackLight_Pin, 1);
			lcd_set_cursor(0,0);
			lcd_print(str1);
			lcd_set_cursor(0,1);
			lcd_print(str2);		
		}
		
		// Every time check the distance
		if (getDist_counter == distTime) {
			__disable_irq();
			Distance = HCSR04_read(); // distance in cm
			__enable_irq();
			// Set getDist_counter to zero
			getDist_counter = 0;
			if (Distance <= maxDist) {
				sprintf(str1, "AvgTemp: %.2f C", avgTemp);
				if (iTemp == 0) {
					sprintf(str2, "Temp: %.2f C", tempArray[23]);
				} else {
					sprintf(str2, "Temp: %.2f", tempArray[iTemp-1]);
				}
				// Set clearLCD_counter to zero 
				clearLCD_counter = 0;
				// Print to LCD
				lcd_clear();
				gpio_set(LCD_BackLight_Pin, 1);
				lcd_set_cursor(0,0);
				lcd_print(str1);
				lcd_set_cursor(0,1);
				lcd_print(str2);
			}
		}
		
		// Close LCD 10 seconds after it opens
		if (clearLCD_counter == clearTime) {
			gpio_set(LCD_BackLight_Pin, 0);
			lcd_clear();
		}
		
	} // While END
} // Main END
