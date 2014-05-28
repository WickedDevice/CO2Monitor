/***************************************************
     Early version of CUCO2 monitor code
     Currently treats device as always active
     Continuously posts to server without waiting for response
 ****************************************************/

#include <WildFire_CC3000.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <WildFire.h>

#include "header.h"

//#include <avr/wdt.h>

#define soft_reset()        \
do                          \
{                           \
    wdt_enable(WDTO_15MS);  \
    for(;;)                 \
    {                       \
    }                       \
} while(0)

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();
    return;
}

uint32_t ip;
WildFire_CC3000_Client client;

char address[13] = "";//Mac address
int experiment_id, time_or_CO2, CO2_cutoff, time_cutoff;
int experimentStart = 0, millisOffset = 0;

///Used for CO2 sensor
byte readCO2[] = { 0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25 }; //Command packet to read Co2 (see app note)
byte response[] = { 0, 0, 0, 0, 0, 0, 0 }; //create an array to store the response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;

enum State { recording, error, no_experiment, uploading, done};

State state = error;

void setup(void)
{
  wdt_enable(WDTO_8S);
  
  wf.begin();
  pinMode(BUTTON, INPUT_PULLUP);
  
  Serial.begin(115200);
  K_30_Serial.begin(9600); //is this value correct?
  Serial.println(F("Welcome to WildFire!\n"));  
  
  if(attemptSmartConfig()) {

    while(!attemptSmartConfigCreate()){
      Serial.println(F("Waiting for Smart Config Create"));
    }
    
  } else {
    Serial.println(F("Attempting to reconnect"));
    wdt_reset();
    if(!attemptSmartConfigReconnect()){
      Serial.println(F("Reconnect failed!"));
      cc3000.disconnect();
      soft_reset();
    }
    
  }
  
  while(!displayConnectionDetails());
  
  // Get the website IP & print it
  ip = IP_3;
  ip |= (((uint32_t) IP_2) << 8);
  ip |= (((uint32_t) IP_1) << 16);
  ip |= (((uint32_t) IP_0) << 24);

   wdt_reset();
  cc3000.printIPdotsRev(ip);
  Serial1.begin(9600);
  
  //Finding Mac address
  uint8_t mac[6] = "";
  if(!cc3000.getMacAddress(mac)) {
    Serial.println(F("Error: unable to get mac address"));
  }
  mactoa(mac,address);
  
  
  
  Serial.println(F("\nSHOULD check for cached data."));
  state = no_experiment;
}


void loop(void) {
  wdt_reset();
  
  switch (state) {
    case no_experiment: {
      //Queries the server to see if there is an experiment
      Serial.println(F("SHOULD BE: Checking for experiment\nPush button to start anyway."));

      experiment_id = 0;
      if(!digitalRead(BUTTON)) {
        //Button pushed
        experimentStart = 0;
        millisOffset = millis();
        experiment_id = -1;
      } else {
        checkForExperiment(experiment_id, time_or_CO2, CO2_cutoff, time_cutoff);
      }
      
      if(experiment_id != 0) {
        state = recording;
      }
      
      //Silly stub:
      
      state = recording;
      
      break;
    }
    
    case recording: {
      if(!sendRequest(readCO2)) {
        Serial.println(F("Check sensor connection"));
        state = error;
        break;
      }
      
      unsigned long valCO2 = getValue(response);
      
      //Later this will probably check to see if the datavalue has changed,
      //  Or have a better delay.
      saveDatum(valCO2);
      
      Serial.println("Recording data... Push button to force upload.");
      if(!digitalRead(BUTTON) || experimentEnded()) {
        Serial.println("Data collection stopped");
        
        state = uploading;
        break;
      }
      
      delay(2000);
      
      break;
    }
    
    case uploading: {
      //Implement doing something here...
      
      if(!hasMoreData()) {
        state = done;
        break;
      }

      if(!sendPacket()) {
        Serial.println(F("SHOULD Reconnect and try again..."));
      }
      
      break;
    }
    
    case error: {
      delay(1000);
      break;
    }
    
    case done: {
      wdt_reset();
      Serial.println(F("Experiment complete. Have a nice day."));
      if(cc3000.checkConnected()) {
        cc3000.disconnect();
      }
      delay(1000);
      state = no_experiment;
      break;
    }
    
    default: {
      Serial.println("Error: Undefined state!");
    }
  }
  return;
}
