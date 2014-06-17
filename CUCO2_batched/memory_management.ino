/////////////////////////////////
////// Memory Management ////////
/////////////////////////////////

#include <avr/eeprom.h>

// This implementation saves data to EEPROM.
//    Uses a 'struct' of an integer for the ppm, long for the time.
//    ....time][#######ppm######][##############time##############][ppm...


////// Macro definitions

#define SAVE_SPACE 500                                  //Number of datapoints saved

//#define encryption_key_loc

#define MAGIC_NUM_LOC ((byte *) 32)                     // First 32 bytes are saved for other things... probably
#define MAGIC_NUM_VAL 'D'                               // 'Magic number' that tells if the eeprom has been compromised/overwritten

#define EXPERIMENT_PTR ((uint16_t *) MAGIC_NUM_LOC+1)
#define SAVE_PTR (EXPERIMENT_PTR + 1)                   // Where the number of saved datapoints is kept
#define SENT_PTR (SAVE_PTR + 1)                         // Where the number of sent values is kept

#define SAVE_START (SENT_PTR + sizeof(void *))          // Where the saved data begins
#define SAVE_END (SAVE_START + (SAVE_SPACE * sizeof(long int))) //and where it ends

//Gets the number of saved or sent datapoints.
#define SAVED() eeprom_read_word(SAVE_PTR)
#define SENT() eeprom_read_word(SENT_PTR)

#define RECORD_SIZE (sizeof(long int) + sizeof(int))  //size of a single record

//Gets the nth record's ppm or time value's location (indexes at 0)
#define PPM_AT(n)   ((uint16_t *)((n)*RECORD_SIZE + SAVE_START))
#define TIME_AT(n)  ((uint32_t *)((n)*RECORD_SIZE + SAVE_START + sizeof(int)))

///// Functions

boolean saveDatum(unsigned int valCO2) {
  //Saves data to array, eeprom, or memory
  if(outOfSpace()) {
    return false;
  }
  
  uint16_t saved = SAVED();
  
  unsigned long time = experimentSeconds();
  
  //Write to save location
  eeprom_write_word(PPM_AT(saved), valCO2);
  eeprom_write_dword(TIME_AT(saved), time);
  
  //Update save value
  eeprom_write_word( SAVE_PTR, saved+1 );
  
  return true;
}

boolean outOfSpace(void) {
  uint16_t saved = SAVED();
  
  if( saved >= SAVE_SPACE) {
    Serial.println(F("Out of space"));
    return true;
  } else {
    return false;
  }
}

boolean hasMoreData(void) {
  //Determines whether there are more data to send
  uint16_t saved = SAVED();
  uint16_t sent  = SENT();
  
  return (saved > sent ? true : false);
}


int mostRecentDataAvg(int numToAverage = 5) {
  uint16_t saved = SAVED();
  numToAverage = (numToAverage > saved) ? saved : numToAverage;
  
  unsigned long sum = 0;
  for(int i = 1; i <= numToAverage; i++) {
    sum += eeprom_read_word(PPM_AT(saved-i));
  }
  
  return sum / numToAverage;
}

void nextDatum(int &ppm, long &timestamp) {
  //Gets the next datapoint
  uint16_t sent = SENT();
  ppm =       eeprom_read_word( PPM_AT( sent));
  timestamp = eeprom_read_dword(TIME_AT(sent));
  
  eeprom_write_word(SENT_PTR, sent+1);
  
  return;
}

void prevDataNotSent(int amt) {
  //Allows resending of data that has been read once
  eeprom_write_word(SENT_PTR, SENT()- amt);
  return;
}

//'deletes' all data from eeprom & reconfigures memory.
void clearData() {
  eeprom_write_word( SENT_PTR, 0);
  eeprom_write_word(SAVE_PTR, 0);
  eeprom_write_byte( MAGIC_NUM_LOC, MAGIC_NUM_VAL);
  setExperimentId(0);
}


boolean validMemory() {
  Serial.println((int) SENT_PTR); //remove later
  return eeprom_read_byte(MAGIC_NUM_LOC) == MAGIC_NUM_VAL;
}

int getExperimentId() {
  return (int) eeprom_read_word(EXPERIMENT_PTR);
}
void setExperimentId(int experiment_id) {
  eeprom_write_word(EXPERIMENT_PTR, (uint16_t) experiment_id);
}
