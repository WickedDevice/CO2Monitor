/***************************************************
     CUCO2 monitor code -- see README for more information
 ****************************************************/
#include <WildFire_CC3000.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <WildFire.h>
#include <LiquidCrystal.h>
#include <TinyWatchdog.h>

TinyWatchdog tinyWDT;

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

WildFire_CC3000 cc3000;
WildFire_CC3000_Client client;
WildFire wf;

uint32_t ip=0;
char address[13] = "";//Mac address
int experiment_id=0, CO2_cutoff=DEFAULT_CO2_CUTOFF;
long experimentStart = 0, millisOffset = 0;
char vignere_key[32] = ""; //Encryption key
volatile boolean offlineMode = false;
boolean justStarted = true; //Whether, during the experiment, the CO2 ppm has risen above the CO2_cutoff threshold
unsigned long loopTime = 0;

///Used for CO2 sensor
const byte readCO2[] = { 0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25 }; //Command packet to read Co2 (see app note)
byte response[] = { 0, 0, 0, 0, 0, 0, 0 }; //create an array to store the response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
const int valMultiplier = 1;

enum State { recording, error, no_experiment, uploading, done};

State state = error; //The state of our FSM

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Compiled on " __DATE__ ", " __TIME__));
  Serial.println(F("Server is " HOST));
  
  lcd.begin(16,2);
#ifdef INSTRUMENTED
  Serial.println(F("wdt disabled"));
  printTimeDiff(F("Millis: "));
#else
  //wdt_enable(WDT_WAIT);
  tinyWDT.begin(MIN_WDT_PET, MAX_WDT_PET);
#endif
  
  wf.begin();
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(MEM_SWITCH, INPUT_PULLUP);
  
  K_30_Serial.begin(9600);
  //lcd_print_top("Began WildFire");  
  
  //Initializing from nonvolatile memory
  if(!validEncryptionKey()) {
    Serial.println(F("\nInvalid encryption key"));
    while(!Serial.available()) {
      lcd_print_top("No encryption");
      lcd_print_bottom("key");
      delay(1000);
      lcd_print_top("Contact");
      lcd_print_bottom("Wicked Device");
      delay(1000);
    }
    configure();
  } else if (!validMemory()) {
    Serial.println(F("EEPROM has incorrect data"));
    Serial.println(F("Clearing data"));
    lcd_print_top("Invalid records..."); lcd_print_bottom("Clearing data");
    clearData();
    lcd_print_bottom("Please restart");
    state = error;
    return;
  }
  
  getEncryptionKey(vignere_key);
  
  ///////////// WIFI setup /////////////
  
  if(attemptSmartConfig()) {
    
    while(!attemptSmartConfigCreate()){
      Serial.println(F("Waiting for Smart Config Create"));
    }

  } else {
    Serial.println(F("Attempting to reconnect"));
    lcd_print_top("Reconnecting...");
    
    petWDT();
    if(!attemptSmartConfigReconnect()){
      
      if(BUTTON_PUSHED || offlineMode) {
        if(hasMoreData()) {
          lcd_print_top("Old Data found");
          lcd_print_bottom("Aborted");
          Serial.println(F("\nData from last reboot found.\nCannot collect more data before uploading."));
          state = error;
          return;
        }
        state = no_experiment;
        offlineMode = true;
        return;
      }
      
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

  Serial.println(F("IP address:"));
  
  lcd_print_top("Finding server");
  lcd_print_bottom("IP address");
  
  uint32_t time = millis();
  while(time + 5000 > millis() && ip == 0) {
    cc3000.getHostByName(HOST, &ip);
  }
  if(ip == 0) {
    lcd_print_top("Couldn't resolve");
    lcd_print_bottom("Server IP");
    Serial.print(F("Couldn't resolve server IP"));
    delay(1000);
    soft_reset();
  }
  
  petWDT();
  cc3000.printIPdotsRev(ip);
  Serial1.begin(9600);
  
#ifdef INSTRUMENTED
  printTimeDiff(F("printIPdotsRev: "));
#endif
  ///////////////// END WIFI setup ///////////
  
  //Finding MAC address (must happen after cc3000.begin() is called)
  uint8_t mac[6] = "";
  if(!cc3000.getMacAddress(mac)) {
    Serial.println(F("Error: unable to get mac address"));
    lcd_print_top("Error:No MACaddr");
    state = error;
    return;
  } else {
    mactoa(mac,address);
  }
  
  
  state = no_experiment;

  if(hasMoreData()) {
    Serial.println(F("Attempting to upload old data"));
    lcd_print_top("Uploading data"); lcd_print_bottom("from last reboot");
    experiment_id = getExperimentId();
    state = uploading;
  } else {
    clearData();
  }
  
} /////end setup /////


///////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void) {  
  
#ifdef INSTRUMENTED
  printTimeDiff(F("Reached top of loop: "));
#endif

  petWDT();
  
  if(!validMemory()) {
    state = error;
    lcd_print_top("Corrupted memory");
    lcd_print_bottom("Contact service");
  }
  
  switch (state) {
    case no_experiment: {
      //Queries the server to see if there is an experiment
      Serial.println(F("Checking for experiment\nPush button to start anyway."));

      lcd_print_top("Querying server");
      lcd_print_bottom("Hold to skip");

      experiment_id = 0;
      if(BUTTON_PUSHED || offlineMode) {
        lcd_print_top("Offline mode");
        Serial.println(F("Offline mode initiated"));
        delay(500); //allowing user to let go of button
        offlineMode = true;
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
          if(BUTTON_PUSHED) {
            offlineMode = true;
            break;
          }
        }//end waiting for message
      }
      
      break;
    }//////end no_experiment ///////
    
    case recording: {

      if(millis() - loopTime < MAX_UPDATE_SPEED) {
        //If we read data in the last MAX_UPDATE_SPEED, delay
        break;
        //delay(MAX_UPDATE_SPEED + loopTime - millis());
      }
      
      if(!sendRequest(readCO2)) {
        lcd_print_top("Check sensor");
        lcd_print_bottom("connection");
        Serial.println(F("Check sensor connection"));
        delay(100);
        break;
      }
      
      unsigned long valCO2 = getValue(response);
      
      if(valCO2 <= 10000L && valCO2 > 0L) {
        //If valCO2 is a legal value,
        //  Record data & wait
        
        lcd_print_top("CO2 ppm: "); lcd.print(valCO2);
      
        Serial.print(F("Recording data... CO2 ppm is: "));
        Serial.println(valCO2);
      
        saveDatum(valCO2);
        loopTime = millis();
      } else {
        lcd_print_top("Bad reading");
        Serial.print(F("Bad reading: ")); Serial.println(valCO2);
      }
      
#ifdef INSTRUMENTED
  printTimeDiff(F("Read Data: "));
#endif

#ifdef REALTIME_UPLOAD
      state = uploading;
#endif

      if(offlineMode) {
        lcd_print_bottom("Offline Mode");
      } else {
        lcd_print_bottom("Hold to upload");
        Serial.println(F("Recording data... Push button to force upload."));
      }
      
      if(MEM_RESET_PUSHED) {
        clearData();
        lcd_print_top("Memory cleared");
        delay(1000);
        soft_reset();
      } else if(BUTTON_PUSHED && hasMoreData()) {
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
      
      if(offlineMode) {
        lcd_print_bottom("Offline mode");
        delay(1000);
        lcd_print_top("Restart sensor");
        lcd_print_bottom("to upload");
        state = error;
        break;
      }
      
      if(!hasMoreData()) {
#ifdef REALTIME_UPLOAD
        state = (experimentEnded() || outOfSpace()) ? done : recording;
#else
        state = done; //erase data & query server again
#endif
        break;
      }

      int dataInPacket = assemblePacket();
      
      Serial.println(F("Sending data..."));
      lcd_print_top("Sending data...");
      lcd_print_bottom("");
      lcd.print((int) (ratioSent()*100));
      lcd.print("% done");
      
      
      if(sendPacket()) {
        dataSent();
      } else {
        lcd_print_top("Upload failed");
        lcd_print_bottom("Retrying");
        delay(1000);
        prevDataNotSent();
        
        if(!cc3000.checkConnected()) {
          //Restart if the Wifi connection stopped
          soft_reset();
        }
      }
      
      break;
    }//End uploading
    
    case error: {
      if(MEM_RESET_PUSHED) {
        clearData();
        lcd_print_top("Memory Cleared");
        delay(1000);
        soft_reset();
      }
      delay(50);
      break;
    }/////end error /////
    
    case done: { //Upload finished, do some cleanup before searching for new experiment
      petWDT();
      
      Serial.print(F("Upload complete."));
      
      lcd_print_top("Upload");lcd_print_bottom("complete");
      
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
      Serial.println(F("Error: Undefined state!"));
    }
  }
  return;
} /////end loop /////
