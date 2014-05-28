
WildFire wf(WILDFIRE_V2);

char packet_buffer[2048];
#define BUFSIZE 511

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//IP address of the server: 192.168.1.5
#define HOST "192.168.1.5"
#define IP_0 192
#define IP_1 168
#define IP_2 1
#define IP_3 5
#define LISTEN_PORT           3000    // What TCP port to listen on for connections.

#define K_30_Serial Serial1

#define DATA_MAX_LENGTH 512          //Maximum length of the data string to submit in the packet
#define PACKET_SIZE 3                //Number of datapoints in a packet

#define SMARTCONFIG_WAIT 1000       //Time to wait before attempting to reconnect

#define BUTTON 5                    //Pin that the button is attached to

WildFire_CC3000 cc3000(WILDFIRE_V2); // you can change this clock speed
