#include <WildFire_CC3000.h>
int mostRecentDataAvg(int numToAverage);


#ifdef INSTRUMENTED
int millisSinceLast = 0; //Number of milliseconds since the last time printTimeDiff was called
/* Prints the number of milliseconds that went by between this and the last time it was called */
inline int printTimeDiff(const __FlashStringHelper *label) {
  Serial.print('\t');
  Serial.print(label);
  Serial.println(millis() - millisSinceLast);
  millisSinceLast = millis();
}
#endif

/* The number of seconds between now and the start of the experiment */
long int experimentSeconds() {
  return (millis()-millisOffset)/1000L + experimentStart;
}


boolean justStarted = true; //Whether, during the experiment, the CO2 ppm has risen above the CO2_cutoff threshold
/* Returns whether the experiment has ended or not
    Currently determines this by checking to see if the CO2 ppm has risen past a threshold and fallen back down below it.
 */
boolean experimentEnded() {
  
  int newAverage = mostRecentDataAvg(5);
  
  justStarted = (CO2_cutoff <= newAverage) ? false : justStarted;
  
  if(CO2_cutoff > newAverage && !justStarted) {
    return true;
  } else {
    return false;
  }
}


/* Contacts the server and sends a single packet of data collected. */
boolean sendPacket(void) {
  //Creates and sends a packet of data to the server containing CO2 results and timestamps
  Serial.println(F("Sending data..."));
  
  client = cc3000.connectTCP(ip, LISTEN_PORT);
  
#ifdef INSTRUMENTED
  printTimeDiff(F("TCP connection established: "));
#endif

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

#ifdef INSTRUMENTED
  printTimeDiff(F("Packet assembled: "));
#endif
    
    wdt_reset();
    if (client.connected()) {
      //Send packet
#ifdef INSTRUMENTED
  printTimeDiff(F("client.connected: "));
#endif

      wdt_reset();
      client.fastrprintln(packet_buffer);

#ifdef INSTRUMENTED
      printTimeDiff(F("Packet sent: "));
      Serial.println("Outgoing request: ");
      Serial.println(packet_buffer);
      Serial.println();
 #endif
 
      client.close();
      
#ifdef INSTRUMENTED
  printTimeDiff(F("client closed: "));
#endif
      delay(100); //SHOULD this be here?
      
      return true;
    } else {
      //Otherwise, retry with the same data.
      prevDataNotSent(dataInPacket);
        
      Serial.println(F("Connection failed"));
      return false;
    }
}

/* Contacts the server, checks to see if there is an experiment, */
void checkForExperiment(int &experiment_id, int &CO2_cutoff) {
  Serial.println(F("Connecting to server...\nIf this is the first time, it may take a while"));
  wdt_reset();
  client = cc3000.connectTCP(ip, LISTEN_PORT);
  Serial.println(F("Connected"));
  int datalength = 0;
  char data[512] = "";
  
#ifdef INSTRUMENTED
  printTimeDiff(F("TCP connection established: "));
#endif
  
  //Sending request
  char putstr_buffer[64] = "GET /first_contact/";
  strcat(putstr_buffer, address);
  strcat_P(putstr_buffer, PSTR(".html HTTP/1.1"));
  makePacketHeader(putstr_buffer, datalength);
  
  strcat(packet_buffer, data);
  Serial.println(F("Sending request"));
  wdt_reset();
  
#ifdef INSTRUMENTED
  printTimeDiff(F("Request constructed: "));
#endif
  
  client.fastrprintln(packet_buffer);

#ifdef INSTRUMENTED
  printTimeDiff(F("Request sent: "));
#endif

  ///Receiving reply
  wdt_reset();
  
  char serverReply[512] = "";
  
  Serial.println(F("Getting Server reply"));
  
  //Ignoring the header:
  int i = 0;
  while(client.connected() && i < 511) {
    while(client.available()) {
      Serial.print('*');
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

#ifdef INSTRUMENTED
  printTimeDiff(F("\nHeader received: "));
#endif

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

#ifdef INSTRUMENTED
  printTimeDiff(F("Body receivced: "));
  Serial.println("\nPacket to server:");
  Serial.println(packet_buffer);
  Serial.println("ServerReply:");
  Serial.println(serverReply);
#endif
  
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

#ifdef INSTRUMENTED
  printTimeDiff(F("Body parsed: "));
#endif

  Serial.print("Time: ");Serial.println(time);
  Serial.print("Experiment id: "); Serial.println(experiment_id);
  Serial.print("CO2 cutoff: "); Serial.println(CO2_cutoff);
  
  client.close();

#ifdef INSTRUMENTED
  printTimeDiff(F("client closed: "));
#endif

  delay(100); //SHOULD this be here?
  
  
  return;
}

/* Constructs a packet header at the start of packet_buffer (which it returns) */
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

void experimentCleanup() {
  justStarted = true;
  clearData();
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
