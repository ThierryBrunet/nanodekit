/* NanodeKit = Test code for Nanode v5 kit after assembly and test feed to Pachube
----------------------------------------------------------------------------------
V1.0
MercinatLabs / MERCINAT SARL France
By: Thierry Brunet de Courssou
http://www.mercinat.com
Created 12 Oct  2011
Last update: 25 Nov 2011
Project hosted at: http://code.google.com/p/nanodebuildtest/ - Repository type: Subversion
Version Control System: TortoiseSVN 1.7.1, Subversion 1.7.1, for Window7 64-bit - http://tortoisesvn.net/downloads.html

Configuration
-------------
Hardware: Nanode v5
Software: Arduino IDE RC2 for Windows at http://code.google.com/p/arduino/wiki/Arduino1
-- did not test with Arduino IDE 0022 or 0023

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

// RAM space is very limited so use PROGMEM to conserve memory with strings
// However PROGMEM brings some instabilities, so not using it for now. 
// Will check from time to time if this is solved in future versions of 
// Arduino IDE and EtherCard library.

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
                        
#define REQUEST_RATE 5000 // in milliseconds - Pachube update rate
unsigned long lastupdate;  // timer value when last Pachube update was done
uint32_t timer;  // a local timer

byte Ethernet::buffer[700];
Stash stash;     // For filling send buffer using satndard "print" instructions

// Initialise values for 4 ramps, each assigned to a different datastream
float Ramp0 = 0;
float Ramp1 = 50;
float Ramp2 = 500;
float Ramp3 = 5000;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\nNanodeKit V1 - MercinatLabs (25 Nov 2011)");
  Serial.println("[getDHCPandDNS] [Pachube webClient]");
 
  GetMac(); // Get and print MAC (via NanodeUNIO)
  
  // Identify which sensor is assigned to ths board
  // If you have boards with identical MAC last 2 values, you will have to adjust you code accordingly
  switch ( macaddr[5] )
  {
    case 0xFA: MyNanode = 6;  Serial.print("n6 "); Serial.print("f38277 - "); Serial.println("Etel 6 m") ; break; 
    case 0xC4: MyNanode = 9;  Serial.print("n9 "); Serial.print("f38278 - "); Serial.println("Etel 9 m") ; break;
    case 0xF4: MyNanode = 8;  Serial.print("n8 "); Serial.print("f38279 - "); Serial.println("Etel 12 m"); break;
    case 0xAF: MyNanode = 7;  Serial.print("n7 "); Serial.print("f38281 - "); Serial.println("Etel 18 m"); break;
    case 0xD6: MyNanode = 1;  Serial.print("n1 "); Serial.print("f37667 - "); Serial.println("Aurora")   ; break;
    case 0xC2: MyNanode = 2;  Serial.print("n2 "); Serial.print("f37668 - "); Serial.println("FemtoGrid"); break;
    case 0xEA: MyNanode = 3;  Serial.print("n3 "); Serial.print("f35020 - "); Serial.println("Skystream"); break;
    case 0xAC: MyNanode = 4;  Serial.print("n4 "); Serial.print("f40385 - "); Serial.println("Grid RMS #1"); break;
    case 0x8E: MyNanode = 5;  Serial.print("n5 "); Serial.print("f40386 - "); Serial.println("Grid RMS #2"); break;
    default:  
      Serial.println("unknown Nanode");
      // assigned to NanodeKit free room - https://pachube.com/feeds/40451
      Serial.print("nx "); Serial.print("f40451 - "); Serial.println("NanoKit Free Room"); break; 
   }
  
  while (ether.begin(sizeof Ethernet::buffer, macaddr) == 0) { Serial.println( "Failed to access Ethernet controller"); }
  while (!ether.dhcpSetup()) { Serial.println("DHCP failed"); }
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
  while (!ether.dnsLookup(PSTR("api.pachube.com"))) { Serial.println("DNS failed"); }
  ether.printIp("SRV: ", ether.hisip);  // IP for Pachupe API found by DNS service

}

void loop()
{
  ether.packetLoop(ether.packetReceive());  // check response from Pachube
      
  if ( ( millis()-lastupdate ) > REQUEST_RATE )
  {
    lastupdate = millis();
    timer = lastupdate;
    
    // DHCP expiration is a bit brutal, because all other ethernet activity and
    // incoming packets will be ignored until a new lease has been acquired
//    if (ether.dhcpExpired() && !ether.dhcpSetup())
//      Serial.println("DHCP failed");

    //--------------------------------------
    // 1) Measurements and data preparation
    //--------------------------------------
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
    ether.tcpSend();
  }
}

void GetMac()
  {
    Serial.print("Reading MAC address... ");
    bMac=unio.read(macaddr,NANODE_MAC_ADDRESS,6);
    if (bMac) Serial.println("success");
    else Serial.println("failure");
    
    Serial.print("MAC     : ");
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



