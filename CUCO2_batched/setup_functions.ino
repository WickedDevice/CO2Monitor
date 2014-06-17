
#ifdef INSTRUMENTED
int millisSinceLastSmaller = 0;

inline void resetSmallTimeDiff(void) {
  millisSinceLastSmaller = millis();
}

/* prints out the number of milliseconds between now and when printTimeDiffSmall was last called 
  Similar to printTimeDiff. 
*/
inline int printTimeDiffSmall(const __FlashStringHelper *label) {
  Serial.print("\t\t");
  Serial.print(label);
  Serial.println(millis() - millisSinceLastSmaller);
  resetSmallTimeDiff();
}

#endif

//Returns whether the user has requested smart config
boolean attemptSmartConfig(void) {
  lcd_print_top("Push button for");
  lcd_print_bottom("Smart Config. . ");
  int time = SMARTCONFIG_WAIT;
  while (time > 0) {
    //wait for a few seconds for user to select smartconfig
    if(!(time % 100)) {
      lcd.setCursor(13,1);
      lcd.print(time / 1000);
      lcd.setCursor(15,1);
      lcd.print((time % 1000) / 100);
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
  lcd_print_top("Reconnecting");
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
#ifdef INSTRUMENTED
  resetSmallTimeDiff();
#endif
  Serial.println(F("Warming up CC3000"));
  
  if (!cc3000.begin(false, true))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    return false;
  }
#ifdef INSTRUMENTED
  printTimeDiffSmall(F("Reconnected: "));
#endif

  /* Round of applause! */
  lcd_print_top("Reconnected");
  Serial.println(F("Reconnected!"));
  
  /* Wait for DHCP to complete */
  lcd_print_bottom("Requesting DHCP");
  Serial.println(F("\nRequesting DHCP"));
  int time = 0;
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
    time += 100;
    wdt_reset();
    if (time > 10000) {
      Serial.println(F("DHCP failed!"));
      lcd_print_bottom("DHCP failed");
      return false;
    }
  }
  
#ifdef INSTRUMENTED
  printTimeDiffSmall(F("DHCP: "));
#endif

  return true;
}

boolean attemptSmartConfigCreate(void){
  /* Initialise the module, deleting any existing profile data (which */
  /* is the default behaviour)  */
  wdt_reset();
  Serial.println(F("\nInitialising the CC3000 ..."));
  
  if (!cc3000.begin(false))
  {
    Serial.println("Wifi chip failed");
    return false;
  }
  wdt_reset();
  wdt_disable();
  /* Try to use the smart config app (no AES encryption), saving */
  /* the connection details if we succeed */
  Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
  
  lcd_print_top("Waiting for"); lcd_print_bottom("SmartConfig(60s)");
  
  if (!cc3000.startSmartConfig(false))
  {
    lcd.clear(); lcd_print_top("SmartConfig"); lcd_print_bottom("failed");
    
    Serial.println(F("SmartConfig failed"));
    return false;
  }
  wdt_enable(WDTO_8S);
  wdt_reset();
  Serial.println(F("Saved connection details and connected to AP!"));
  lcd.clear(); lcd_print_top("Connected"); 
  
  int time = 0;
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP")); lcd_print_bottom("Requesting DHCP");
  while (!cc3000.checkDHCP())
  {
    delay(100);
    time += 100;
    wdt_reset();
    if (time > 20000) {
      time = 0;
      Serial.println(F("DHCP failed!"));
      lcd.clear();lcd_print_top("DHCP failed");
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


