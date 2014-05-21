
/***************************************************
     Demo version of CUCO2 monitor code
     Currently treats device as always active
     Continuously posts to server without waiting for response
 ****************************************************/
#include <WildFire_CC3000.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <WildFire.h>
WildFire wf(WILDFIRE_V2);

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

WildFire_CC3000 cc3000(WILDFIRE_V2); // you can change this clock speed

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           80    // What TCP port to listen on for connections.

boolean attemptSmartConfigReconnect(void);
boolean attemptSmartConfigCreate(void);

#define K_30_Serial Serial1

int time = 0;
int sm_button = 5; //DIG5? this is for the smartconfig button
int dev_status = 0; //0 is not selected, 1 is selected
int exp_status = 0; //0 is not running, 1 is running

uint32_t ip;
WildFire_CC3000_Client client;

long value = 1;

byte readCO2[] = { 0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25 }; //Command packet to read Co2 (see app note)
byte response[] = { 0, 0, 0, 0, 0, 0, 0 }; //create an array to store the response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;

void setup(void)
{  
 
  wf.begin();
  pinMode(sm_button, INPUT_PULLUP);
  
  Serial.begin(115200);
  K_30_Serial.begin(9600); //is this value correct?
  Serial.println(F("Welcome to WildFire!\n")); 
  
  while (time < 7000) {
    //wait for 10 seconds for user to select smartconfig 
    if (!digitalRead(sm_button)) {
      while(!attemptSmartConfigCreate()){
        Serial.println(F("Waiting for Smart Config Create"));
      }
      break;
    }
    else {
     // Serial.println(F("Waiting for smartconfig"));
      time += 100;
      delay(100);
    }
  }
  if (time >= 7000) {  
    Serial.println(F("Attempting to reconnect"));
    //after 10 seconds with no button press detected, attempt reconnect
    wdt_reset();
    if(!attemptSmartConfigReconnect()){
      Serial.println(F("Reconnect failed!"));
      cc3000.disconnect();
      soft_reset();
    }
  }
  time = 0;
  
  while(!displayConnectionDetails());
  
  // Get the website IP & print it
  ip = 0;
  // 162.243.18.121
  ip = 121;
  ip |= (((uint32_t) 18) << 8);
  ip |= (((uint32_t) 243) << 16);
  ip |= (((uint32_t) 162) << 24);

   wdt_reset();   
  cc3000.printIPdotsRev(ip);
  Serial1.begin(9600);
}
char packet_buffer[2048];
#define BUFSIZE 511
void loop(void)
{
  //poll server for device status
  //For demo: device is selected and experiment is running
  //Need to add if statement to take this into account
  dev_status = 1;
  exp_status = 1;
  
//    Serial.print("Begin: ");
//    Serial.println(millis());

//    check_mem();
    wdt_reset();    
    client = cc3000.connectTCP(ip, LISTEN_PORT);
    
//    Serial.print("After ConnectTCP: ");
//    Serial.println(millis());

    if (!sendRequest(readCO2)) {
        wdt_disable();
        Serial.println("Check sensor connection");
        while (1);
    }
    unsigned long valCO2 = getValue(response);
//    Serial.print("CO2 reading is : "); Serial.println(valCO2);
    char val[10] = "";
    itoa(valCO2, val, 10);
//    Serial.print("CO2 reading as string : "); Serial.println(val);
    int datalength = 0;
    #define DATA_MAX_LENGTH 512
    char data[DATA_MAX_LENGTH] = "\n";
    strcat_P(data, PSTR("{\"sensor_datum\": {\"ppm\": \"")); // val "\", \"device_address\": \"42\"}}"));
    strcat(data, val);
    strcat_P(data, PSTR("\", \"device_address\": \"42\"}}"));
//    Serial.print("Outgoing data: "); Serial.println(data);
    datalength = strlen(data);
//    Serial.print("Data length : ");
//    Serial.println(datalength);
//    Serial.println();
    wdt_reset();    
    // Send request
   
    char putstr_buffer[64] = "POST /sensor_data";
    strcat_P(putstr_buffer, PSTR(".json HTTP/1.1"));
   
    char contentlen_buffer[64] = "Content-Length: ";
    char len_buffer[32] = "";
    itoa(datalength, len_buffer, 10);
    strcat(contentlen_buffer, len_buffer);
   
//    Serial.print("Before if(client.connected()): ");
//    Serial.println(millis());  

    wdt_reset();    
    if (client.connected()) {
//      Serial.println("Client connected!");
      packet_buffer[0] = '\0';    
      strcat(packet_buffer, putstr_buffer);
      strcat_P(packet_buffer, PSTR("\n")); 
      strcat_P(packet_buffer, PSTR("Host: 162.243.18.121\n"));
      strcat_P(packet_buffer, PSTR("Content-Type: application/json; charset=UTF-8\n"));  
      strcat(packet_buffer, contentlen_buffer);
      strcat_P(packet_buffer, PSTR("\n"));  
      strcat_P(packet_buffer, PSTR("Connection: close\n")); 
      
      strcat(packet_buffer, data);

      wdt_reset();      
      client.println(packet_buffer); 
//     Serial.println("Outgoing request: ");
//    Serial.println(packet_buffer);     
//    Serial.println();
    } else {
      Serial.println(F("Connection failed"));    
      return;
    }
    
//    Serial.print("After if(client.connected()): ");
//    Serial.println(millis());
    
    Serial.println(F("-------------------------------------"));
    wdt_reset();    
    while (client.connected()) {
      wdt_reset();      
      while (client.available()) {     
        wdt_reset();        
        char c = client.read();
        Serial.print(c);
      }
    }
  
//    Serial.print("After client.read() loop: ");
//    Serial.println(millis());

    wdt_reset();  
    client.close();
    delay(200);
  
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
  time = 0;
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
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin(false))
  {
    return false;
  }  
  
  /* Try to use the smart config app (no AES encryption), saving */
  /* the connection details if we succeed */
  Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
  if (!cc3000.startSmartConfig(false))
  {
    Serial.println(F("SmartConfig failed"));
    return false;
  }
  
  Serial.println(F("Saved connection details and connected to AP!"));
  
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

void _reset(void) {
   Serial.println(F("Resetting in 10 seconds..."));
   delay(5000);
   Serial.println(F("Resetting in 5 seconds..."));
   delay(5000);
   Serial.println(F("Resetting now..."));
   soft_reset();
}

uint8_t * heapptr, * stackptr;
void check_mem() {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
  Serial.println();
  Serial.print("Stack pointer:");
  Serial.println((int)stackptr);
  Serial.print("Heap pointer:");
  Serial.println((int)heapptr);
}

boolean sendRequest(byte packet[]) {
  time = 0;
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
