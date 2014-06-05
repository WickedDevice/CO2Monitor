
WildFire wf(WILDFIRE_V2);

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//IP address of the server:
#define HOST "107.170.187.156"
#define IP_0 107
#define IP_1 170
#define IP_2 187
#define IP_3 156

/*
#define HOST "162.243.18.121"
#define IP_0 162
#define IP_1 243
#define IP_2 18
#define IP_3 121
*/
#define LISTEN_PORT           80    // What TCP port to listen on for connections.

#define K_30_Serial Serial1

#define PACKET_SIZE 50                          //Number of datapoints in a packet
                                                   // Don't make this too big or the Wildfire won't have space for it
#define DATA_MAX_LENGTH (PACKET_SIZE * 35 + 150) //Maximum length of the data string to submit in the packet
                                                   // <35 for each datapoint, 150 for metadata
char packet_buffer[160 + DATA_MAX_LENGTH + 64];  //Array that holds the packet
                                                   // ~160 for header, ~64 for extra space


#define SMARTCONFIG_WAIT 1000       //Time to wait before attempting to reconnect (milliseconds)

#define BUTTON 5                    //Pin that the button is attached to
                                    //Note: Button is pushed when low

WildFire_CC3000 cc3000(WILDFIRE_V2); // you can change this clock speed
