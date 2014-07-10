
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//Location of the server:
//#define HOST "192.168.1.2"
#define HOST "aerosense.cc"

#define LISTEN_PORT           80    // What TCP port to listen on for connections.

#define PACKET_SIZE 50                          //Number of datapoints in a packet
                                                   // Don't make this too big or the Wildfire won't have space for it
                                                   // This shouldn't cause packets to exceed TX_BUFFER_SIZE for the CC3000
                                                   
#define DATA_MAX_LENGTH (PACKET_SIZE * 35 + 150) //Maximum length of the data string to submit in the packet
                                                   // <35 for each datapoint, 150 for metadata
char packet_buffer[160 + DATA_MAX_LENGTH + 64];  //Array that holds the packet
                                                   // ~160 for header, ~64 for extra space


#define DEFAULT_CO2_CUTOFF 2000    //when an offline experiment dips below this ppm of CO2, recording stops


#define SMARTCONFIG_WAIT 4000       //Time to wait before attempting to reconnect (milliseconds)

#define MIN_WDT_PET 1000   //Pets of the Watchdog timer have to be farther apart than this to register (millis)
#define MAX_WDT_PET 60000  //Amount of time the watch dog timer waits before restarting the WildFire (millis)


#define MAX_UPDATE_SPEED 2000   //Read the sensor at most this often


//If defined, prints out a bunch of timing related debuggery & disables the watchdog timer
//#define INSTRUMENTED

//If this is defined, the device attempts to send realtime data as it is collected
//#define REALTIME_UPLOAD


///////////////////////
/////// Wiring ////////
///////////////////////
#define WITH_SHIELD true //If the WildFire is wired for the shield.

//For Wiring:
//  Both the interface button & the memory clearing switch are activated when low.

#if WITH_SHIELD == true
  //HAS NOT BEEN TESTED WITH AN ACTUAL SHIELD!!!


  //Order is: LCD( RS, Enable, D4, D5, D6, D7 )
  LiquidCrystal lcd(A3, A2, 4, 5, 6, 8);
  
  #define MEM_SWITCH A1
  
  //Stuff for the button
  #define BUTTON             A0
  #define BUTTON_PUSHED      ((PINA & _BV(0)) == 0) //faster than: (!digitalRead(BUTTON))
  #define BUTTON_INT_vect    PCINT0_vect
  
    //Pin Change Interrupt control enabling & disabling for BUTTON
  #define BUTTON_INIT_PCINT do{\  
    pcicr = PCICR;\
    pcmsk = PCMSK0;\
    PCICR |=  _BV(PCIE0);\
    PCMSK0 |= _BV( PCINT0 );\
    } while(0)
  
  #define BUTTON_RESTORE_PCINT do{\
    PCICR = pcicr;\
    PCMSK0 = pcmsk;\
    } while(0)
    
    
#elif WITH_SHIELD == false

  //Stuff for the button
  #define BUTTON            5
  #define BUTTON_PUSHED     ((PIND & _BV(5)) == 0) //faster than: (!digitalRead(BUTTON))
  #define BUTTON_INT_vect   PCINT3_vect
  
    //Pin Change Interrupt control enabling & disabling for BUTTON
  #define BUTTON_INIT_PCINT do{\  
    pcicr = PCICR;\
    pcmsk = PCMSK3;\
    PCICR |=  _BV(PCIE3);\
    PCMSK3 |= _BV( PCINT29 );\
    } while(0)
  
  #define BUTTON_RESTORE_PCINT do{\
    PCICR = pcicr;\
    PCMSK3 = pcmsk;\
    } while(0)
  
  
  #define MEM_SWITCH 6                 //Pin that the memory reset switch is attached to
  
  //Order is: LCD( RS, Enable, D4, D5, D6, D7 )
  LiquidCrystal lcd(A1, A2, A3, A4, A5, A6); //Pins attached to the LCD display.
  //  http://arduino.cc/en/Tutorial/LiquidCrystal

#endif

#define MEM_RESET_PUSHED (BUTTON_PUSHED && !digitalRead(MEM_SWITCH))
#define K_30_Serial Serial1



#if WILDFIRE_VERSION == 2
#pragma message "Watch dog timer code needs fixing."
#pragma message "If you're changing the max time without petting the timer to less than 10 seconds:"
#pragma message "    you'll also need to fix SmartConfig reconnect to timeout in less than 10 seconds,"
#pragma message "    or have it respond appropriately to the offlineMode interrupt"
#elif WILDFIRE_VERSION == 3
//Probably won't be displayed during compilation, sadly
#pragma message "Make sure that the RFM69 or R16 is depopulated"
#else
#endif


