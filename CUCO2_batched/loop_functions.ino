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

/* Assembles a packet for uploading recorded data */
int assemblePacket(void) {
      //Assembling packet.
    int datalength = 0;
    char data[DATA_MAX_LENGTH] = "";
    strcat_P(data, PSTR("{\"sensor_datum\": "));
    int before_encryption = strlen(data);
    
    strcat_P(data, PSTR("{\"ppm\": {"));
    
    char ppm[10] = "";
    char timestamp[10] = "";
    
    int dataInPacket;
    for(dataInPacket=0; dataInPacket<PACKET_SIZE && hasMoreData(); dataInPacket++) {
      int tmp_ppm; long tmp_timestamp;
      nextDatum(tmp_ppm, tmp_timestamp);
      
      itoa(tmp_ppm,ppm,10);
      ltoa(tmp_timestamp,timestamp,10);
      
      strcat_P(data, PSTR("\""));
      strcat(data, timestamp);
      strcat_P(data, PSTR("\":\""));
      strcat(data, ppm);
      strcat_P(data, PSTR("\","));
    }
    
    //Removing trailing comma if there is data
    datalength = strlen(data); //Not its final value
    data[datalength-1] = dataInPacket ? '\0' : data[datalength-1];
    
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
    
    strcat_P(data, PSTR("\"},\"device_address\":\""));
    strcat(data, address);
    strcat_P(data, PSTR("\"}"));
    
    //Run encryption
    encrypt(data, vignere_key, data);

    Serial.print("Outgoing data: "); Serial.println(data);

    datalength = strlen(data);

    //Serial.print("Data length : ");
    //Serial.println(datalength);
//    Serial.println();
    wdt_reset();
   
    //Assembling header
    char putstr_buffer[64] = "POST /sensor_data/batch_create/";
    strcat(putstr_buffer,address);
    strcat_P(putstr_buffer, PSTR(".json HTTP/1.1"));

    int additionalCharacters = 17; // the brackets, :, and "s
    //Account for characters that will be escaped
    for(int i=0; i<datalength; i++) {
      if(data[i] == '\\' || data[i]=='"')
        additionalCharacters++;
    }

    makePacketHeader(putstr_buffer, "application/json", datalength + additionalCharacters);
    
    //Serial.print("Header: "); Serial.println(packet_buffer);
    
    //strcat(packet_buffer, data);
    strcat_P(packet_buffer, PSTR("\n{\"encrypted\":\""));
    
    //Copy encrypted text and escape " and \s
    int packetSize = strlen(packet_buffer);
    for(int i=0; i<datalength; i++) {
      if(data[i] == '"' || data[i] == '\\') {
        packet_buffer[packetSize] = '\\';
        packetSize++;
      }
      packet_buffer[packetSize] = data[i];
      packetSize++;
    }
    packet_buffer[packetSize] = '\0';
    
    strcat_P(packet_buffer, PSTR("\"}"));
    
    Serial.println(packet_buffer);


#ifdef INSTRUMENTED
  printTimeDiff(F("Packet assembled: "));
#endif
  return dataInPacket;
}

/* Contacts the server and sends a single packet of data collected. */
boolean sendPacket() {
  //Creates and sends a packet of data to the server containing CO2 results and timestamps
  Serial.println(F("Sending data..."));
  lcd_print_top("Sending data...");
  
  client = cc3000.connectTCP(ip, LISTEN_PORT);
  
#ifdef INSTRUMENTED
  printTimeDiff(F("TCP connection established: "));
#endif
    
    wdt_reset();
    if (client.connected()) {
      //Send packet
#ifdef INSTRUMENTED
  printTimeDiff(F("client.connected: "));
#endif

      wdt_reset();
      
      while(client.available()) { //flushing input buffer, just in case
        client.read();
      }
      
      client.fastrprintln(packet_buffer);

#ifdef INSTRUMENTED
      printTimeDiff(F("Packet sent: "));
      Serial.println("Outgoing request: ");
      Serial.println(packet_buffer);
      Serial.println();
 #endif

      Serial.println(F("Packet sent.\nWaiting for response."));
      wdt_reset();
      
      int timeLeft = 5000;
      char headerBuffer[7] = {0,0,0,0,0,0,0};
      while(timeLeft) {
        if(!strcmp("start\n", headerBuffer) && client.available()) {
          //When the header is over, and there is one character from the actual body
          break;
        }
        
        if(client.available()) {
          //Add the new character to the end of headerBuffer
          for(int i=0; i<5; i++) {
            headerBuffer[i] = headerBuffer[i+1];
          }
          headerBuffer[5] = client.read();
          //Serial.print(headerBuffer[5]);
        } else {
          delay(50);
          timeLeft -= 50;
        }
        
      } //End ignoring header
      
      wdt_reset();

      if(client.read() != 'S') {
        Serial.println(F("Upload failed"));
        //if uploading succeeded, the server will display a page that says "Success uploading data".
        // otherwise, it will show "Failed to upload"
        //On a timeout, client.read() gives -1.
        return false;
      } else {
        Serial.println(F("Upload succeeded"));
      }
 
 
      client.close();
      
#ifdef INSTRUMENTED
  printTimeDiff(F("client closed: "));
#endif
      delay(100); //SHOULD this be here?
      
      return true;
    } else {
      //Fail
      lcd_print_top("No connection");
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
  char data[1] = "";
  
#ifdef INSTRUMENTED
  printTimeDiff(F("TCP connection established: "));
#endif
  
  //Sending request
  char putstr_buffer[64] = "GET /first_contact/";
  strcat(putstr_buffer, address);
  strcat_P(putstr_buffer, PSTR(".html HTTP/1.1"));
  makePacketHeader(putstr_buffer, "application/json", datalength);
  
  strcat(packet_buffer, data);
  Serial.println(F("Sending request"));
  wdt_reset();
  
#ifdef INSTRUMENTED
  printTimeDiff(F("Request constructed: "));
#endif
  
  Serial.println(packet_buffer);
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
      i = (i == 0 && ( serverReply[0] == ' ' || serverReply[0] == '\n') ) ?  i : i+1;
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
  
  //Decoding server reply
  Serial.print("\nserverReply[0]: "); Serial.println((int)serverReply[0]);
  Serial.println("ServerReply:");
  Serial.println(serverReply);
  decrypt(serverReply, vignere_key, serverReply);
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
    CO2_cutoff = DEFAULT_CO2_CUTOFF;
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
char *makePacketHeader(char *request_type_and_location, char *mime_type, int datalength) {
  char len_buffer[32] = "";
  itoa(datalength, len_buffer, 10);
  packet_buffer[0] = '\0';
  strcat(packet_buffer, request_type_and_location);
  strcat_P(packet_buffer, PSTR("\nHost: " HOST "\nContent-Type: "));
  strcat(packet_buffer, mime_type);  
  strcat_P(packet_buffer, PSTR("; charset=UTF-8\nContent-Length: "));  
  strcat(packet_buffer, len_buffer);
  strcat_P(packet_buffer, PSTR("\nConnection: close\n")); 
  
  return packet_buffer;
  
}

void experimentCleanup() {
  justStarted = true;
  clearData();
}

//////////////////////////////
//// LCD display functions ///
//////////////////////////////
void lcd_print_top(char* string) {
  lcd.clear();
  lcd.print(string);
}

void lcd_print_bottom(char* string) {
  lcd.setCursor(0,1);
  lcd.print(string);
}

//////////////////////////////
//// CO2 sensor functions ////
//////////////////////////////

boolean sendRequest(byte packet[]) {
  int time = 0;
    while (!K_30_Serial.available()) //keep sending request until we start to get a response
    {        
        wdt_reset();
        if (time > 5000) //if it takes too long there was probably an error
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
            Serial.println(F("Could not get response from sensor!"));
            while (K_30_Serial.available()) { //flush whatever we have
                K_30_Serial.read();
            }
            return false;
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
    uint8_t high = packet[3]; //high byte for value is 4th byte in packet in the packet
    uint8_t low = packet[4]; //low byte for value is 5th byte in the packet
    uint16_t val = high;
    val <<= 8;
    val |= low;
    return (unsigned long) val * valMultiplier;
}
