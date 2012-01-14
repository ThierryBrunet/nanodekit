/* NanodeWatchDogTest = Watchdog Test code for Nanode 
------------------------------------------------------
V1.0
MercinatLabs / MERCINAT SARL France
By: Thierry Brunet de Courssou
http://www.mercinat.com
Created 14 Jan  2012
Last update: 14 Jan 2012
Project hosted at: http://code.google.com/p/nanodebuildtest/ - Repository type: Subversion
Version Control System: TortoiseSVN 1.7.3, Subversion 1.7.3, for Window7 64-bit - http://tortoisesvn.net/downloads.html

Changes
-------
V1.0 - Original release

Configuration
-------------
Hardware: Nanode v5
Software:
- Arduino 1.0 IDE for Windows at http://files.arduino.cc/downloads/arduino-1.0-windows.zip
- Arduino for Visual Studio at http://www.visualmicro.com/

Project Summary
---------------
- test watchdog

Comments
--------
- Adding a watchdog timeout is fundamental in achieving reliable Pachube updates (no more feed hang))
As the watchdog issue is not obvious, this code is usefull in making sure everything is
OK including the Bootloader.
- Watchdog timeout has been rolled into NanodeKit to unsure there is no conflict with Pachube/Ethercard code

*/

#include <avr/interrupt.h>
#include <avr/wdt.h>

int TimeOutLoopsBeforeReboot = 5;  // will reboot after getting X times into the timeout loop

//initialize watchdog
void WatchdogSetup(void)
{
	//disable interrupts
	cli();
	//reset watchdog
	wdt_reset();
	

	// WDTCSR = WatchDog Timer Control Register
	WDTCSR = (1<<WDCE)|(1<<WDE);
	
	//Start watchdog timer with 4s prescaller
	// WDTCSR = (1<<WDIE)|(1<<WDE)|(1<<WDP3)| (0<<WDP2) | (0<<WDP1) | (0<<WDP0);
	
	//Start watchdog timer with 8s prescaller
	WDTCSR = (1<<WDIE)|(1<<WDE)|(1<<WDP3) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
	
	//Enable global interrupts
	sei();
}

//Watchdog timeout ISR
ISR(WDT_vect)
{
	//Burst 0.1Hz pulses
	for (int i=0; i < 200; i++) 
	{ 
		digitalWrite(6, LOW);
		delay (250);
		digitalWrite(6, HIGH);
		delay (250);
	} 
	
	Serial.println("timeout after x seconds");
	
	WatchdogSetup(); // This is only for this TEST code. If not there, LOOP will hang
	
	// Reboot when TimeOut function done several times
	if ( TimeOutLoopsBeforeReboot-- <= 0)  
	{
		// (B) This is only to insert for your FULL code
		// ----------------------------------------------
		Serial.print("\nREBOOTING....\n\n");
		delay(1000); // leave some time for printing on serial port to complete
		software_Reset();
	}
	
}

void setup()
{
	Serial.begin(115200);
	Serial.println("\nNanodeWatchDogTest V1.0 - MercinatLabs (14 Jan 2012)\r\n");
	pinMode(6, OUTPUT);	// initialize LED on pin 6
	
	//initialize watchdog
	WatchdogSetup();
}


void loop ()
{
	while(1)
	{
		//LED ON
		digitalWrite(6, LOW);
		delay (150);
		//LED OFF
		digitalWrite(6, HIGH);
		//~0.5s delay
		delay(850);
		Serial.print(".");
		
		
		// put this line below when you want to check what happens when there is no timeout
		// wdt_reset();  
	}
}


void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
	asm volatile ("  jmp 0");  
} 
