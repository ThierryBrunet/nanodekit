/* NanodeKit = Test code for Nanode v5 kit after assembly and test feed to Pachube
----------------------------------------------------------------------------------
V1.1
MercinatLabs / MERCINAT SARL France
By: Thierry Brunet de Courssou
http://www.mercinat.com
Created 12 Oct  2011
Last update: 14 Jan 2012
Project hosted at: http://code.google.com/p/nanodebuildtest/ - Repository type: Subversion
Version Control System: TortoiseSVN 1.7.3, Subversion 1.7.3, for Window7 64-bit - http://tortoisesvn.net/downloads.html

Changes
-------
V1.0 - Original release
V1.1 - Reduced SRAM usage with showString, added watchdog timer, added SRAM memory check, randomize network start
	over 2 seconds to avoid all Nanodes to collide on network access - HW: add 10uF and 10nF power supply decoupling
	caps on Nanode board to protect against hardware latch-up
V1.2 - get NTP atomic time, check ATmega328 1024 bytes EEPROM, check Microchip 11AA02E48 2Kbit serial EEPROM (MAC chip)


Configuration
-------------
Hardware: Nanode v5
Software:
- Arduino 1.0 IDE for Windows at http://files.arduino.cc/downloads/arduino-1.0-windows.zip
- Arduino for Visual Studio at http://www.visualmicro.com/

Project Summary
---------------
- test MAC address
- DHCP & DNS
- test Ethernet communication to Pachube by generating a simple ramp
- Pachube results may be viewed at https://pachube.com/feeds/37267

Comments
--------
- Pachube key given in this code uses MercinatLabs free room test account
you may use this key for your initial testing

- the ramps allow to verify the stability and regularity of the feed over a long period of time

// RAM space is very limited so use PROGMEM and showString to conserve memory with strings

// As Pachube feeds may hang at times, we reboot regularly. 
// We will monitor stability then remove reboot when OK.

// The last datastream ID is a health indicator. This is simply an incremeting number so any
// interruption can be easily addentified as a discontinuity on the ramp graph shown on Pachube

// Mercinat Etel test site Nanodes -- As we use one Nanode per sensor, making use of the MAC allows to assign IP by DHCP and identify each sensor
// -------------------------------
//Nanode: 1	Serial: 266	 Mac: 00:04:A3:2C:2B:D6 --> Aurora
//Nanode: 2	Serial: 267	 Mac: 00:04:A3:2C:30:C2 --> FemtoGrid
//Nanode: 3	Serial: 738	 Mac: 00:04:A3:2C:1D:EA --> Skystream
//Nanode: 4	Serial: 739	 Mac: 00:04:A3:2C:1C:AC --> Grid RMS #1
//Nanode: 5	Serial: 740	 Mac: 00:04:A3:2C:10:8E --> Grid RMS #2
//Nanode: 6	Serial: 835	 Mac: 00:04:A3:2C:28:FA -->  6 m anemometer/vane
//Nanode: 7	Serial: 836	 Mac: 00:04:A3:2C:26:AF --> 18 m anemometer/vane
//Nanode: 8	Serial: 837	 Mac: 00:04:A3:2C:13:F4 --> 12 m anemometer/vane
//Nanode: 9	Serial: 838	 Mac: 00:04:A3:2C:2F:C4 -->  9 m anemometer/vane

// Pachube feeds assignement
// -------------------------
//  37267 -- newly built Nanode test feed (Mercinat) -- https://pachube.com/feeds/37267
//  40451 -- Free room for anyone to test the NanodeKit code  -- https://pachube.com/feeds/40451
//  40447 -- Free room for anyone to test the ArduWind code   -- https://pachube.com/feeds/40447
//  40448 -- Free room for anyone to test the ArduGrid code   -- https://pachube.com/feeds/40448
//  40450 -- Free room for anyone to test the SkyChube code   -- https://pachube.com/feeds/40450
//  40449 -- Free room for anyone to test the ArduSky code    -- https://pachube.com/feeds/40449

========================================================================================================*/
//#if ARDUINO >= 100
//  #include <Arduino.h> // Arduino 1.0
//  #define WRITE_RESULT size_t
//  #define WRITE_RETURN return 1;
//#else
//  #include <WProgram.h> // Arduino 0022+
//  #define WRITE_RESULT void
//  #define WRITE_RETURN
//#endif

// Watch dog timer
#include <avr/wdt.h> // Watchdog timer
// Watchdog further info at:
// -------------------------
// http://icanbuild.it/hacks/arduino-watchdog-with-optiboot-and-avrisp-mk2/
// http://tushev.org/articles/electronics/48-arduino-and-watchdog-timer
// http://www.embedds.com/using-watchdog-timer-in-your-projects/



/* PROGMEM, pgmspace.h library and showString() function
Store data in flash (program) memory instead of SRAM. There's a description of the various types of 
memory available on an Arduino board. The PROGMEM keyword is a variable modifier, it should be used 
only with the datatypes defined in pgmspace.h. It tells the compiler "put this information into flash 
memory", instead of into SRAM, where it would normally go. PROGMEM is part of the pgmspace.h library.
So you first need to include the library at the top your sketch.
- showStrings allows to display the string stored in flash.
*/
#include <avr/pgmspace.h>

/* EEPROM
This library enables you to read and write bytes from/to EEPROM.
EEPROM size: 1024 bytes on the ATmega328
An EEPROM write takes 3.3 ms to complete. The EEPROM memory has a specified life of 100,000 write/erase 
cycles, so you may need to be careful about how often you write to it.

	value = EEPROM.read(a);  // [a] int - address starting from zero,  [value] byte - returned value
	EEPROM.write(a, ivalue);

*/
//#include <EEPROM.h>

#include <EtherCard.h>  // get latest version from https://github.com/jcw/ethercard
// EtherShield uses the enc28j60 IC (not the WIZnet W5100 which requires a different library

#include <NanodeUNIO.h>   // get latest version from https://github.com/sde1000/NanodeUNIO 
// All Nanodes have a Microchip 11AA02E48 serial EEPROM chip
// soldered to the underneath of the board (it's the three-pin
// surface-mount device).  This chip contains a unique ethernet address
// ("MAC address") for the Nanode.
// To read the MAC address the library NanodeUNIO is needed

// If the above library has not yet been updated for Arduino1, 
// in the 2 files NanodeUNIO.h and NanoUNIO.cpp
// you will have to make the following modification:
//#if ARDUINO >= 100
//  #include <Arduino.h> // Arduino 1.0
//#else
//  #include <WProgram.h> // Arduino 0022+
//#endif

byte macaddr[6];  // Buffer used by NanodeUNIO library
NanodeUNIO unio(NANODE_MAC_DEVICE);
boolean bMac; // Success or Failure upon function return

int MyNanode = 0; 

#define APIKEY  "fqJn9Y0oPQu3rJb46l_Le5GYxJQ1SSLo1ByeEG-eccE"  // MercinatLabs FreeRoom Pachube key for anyone to test this code

//#define REQUEST_RATE 6000 // in milliseconds - Pachube update rate
#define REQUEST_RATE 10000 // in milliseconds - Pachube update rate
unsigned long lastupdate;  // timer value when last Pachube update was done
unsigned long pingtimer;   // ping timer
uint32_t timer;  // a local timer

byte Ethernet::buffer[700];
Stash stash;     // For filling send buffer using satndard "print" instructions

// Initialise values for 4 ramps, each assigned to a different datastream
float Ramp0 = 0;
float Ramp1 = 50;
float Ramp2 = 500;
float Ramp3 = 5000;

// Misc.
// -----
const unsigned long OneMinuteMillis = 1000 * 60;
const unsigned long OneHourMillis   = OneHourMillis * 60;
const unsigned long OneDayMillis    = OneHourMillis * 24;  

unsigned long TimeStampSinceLastReboot;

// Watchdog timer variables
// ------------------------
unsigned long previousWdtMillis = 0;
unsigned long wdtInterval = 0;


void setup()
{
	delay( random(0,2000) ); // delay startup to avoid all Nanodes to collide on network access after a general power-up
								// and during Pachube updates
									
	WatchdogSetup;// setup Watch Dog Timer to 8 sec
	TimeStampSinceLastReboot = millis();
	pinMode(6, OUTPUT);
	Serial.begin(115200);


	showString(PSTR("\nNanodeKit V1.1 - MercinatLabs (10 Dec 2012)\r\n"));
	showString(PSTR("[getDHCPandDNS] [Pachube webClient]\r\n"));
	showString(PSTR("[memCheck bytes] = ")); Serial.println(freeRam());

	GetMac(); // Get and print MAC (via NanodeUNIO)

	// Identify which sensor is assigned to ths board
	// If you have boards with identical MAC last 2 values, you will have to adjust you code accordingly
	switch ( macaddr[5] )
	{
	case 0xFA: MyNanode = 6;  showString(PSTR("n6 ")); showString(PSTR("f38277 - ")); showString(PSTR("Etel 6 m")) ; break; 
	case 0xC4: MyNanode = 9;  showString(PSTR("n9 ")); showString(PSTR("f38278 - ")); showString(PSTR("Etel 9 m")) ; break;
	case 0xF4: MyNanode = 8;  showString(PSTR("n8 ")); showString(PSTR("f38279 - ")); showString(PSTR("Etel 12 m")); break;
	case 0xAF: MyNanode = 7;  showString(PSTR("n7 ")); showString(PSTR("f38281 - ")); showString(PSTR("Etel 18 m")); break;
	case 0xD6: MyNanode = 1;  showString(PSTR("n1 ")); showString(PSTR("f37667 - ")); showString(PSTR("Aurora"))   ; break;
	case 0xC2: MyNanode = 2;  showString(PSTR("n2 ")); showString(PSTR("f37668 - ")); showString(PSTR("FemtoGrid")); break;
	case 0xEA: MyNanode = 3;  showString(PSTR("n3 ")); showString(PSTR("f35020 - ")); showString(PSTR("Skystream")); break;
	case 0xAC: MyNanode = 4;  showString(PSTR("n4 ")); showString(PSTR("f40385 - ")); showString(PSTR("Grid RMS #1")); break;
	case 0x8E: MyNanode = 5;  showString(PSTR("n5 ")); showString(PSTR("f40386 - ")); showString(PSTR("Grid RMS #2")); break;
	default:  
		showString(PSTR("unknown Nanode\n\r"));
		// assigned to NanodeKit free room - https://pachube.com/feeds/40451
		Serial.print("nx "); showString(PSTR("f40451 - ")); showString(PSTR("NanoKit Free Room\r\n")); break; 
	}
	wdt_reset(); 
	while (ether.begin(sizeof Ethernet::buffer, macaddr) == 0) { showString(PSTR("Failed to access Ethernet controller\r\n")); }
	wdt_reset();
	while (!ether.dhcpSetup()) { showString(PSTR("DHCP failed\n\r")); }
	ether.printIp("IP:  ", ether.myip);
	ether.printIp("GW:  ", ether.gwip);  
	ether.printIp("DNS: ", ether.dnsip); 
	wdt_reset();	
	while (!ether.dnsLookup(PSTR("api.pachube.com"))) { showString(PSTR("DNS failed\r\n")); }
	ether.printIp("SRV: ", ether.hisip);  // IP for Pachupe API found by DNS service
}

void loop()
{
    // Use this delay hereunder when you want to verify the effect of the Watchdog timeout by forcing timeout
	// delay (10000); 
	
	// Reset watch dog timer to prevent timeout
	wdt_reset();

	ether.packetLoop(ether.packetReceive());  // check response from Pachube
	delay(100);
	Serial.print(".");
	
	if ( ( millis()-lastupdate ) > REQUEST_RATE )
	{
		lastupdate = millis();
		timer = lastupdate;
		
		showString(PSTR("\n************************************************************************************************\n"));    
		showString(PSTR("\nStarting Pachube update loop --- "));
		showString(PSTR("[memCheck bytes] ")); Serial.print(freeRam());
		showString(PSTR(" -- [Reboot Time Stamp] "));
		Serial.println( millis() - TimeStampSinceLastReboot );
		showString(PSTR("-> Check response from Pachube\n"));
		
		// DHCP expiration is a bit brutal, because all other ethernet activity and
		// incoming packets will be ignored until a new lease has been acquired
		//    Serial.print("-> DHCP? "); 
		//    if (ether.dhcpExpired() && !ether.dhcpSetup())
		//      Serial.println("DHCP failed");
		//   Serial.println("is fine"); 


		// ping server 
		ether.printIp("-> Pinging: ", ether.hisip);
		pingtimer = micros();
		ether.clientIcmpRequest(ether.hisip);
		if ( ( ether.packetReceive() > 0 ) && ether.packetLoopIcmpCheckReply(ether.hisip) ) 
		{
			showString(PSTR("-> ping OK = "));
			Serial.print((micros() - pingtimer) * 0.001, 3);
			Serial.println(" ms");
		} 
		else 
		{
			showString(PSTR("-> ping KO = "));
			Serial.print((micros() - pingtimer) * 0.001, 3);
			Serial.println(" ms");
		}

		
		//--------------------------------------
		// 1) Measurements and data preparation
		//--------------------------------------
		showString(PSTR("-> measurement cycle\n"));

		// Next data in the ramp
		Ramp0 =  Ramp0 + 1;
		Ramp1 =  Ramp1 + 1;  
		Ramp2 =  Ramp2 + 1;  
		Ramp3 =  Ramp3 + 1;      

		// To restart ramp
		if (Ramp0 >=  101) Ramp0 = 0;  
		if (Ramp1 >=  101) Ramp1 = 0;  
		if (Ramp2 >=  1001) Ramp2 = 0;  
		if (Ramp3 >=  10001) Ramp3 = 0;  

		byte sd = stash.create();  // Initialise streaming send data buffer
		
		stash.print("0,"); // Datastream 0
		stash.println( Ramp0 );
		stash.print("1,"); // Datastream 1
		stash.println( Ramp1 );
		stash.print("2,"); // Datastream 2
		stash.println( Ramp2 );
		stash.print("3,");  // Datastream 3
		stash.println( Ramp3 );

		// Stash MAC address here
		stash.print("4,");  // Datastream 3
		for (int i=0; i<6; i++) 
		{
			if (macaddr[i]<16) {
				stash.print("0");
			}
			stash.print(macaddr[i], HEX);
			if (i<5) {
				stash.print(":");
			} else {
				stash.print("");
			}
		}

		stash.save(); // Close streaming send data buffer

		//----------------------------------------
		// 2) Send the data
		//----------------------------------------

		// generate the header with payload - note that the stash size is used,
		// and that a "stash descriptor" is passed in as argument using "$H"
		Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
		"Host: $F" "\r\n"
		"X-PachubeApiKey: $F" "\r\n"
		"Content-Length: $D" "\r\n"
		"\r\n"
		"$H"),
		// assigned to NanodeKit free room - https://pachube.com/feeds/40451
		PSTR("api.pachube.com"), PSTR("40451"), PSTR("api.pachube.com"), PSTR(APIKEY), stash.size(), sd);

		// send the packet - this also releases all stash buffers once done
		showString(PSTR("-> sending...  "));
		Serial.print(ether.tcpSend());
		ether.tcpSend();
		showString(PSTR("  done sending\n\n"));
		showString(PSTR("---------- Waiting for REQUEST/PUT Message ----------\r\n"));
		
		// blink LED 6 a bit to show some activity on the board when sending to Pachube       
		for (int i=0; i < 4; i++) { digitalWrite(6,!digitalRead(6)); delay (50);} 
	}
}


// ++++++++++++++++
//    FUNCTIONS
// ++++++++++++++++

// Determines how much RAM is currently unused
int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// Display string stored in PROGMEM
void showString (PGM_P s)
{
	char c;
	while ((c = pgm_read_byte(s++)) != 0)
	Serial.print(c);
}

//// EEPROM Clear - Sets all of the bytes of the EEPROM to 0.
//void ClearEEPROM()
//{
//  // write a 0 to all 512 bytes of the EEPROM
//  for (int i = 0; i < 512; i++)
//    EEPROM.write(i, 0);
//}

void GetMac()
{
	showString(PSTR("Reading MAC address... "));
	bMac=unio.read(macaddr,NANODE_MAC_ADDRESS,6);
	if (bMac) showString(PSTR("success\n\r"));
	else showString(PSTR("failure\n\r"));
	
	showString(PSTR("MAC     : "));
	for (int i=0; i<6; i++) 
	{
		if (macaddr[i]<16) 
		{
			Serial.print("0");
		}
		Serial.print(macaddr[i], HEX);
		if (i<5) 
		{
			Serial.print(":");
		} 
		else 
		{
			Serial.print("");
		}
	}
	Serial.println("\n--");
}

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
	asm volatile ("  jmp 0");  
} 

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
int TimeOutLoopsBeforeReboot = 4;  // will reboot after getting X times into the timeout loop
ISR(WDT_vect)
{
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

