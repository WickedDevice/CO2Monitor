#include <WildFire_CC3000.h>
int mostRecentDataAvg(int numToAverage);


long int experimentSeconds() {
  return (millis()-millisOffset)/1000L + experimentStart;
}


boolean justStarted = true;
boolean experimentEnded() {
  //Logic for deciding whether an experiment has ended goes here...
  
  int newAverage = mostRecentDataAvg(5);
  
  justStarted = CO2_cutoff <= newAverage ? false : justStarted;
  
  if(CO2_cutoff > newAverage && !justStarted) {
    return true;
  } else {
    return false;
  }
}


boolean sendPacket(void) {
  //Creates and sends a packet of data to the server containing CO2 results and timestamps
  Serial.println(F("Sending data..."));
  
  client = cc3000.connectTCP(ip, LISTEN_PORT);

    //Assembling packet.
    int datalength = 0;
    char data[DATA_MAX_LENGTH] = "\n";
    strcat_P(data, PSTR("{\"sensor_datum\": {\"ppm\": {"));
    
    char ppm[10] = "";
    char timestamp[10] = "";
    
    int dataInPacket;
    for(dataInPacket=0; dataInPacket<PACKET_SIZE && hasMoreData(); dataInPacket++) {
      long tmp_ppm, tmp_timestamp;
      nextDatum(tmp_ppm, tmp_timestamp);
      
      ltoa(tmp_ppm,ppm,10);
      ltoa(tmp_timestamp,timestamp,10);
      
      strcat_P(data, PSTR("\""));
      strcat(data, timestamp);
      strcat_P(data, PSTR("\":\""));
      strcat(data, ppm);
      strcat_P(data, PSTR("\","));
    }
    data[strlen(data)-1] = '\0'; //Removing trailing comma
          //(will fail on packets with no data, which shouldn't be a problem)
    
    strcat_P(data, PSTR("}, \"device_address\": \""));
    strcat(data, address);
    
    strcat_P(data, PSTR("\", \"experiment_id\": \""));
    char experiment_string[10];
    itoa(experiment_id, experiment_string, 10);
    strcat(data,experiment_string);
    
    strcat_P(data, PSTR("\", \"experiment_ended\": \""));
    if(experimentEnded()) {
      strcat_P(data, PSTR("true"));
    } else {
      strcat_P(data, PSTR("false"));
    }
    
    strcat_P(data,PSTR("\"}}"));

    //Serial.print("Outgoing data: "); Serial.println(data);

    datalength = strlen(data);

    //Serial.print("Data length : ");
    //Serial.println(datalength);
//    Serial.println();
    wdt_reset();
   
    //Assembling header
    char putstr_buffer[64] = "POST /sensor_data/batch_create/";
    strcat(putstr_buffer,address);
    strcat_P(putstr_buffer, PSTR(".json HTTP/1.1"));

    makePacketHeader(putstr_buffer, datalength);
    strcat(packet_buffer, data);
    
    
    wdt_reset();
    if (client.connected()) {
      //Send packet
      
//      Serial.println("Client connected!");

      wdt_reset();
      client.println(packet_buffer);
      
      Serial.println("Outgoing request: ");
      Serial.println(packet_buffer);
      Serial.println();
      client.close();
      
      delay(100); //SHOULD this be here?
      
      return true;
    } else {
      //Otherwise, retry with the same data.
      prevDataNotSent(dataInPacket);
        
      Serial.println(F("Connection failed"));
      return false;
    }
}

void checkForExperiment(int &experiment_id, int &CO2_cutoff) {
  Serial.println(F("Connecting to server...\nIf this is the first time, it may take a while"));
  wdt_reset();
  client = cc3000.connectTCP(ip, LISTEN_PORT);
  Serial.println(F("Connected"));
  int datalength = 0;
  char data[512] = "";
  
  //Sending request
  char putstr_buffer[64] = "GET /first_contact/";
  strcat(putstr_buffer, address);
  strcat_P(putstr_buffer, PSTR(".html HTTP/1.1"));
  makePacketHeader(putstr_buffer, datalength);
  
  strcat(packet_buffer, data);
  Serial.println(F("Sending request"));
  wdt_reset();
  client.println(packet_buffer);
  
  ///Receiving reply
  wdt_reset();
  
  char serverReply[512] = "";
  
  Serial.println(F("Getting Server reply"));
  
  //Ignoring the header:
  int i = 0;
  while(client.connected() && i < 511) {
    while(client.available()) {
      Serial.print('.');
      wdt_reset();
      serverReply[i] = (char)client.read();
      i++;
      serverReply[i] = '\0';
      if(i >= 5 && !strcmp("start", serverReply+i-5)) {
        break;
      }
    }
    
    
    if(i >= 5 && !strcmp("start", serverReply+i-5)) {
      break;
    }
  }
  Serial.print(' ');
  //Reading the body
  i=0;
  while(client.connected() && i < 511){
    Serial.print('.');
    if(client.available()) {
      wdt_reset();
      serverReply[i] = (char)client.read();
      i++;
    } else {
      //delay(50);
    }
    
    if(i >= 3 && !strcmp("end", serverReply+i-3)) {
      i -= 3;
      break;
    }
  }
  serverReply[i] = '\0';
  
  Serial.println("\nPacket to server:");
  Serial.println(packet_buffer);
  Serial.println("ServerReply:");
  Serial.println(serverReply);
  
  
  //Extracting information from the reply
  long int time;
  int experiment_id_tmp, CO2_cutoff_tmp;
  int varsRead = sscanf(serverReply, "%ld %*s %d %*s %d", &time, &experiment_id_tmp, &CO2_cutoff_tmp);
  if(varsRead >= 1) {
    millisOffset = millis();
    experimentStart = time;
  }
  
  if(varsRead >= 2) {
    experiment_id = experiment_id_tmp;
  }

  if(varsRead >= 3) {
    CO2_cutoff = CO2_cutoff_tmp;
  } else {
    CO2_cutoff = 2000;
  }


  Serial.print("Time: ");Serial.println(time);
  Serial.print("Experiment id: "); Serial.println(experiment_id_tmp);
  Serial.print("CO2 cutoff: "); Serial.println(CO2_cutoff_tmp);
  
  client.close();
  
  delay(100); //SHOULD this be here?
  
  
  return;
}

char *makePacketHeader(char *request_type_and_location, int datalength) {
  char len_buffer[32] = "";
  itoa(datalength, len_buffer, 10);
  packet_buffer[0] = '\0';
  strcat(packet_buffer, request_type_and_location);
  strcat_P(packet_buffer, PSTR("\nHost: " HOST "\nContent-Type: application/json; charset=UTF-8\nContent-Length: "));  
  strcat(packet_buffer, len_buffer);
  strcat_P(packet_buffer, PSTR("\nConnection: close\n")); 
  
  return packet_buffer;
  
}

/////////////////////////////////
////// Memory Management ////////
/////////////////////////////////

// This is using a very simple version of memory management, just using an array
// In the future, it will probably save to EEPROM
#define SAVE_SPACE 500
int savePtr = 0;
int sentPtr = 0;
long int savedData[SAVE_SPACE*2];
boolean saveDatum(unsigned long valCO2) {
  //Saves data to array, eeprom, or memory
  //right now just prints it out
  if(outOfSpace()) {
    return false;
  }
  
  savedData[savePtr*2] = valCO2;
  savedData[savePtr*2+1] = experimentSeconds();
  savePtr++;
  Serial.print(F("Recording data... CO2 ppm is: "));
  Serial.println(valCO2);
  return true;
}

boolean outOfSpace(void) {
  return savePtr >= SAVE_SPACE;
}

boolean hasMoreData(void) {
  //Determines whether there are more data to send
  return (savePtr > sentPtr ? true : false);
}


int mostRecentDataAvg(int numToAverage = 5) {
  numToAverage = numToAverage > savePtr ? savePtr : numToAverage;
  int sum = 0;
  for(int i = 0; i < numToAverage; i++) {
    sum += savedData[(savePtr-i)*2];
  }
  
  return sum / numToAverage;
}

void nextDatum(long &ppm, long &timestamp) {
  //Returns a ppm, timestamp tuple
  ppm = savedData[sentPtr*2];
  timestamp = savedData[sentPtr*2+1];
  sentPtr++;
  return;
}

void prevDataNotSent(int amt) {
  sentPtr -= amt;
  return;
}

//////////////////////////////
//// CO2 sensor functions ////
//////////////////////////////

boolean sendRequest(byte packet[]) {
  int time = 0;
    while (!K_30_Serial.available()) //keep sending request until we start to get a response
    {        
        wdt_reset();
        if (time > 5000) //if it takes to long there was probably an error
                {
                Serial.println("Could not send request to sensor!");
                return false;
            }
        K_30_Serial.write(readCO2, 7);
        time += 50;
        delay(50);
    }

    time = 0; //set a timeoute counter
    while (K_30_Serial.available() < 7) //Wait to get a 7 byte response
    {
        
        wdt_reset();
        if (time > 5000) //if it takes to long there was probably an error
                {
            while (K_30_Serial.available()) { //flush whatever we have
                K_30_Serial.read();
                Serial.println("Could not get response from sensor!");
                return false;
            }
        }
        time += 50;
        delay(50);
    }
    for (int i = 0; i < 7; i++) {
        wdt_reset();
        response[i] = K_30_Serial.read();
    }
    return true;
}

unsigned long getValue(byte packet[]) {
    int high = packet[3]; //high byte for value is 4th byte in packet in the packet
    int low = packet[4]; //low byte for value is 5th byte in the packet
    unsigned long val = high * 256 + low; //Combine high byte and low byte with this formula to get value
    return val * valMultiplier;
}
