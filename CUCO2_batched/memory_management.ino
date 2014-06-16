/////////////////////////////////
////// Memory Management ////////
/////////////////////////////////

// This is using a very simple version of memory management, just using an array
// In the future, it will probably save to EEPROM
#define SAVE_SPACE 500
int savePtr = 0;
int sentPtr = 0;
long int savedData[SAVE_SPACE*2];


//#define encryption_key_loc
#define savePtr_loc 0
#define sentPtr_loc (savePtr_loc + sizeof(void *))

#define saveSpaceStart (sentPtr_loc + sizeof(void *))
#define saveSpaceEnd (saveSpaceStart + (SAVE_SPACE * sizeof(long int)))

boolean saveDatum(unsigned long valCO2) {
  //Saves data to array, eeprom, or memory
  if(outOfSpace()) {
    return false;
  }
  
  savedData[savePtr*2] = valCO2;
  savedData[savePtr*2+1] = experimentSeconds();
  savePtr++;
  return true;
}

boolean outOfSpace(void) {
  if(savePtr >= SAVE_SPACE) {
    Serial.println(F("Out of space"));
  }
  return savePtr >= SAVE_SPACE;
}

boolean hasMoreData(void) {
  //Determines whether there are more data to send
  return (savePtr > sentPtr ? true : false);
}


int mostRecentDataAvg(int numToAverage = 5) {
  numToAverage = (numToAverage > savePtr) ? savePtr : numToAverage;
  unsigned long sum = 0;
  for(int i = 1; i <= numToAverage; i++) {
    sum += savedData[(savePtr-i)*2];
  }
  
  return sum / numToAverage;
}

void nextDatum(long &ppm, long &timestamp) {
  //Gets the next datapoint
  ppm = savedData[sentPtr*2];
  timestamp = savedData[sentPtr*2+1];
  sentPtr++;
  return;
}

void prevDataNotSent(int amt) {
  //Allows resending of data that has been read once
  sentPtr -= amt;
  return;
}

void clearData() {
  savePtr = 0;
  sentPtr = 0;
}

