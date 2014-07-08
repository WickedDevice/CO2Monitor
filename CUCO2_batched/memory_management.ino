/////////////////////////////////
////// Memory Management ////////
/////////////////////////////////

#include <avr/eeprom.h>

// This implementation saves data to EEPROM.
//    Uses a 'struct' of an integer for the ppm, long for the time.
//    ....time][#######ppm######][##############time##############][ppm...


////// Macro definitions

#define SAVE_SPACE 600                                  //Number of datapoints that can be saved
   //Each datapoint takes up 6 bytes. 4096 / 6 = 682.666. Bottom 60 or so bytes are used for other information


#define ENCRYPTION_MAGIC_NUM_LOC ((byte *) 0)

#define ENCRYPTION_KEY_PTR ((byte *) (ENCRYPTION_MAGIC_NUM_LOC+1))

#define MAGIC_NUM_LOC ((byte *) (ENCRYPTION_KEY_PTR + 32))
#define MAGIC_NUM_VAL 'D'                               // 'Magic number' that tells if the eeprom has been compromised/overwritten

#define EXPERIMENT_PTR ((uint16_t *) MAGIC_NUM_LOC+1)
#define SENT_PTR (EXPERIMENT_PTR + 1)                   // Where the number of sent values is kept

#define SAVE_START (SENT_PTR + sizeof(void *))          // Where the saved data begins
#define SAVE_END (SAVE_START + (SAVE_SPACE * sizeof(long int))) //and where it ends

#define ILLEGAL_VALUE 65535 //UINT16_MAX, but isn't defined in the Arduino IDE

//Gets the number of saved or sent datapoints.
#define SENT() eeprom_read_word(SENT_PTR)

#define RECORD_SIZE (sizeof(long int) + sizeof(int))  //size of a single record

//Gets the nth record's ppm or time value's location (indexes at 0)
#define PPM_AT(n)   ((uint16_t *)((n)*RECORD_SIZE + SAVE_START))
#define TIME_AT(n)  ((uint32_t *)((n)*RECORD_SIZE + SAVE_START + sizeof(int)))

uint16_t dataRead = SENT();

uint16_t savedCounter = 0; //number of values aready saved in EEPROM (access this through the savedValues function)

boolean invalidMemory = false;

///// Functions

//Gets the number of values already saved in EEPROM
inline uint16_t savedValues() {
  //Uses memoization
  
  while(eeprom_read_word(PPM_AT(savedCounter)) != ILLEGAL_VALUE)
    { savedCounter++; }
  return savedCounter;
}

//Returns a ratio: successfully sent datapoints / all saved datapoints
float ratioSent() {
  return (float) SENT() /(float) savedValues();
}

boolean saveDatum(unsigned int valCO2) {
  //Saves data to array, eeprom, or memory
  if(outOfSpace()) {
    return false;
  }
  
  uint16_t saved = savedValues();
  
  unsigned long time = experimentSeconds();
  
  //Write to save location
  Serial.print(F("Writing to EEPROM locations ")); Serial.print((int)PPM_AT(saved));
  Serial.print(F(" to ")); Serial.println((int) PPM_AT(saved+1));
  
  eeprom_write_word(PPM_AT(saved), valCO2);
  eeprom_write_dword(TIME_AT(saved), time);
  
  //Update save value
  eeprom_write_word( PPM_AT(saved+1), ILLEGAL_VALUE );
  
  //Verify that it was written properly
  if( eeprom_read_word(   PPM_AT(saved)) != valCO2
   || eeprom_read_dword( TIME_AT(saved)) != time
   || eeprom_read_word( PPM_AT(saved+1)) != ILLEGAL_VALUE) {
     invalidMemory = true;
   }
  
  return true;
}

boolean outOfSpace(void) {
  uint16_t saved = savedValues();
  
  if( saved >= SAVE_SPACE) {
    Serial.println(F("Out of space"));
    return true;
  } else {
    return false;
  }
}


//Determines whether there are more data to send
boolean hasMoreData(void) {
  return (savedValues() > dataRead ? true : false);
}


int mostRecentDataAvg(int numToAverage = 5) {
  uint16_t saved = savedValues();
  numToAverage = (numToAverage > saved) ? saved : numToAverage;
  
  unsigned long sum = 0;
  for(int i = 1; i <= numToAverage; i++) {
    sum += eeprom_read_word(PPM_AT(saved-i));
  }
  
  return sum / numToAverage;
}

//Gets the next datapoint
void nextDatum(int &ppm, long &timestamp) {
  
  ppm =       eeprom_read_word( PPM_AT( dataRead));
  timestamp = eeprom_read_dword(TIME_AT(dataRead));
  
  dataRead++;
  
  return;
}

//Allows (re)sending of data that has been read already
void prevDataNotSent() {
  dataRead = SENT();
  return;
}

//Updates EEPROM to reflect that some data has been sent
void dataSent() {
  eeprom_write_word(SENT_PTR, dataRead);
  
  if(eeprom_read_word(SENT_PTR) != dataRead)
  {  invalidMemory = true; }
}

//'deletes' all data from eeprom & reconfigures memory.
void clearData() {
  eeprom_write_byte( MAGIC_NUM_LOC, '\0');//make memory invalid until this is fixed
  eeprom_write_word( SENT_PTR, 0);
  eeprom_write_word( PPM_AT(0), ILLEGAL_VALUE); //'removing' the saved data
  eeprom_write_byte( MAGIC_NUM_LOC, MAGIC_NUM_VAL);
  dataRead = SENT();//0
  setExperimentId(0);
  
  savedCounter = 0;
  
  //Verify newly written memory
  if( eeprom_read_word( SENT_PTR ) != 0
   || eeprom_read_word(PPM_AT(0)) != ILLEGAL_VALUE
   || eeprom_read_byte(MAGIC_NUM_LOC) != MAGIC_NUM_VAL) {
     invalidMemory = true;
  }
}


boolean validMemory() {
  return eeprom_read_byte(MAGIC_NUM_LOC) == MAGIC_NUM_VAL
      && eeprom_read_byte(ENCRYPTION_MAGIC_NUM_LOC) == MAGIC_NUM_VAL
      && !invalidMemory;
}

int getExperimentId() {
  return (int) eeprom_read_word(EXPERIMENT_PTR);
}
void setExperimentId(int experiment_id) {
  eeprom_write_word(EXPERIMENT_PTR, (uint16_t) experiment_id);
  
  //Verify newly written memory
  if(eeprom_read_word(EXPERIMENT_PTR) != (uint16_t) experiment_id)
  {  invalidMemory = true; }
}


boolean validEncryptionKey() {
  return eeprom_read_byte(ENCRYPTION_MAGIC_NUM_LOC) == MAGIC_NUM_VAL;
}

void getEncryptionKey(char *buffer) {
  char c;
  int i = 0;
  do {
    c = eeprom_read_byte(ENCRYPTION_KEY_PTR + i);
    buffer[i] = c;
    i++;
  } while(c != '\0');
}

void setEncryptionKey(char *key) {
  int keyLength = strlen(key) > 32 ? 32 : strlen(key);
  int i;
  for(i = 0; i < keyLength; i++) {
    eeprom_write_byte(ENCRYPTION_KEY_PTR + i, key[i]);
    
    if(eeprom_read_byte(ENCRYPTION_KEY_PTR+i) != key[i]) {
      //Validate newly set byte
      invalidMemory = true;
    }
  }
  eeprom_write_byte(ENCRYPTION_KEY_PTR + i, '\0');
  
  eeprom_write_byte(ENCRYPTION_MAGIC_NUM_LOC, MAGIC_NUM_VAL);
  
  //Validate '\0' && magic number
  if( eeprom_read_byte(ENCRYPTION_KEY_PTR + i)   != '\0'
   || eeprom_read_byte(ENCRYPTION_MAGIC_NUM_LOC) != MAGIC_NUM_VAL) {
    invalidMemory = true;
   }
}
