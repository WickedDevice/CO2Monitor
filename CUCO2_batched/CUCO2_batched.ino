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
#include <LiquidCrystal.h>

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
WildFire_CC3000 cc3000;
WildFire_CC3000_Client client;
WildFire wf;

char address[13] = "";//Mac address
int experiment_id, CO2_cutoff;
long experimentStart = 0, millisOffset = 0;

#define MAX_UPDATE_SPEED 2000 //Read the sensor at most this often
unsigned long loopTime = 0;

///Used for CO2 sensor
byte readCO2[] = { 0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25 }; //Command packet to read Co2 (see app note)
byte response[] = { 0, 0, 0, 0, 0, 0, 0 }; //create an array to store the response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;

enum State { recording, error, no_experiment, uploading, done};

State state = error;

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Programmed on " __DATE__ ", " __TIME__));
  Serial.println(F("Server is " HOST));
  
  lcd.begin(16,2);
#ifdef INSTRUMENTED
  Serial.println(F("wdt disabled"));
  printTimeDiff(F("Millis: "));
#else
  wdt_enable(WDTO_8S);
#endif
  
  wf.begin();
  pinMode(BUTTON, INPUT_PULLUP);
  
  
  K_30_Serial.begin(9600);
  //lcd_print_top("Began WildFire");
  
  if(attemptSmartConfig()) {
    
    while(!attemptSmartConfigCreate()){
      Serial.println(F("Waiting for Smart Config Create"));
    }

  } else {
    Serial.println(F("Attempting to reconnect"));
    lcd_print_top("Reconnecting...");
    
    wdt_reset();
    if(!attemptSmartConfigReconnect()){
      Serial.println(F("Reconnect failed!"));
      lcd_print_top("Reconnect failed");
      cc3000.disconnect();
      soft_reset();
    }
    
  }

#ifdef INSTRUMENTED
  printTimeDiff(F("Smart Config: "));
#endif

  while(!displayConnectionDetails());

#ifdef INSTRUMENTED
  printTimeDiff(F("displayConnectionDetails:"));
#endif

  // Get the website IP & print it
  ip = IP_3;
  ip |= (((uint32_t) IP_2) << 8);
  ip |= (((uint32_t) IP_1) << 16);
  ip |= (((uint32_t) IP_0) << 24);

   wdt_reset();
  cc3000.printIPdotsRev(ip);
  Serial1.begin(9600);
  
#ifdef INSTRUMENTED
  printTimeDiff(F("printIPdotsRev: "));
#endif

  //Finding Mac address
  uint8_t mac[6] = "";
  if(!cc3000.getMacAddress(mac)) {
    Serial.println(F("Error: unable to get mac address"));
    lcd_print_top("Error:No MACaddr");
  }
  mactoa(mac,address);
  
#ifdef INSTRUMENTED
  printTimeDiff(F("Found mac address: "));
#endif
  
  state = no_experiment;
  
  if(!validMemory()) {
    Serial.println(F("EEPROM has incorrect data"));
    lcd_print_top("Invalid records..."); lcd_print_bottom("Clearing data");
    clearData();
  } else if(hasMoreData()) {
    Serial.println(F("Attempting to upload old data"));
    lcd_print_top("Uploading data"); lcd_print_bottom("from last reboot");
    experiment_id = getExperimentId();
    state = uploading;
  } else {
    clearData();
  }
  
} /////end setup /////


void loop(void) {
#ifdef INSTRUMENTED
  printTimeDiff(F("Reached top of loop: "));
#endif

  wdt_reset();
  
  switch (state) {
    case no_experiment: {
      //Queries the server to see if there is an experiment
      Serial.println(F("Checking for experiment\nPush button to start anyway."));

      lcd_print_top("Querying server");
      lcd_print_bottom("Hold to skip");

      experiment_id = 0;
      if(!digitalRead(BUTTON)) {
        //Button pushed
        experimentStart = 0L;
        millisOffset = millis();
        experiment_id = -1;
      } else {
        checkForExperiment(experiment_id, CO2_cutoff);
      }
      
      if(experiment_id != 0) {
        state = recording;
        setExperimentId(experiment_id);
      } else {
        lcd_print_top("No experiment"); lcd_print_bottom("found");
        
        //Leave the message up for a second
        int time = 0;
        for(time = 0; time < 10; time++) {
          delay(100);
          if(!digitalRead(BUTTON)) {
            experimentStart = 0L;
            millisOffset = millis();
            experiment_id = -1;
            break;
          }
        }
      }
      
      break;
    }//////end no_experiment ///////
    
    case recording: {

      if(millis() - loopTime < MAX_UPDATE_SPEED) {
        //If we read data in the past MAX_UPDATE_SPEED, delay
        delay(MAX_UPDATE_SPEED + loopTime - millis());
      }
      
      if(!sendRequest(readCO2)) {
        lcd_print_top("Check sensor");
        lcd_print_bottom("connection");
        Serial.println(F("Check sensor connection"));
        state = error;
        break;
      }
      
      unsigned long valCO2 = getValue(response);
      
      lcd_print_top("CO2 ppm: "); lcd.print(valCO2);
      
      Serial.print(F("Recording data... CO2 ppm is: "));
      Serial.println(valCO2);
      
      saveDatum(valCO2);
      
#ifdef INSTRUMENTED
  printTimeDiff(F("Read Data: "));
#endif


      loopTime = millis();

#ifdef REALTIME_UPLOAD
      state = uploading;
#endif
      lcd_print_bottom("Hold to upload");
      Serial.println("Recording data... Push button to force upload.");
      if(!digitalRead(BUTTON) && hasMoreData()) {
        lcd_print_top("Button pushed");
        Serial.println(F("Interrupted recording early."));
        state = uploading;
      } else if(experimentEnded()) {
        lcd_print_top("Experiment ended");
        Serial.println(F("Experiment finished."));
        state = uploading;
      } else if(outOfSpace()) {
        lcd_print_top("Out of memory");
        Serial.println(F("Out of Memory."));
        state = uploading;
      }
      break;
    }/////end recording /////
    
    case uploading: {
      
      if(!hasMoreData()) {
        if(experimentEnded() || outOfSpace() ) {
          state = done; 
        } else {
          state = no_experiment; // If someone pushed the button, it will stop recording anyway
        }
        break;
      }

      int dataInPacket = assemblePacket();

      if(!sendPacket()) {
        Serial.println(F("SHOULD Reconnect and try again..."));
        prevDataNotSent(dataInPacket);
      }
      
      break;
    }//End uploading
    
    case error: {
      delay(1000);
      break;
    }/////end error /////
    
    case done: {
      wdt_reset();
      
      Serial.print(F("Experiment complete."));
      
      lcd_print_top("Experiment");lcd_print_bottom("complete");
      
      //Client should have been closed in SendPacket
      /*
      if(client.connected()) {
        Serial.println(F("\nClosing client"));
        client.close();
        delay(1000); //Need to wait 2 seconds before reconnecting to client
      }
#ifdef INSTRUMENTED
  printTimeDiff(F("Disconnected: "));
#endif
      */
      
      experimentCleanup();
      Serial.println(F(" Have a nice day."));
      state = no_experiment;
      
      delay(1000);
      break;
    }/////end done //////
    
    default: {
      Serial.println("Error: Undefined state!");
    }
  }
  return;
} /////end loop /////
