
//Returns whether the user has requested smart config
boolean attemptSmartConfig(void) {
  int time = SMARTCONFIG_WAIT;
  while (time > 0) {
    //wait for 7 seconds for user to select smartconfig
    if(!(time % 100)) {
      Serial.print(F("Push button for Smart Config. "));
      Serial.print((time / 1000));
      Serial.print('.');
      Serial.println((time % 1000) / 100);
    }
    
    if (!digitalRead(BUTTON)) {
      return true;
    }
    else {
     // Serial.println(F("Waiting for smartconfig"));
      time -= 100;
      delay(100);
    }
  }
  
  return false;
}

bool displayConnectionDetails(void) {
  uint32_t addr, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&addr, &netmask, &gateway, &dhcpserv, &dnsserv))
    return false;

  Serial.print(F("IP Addr: ")); cc3000.printIPdotsRev(addr);
  Serial.print(F("\r\nNetmask: ")); cc3000.printIPdotsRev(netmask);
  Serial.print(F("\r\nGateway: ")); cc3000.printIPdotsRev(gateway);
  Serial.print(F("\r\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
  Serial.print(F("\r\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
  Serial.println();
  return true;
}


boolean attemptSmartConfigReconnect(void){
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!! Note the additional arguments in .begin that tell the   !!! */
  /* !!! app NOT to deleted previously stored connection details !!! */
  /* !!! and reconnected using the connection details in memory! !!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */  
  if (!cc3000.begin(false, true))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    return false;
  }

  /* Round of applause! */
  Serial.println(F("Reconnected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  int time = 0;
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
    time += 100;
    wdt_reset();
    if (time > 10000) {
      Serial.println(F("DHCP failed!"));
      return false;
    }
  }  
  return true;
}

boolean attemptSmartConfigCreate(void){
  /* Initialise the module, deleting any existing profile data (which */
  /* is the default behaviour)  */
  wdt_reset();
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin(false))
  {
    return false;
  }  
  wdt_reset();
  wdt_disable();
  /* Try to use the smart config app (no AES encryption), saving */
  /* the connection details if we succeed */
  Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
  if (!cc3000.startSmartConfig(false))
  {
    Serial.println(F("SmartConfig failed"));
    return false;
  }
  wdt_enable(WDTO_8S);
  wdt_reset();
  Serial.println(F("Saved connection details and connected to AP!"));
  
  int time = 0;
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) 
  {
    delay(100); // ToDo: Insert a DHCP timeout!
    time += 100;
    wdt_reset();
    if (time > 20000) {
      time = 0;
      Serial.println(F("DHCP failed!"));
      return false;
    }
  }  
  
  return true;
}


/////////////////////
///// Printing //////
/////////////////////

void mactoa(uint8_t ip[], char *string) {
  //Mac address to ascii
  for(int i=0; i < 6; i++) {
    string[i*2] = to_hex(ip[i]>>4);
    string[i*2+1] = to_hex(ip[i]);
  }
  string[12] = '\0';
}
char to_hex(uint8_t h) {
  h &= 0xF;
  if (h >= 10) {
    return (h - 10 + 'A');
  } else {
    return (h + '0');
  }
}


