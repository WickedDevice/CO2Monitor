
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
// if the user has a serial and types something, it will jump to 
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
    
    if (BUTTON_PUSHED) {
      return true;
    }
    else {
     // Serial.println(F("Waiting for smartconfig"));
      time -= 100;
      delay(100);
      
      if(Serial.available()) {
        configure(); //One-time setup for encryption key
      }
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
    delay(100);
    time += 100;
    wdt_reset();
    if(BUTTON_PUSHED) {
      return false;
    } if (time > 10000) {
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

#include <utility/nvmem.h>
#include <utility/wlan.h>
#include <utility/hci.h>
/* Prompt for one-time configuration of WildFire */
void configure() {
  lcd_print_top("Configuring");
  //Ignore the first bit of typing
  while(Serial.available()) {
    Serial.read();
  }
  
  
  //////////Begin get mac address
  
  //Does what it needs to to initialize the cc3000 enough
  // to read the MAC address
  //    We don't want to just call cc3000.begin, because that would attempt to connect to a network.
  Serial.print(F("Finding mac address ."));
  init_spi();
  Serial.print('.');
  wlan_init(CC3000_UsynchCallback,
          sendWLFWPatch, sendDriverPatch, sendBootLoaderPatch,
          ReadWlanInterruptPin,
          WlanInterruptEnable,
          WlanInterruptDisable,
          WriteWlanPin);
  Serial.print('.');
  wlan_start(0);
  Serial.println('.');
  
  wdt_reset();
  uint8_t mac[6] = "";
  
  if(nvmem_get_mac_address(mac)) {
    Serial.println(F("Error: unable to get mac address"));
  } else {
    mactoa(mac,address);
    Serial.println(F("MAC address:"));
    Serial.println(address);
  }
  /////////End get mac address
  
  Serial.println(F("Configuration started.\nPlease make sure you are using a serial mode with newlines."));
  
  if(validEncryptionKey()){
    Serial.println(F("\nOld encryption key found:"));
    char buffer[32] = "";
    getEncryptionKey(buffer);
    Serial.println(buffer);
    Serial.println(F("Overwrite? y/n"));
  } else {
    Serial.println(F("\nEncryption key not found, make a new one? y/n"));
  }
  wdt_reset(); //I would disable the watch dog timer, but I don't want to be stuck here by accident
  
  while(!Serial.available()) { delay(100);}
  
  wdt_reset();
  char yesNo = Serial.read();
  Serial.read(); //Get rid of newline
  if(yesNo == 'Y' || yesNo == 'y') {
    wdt_reset();
    setEncryptionKeyBySerial();
  }
  
  Serial.println(F("Configuration finished, restarting."));
  soft_reset();  
}

/* Serial interface for setting the encryption key */
void setEncryptionKeyBySerial() {
  Serial.println(F("\nPlease type in new encryption key. (<32 characters)"));
  
  boolean done = false;
  char buffer[32] = "";
  int index = 0;
  char c = ' ';  //' ' is an arbitrary value
  
  wdt_disable();
  
  while(c != '\n' && c != '\0') { //Until newline
    while(Serial.available()) {
      c = Serial.read();
      if(c == '\n') {
        buffer[index] = '\0';
        break;
      }
      buffer[index] = c;
      index++;
    }
  } //end while for Serial reading
  
  Serial.println(F("Your new encryption key is:"));
  Serial.println(buffer);
  Serial.println(F("Is this OK? y/n"));
  while(!Serial.available()){}
  c = Serial.read();
  Serial.read(); //Getting rid of newline
  if(c == 'Y' || c == 'y') {
    wdt_enable(WDT_WAIT);
    setEncryptionKey(buffer);
    Serial.println(F("Key saved."));
  } else {
    while(Serial.available())
    { Serial.read(); }
    wdt_enable(WDT_WAIT);
    setEncryptionKeyBySerial();
  }
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


