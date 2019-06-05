#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <tr064.h>
#include <ESP8266TelegramBOT.h>
#include <PubSubClient.h>

#define mqtt_server "<ip>"
#define mqtt_user "<user>"
#define mqtt_password "<password>"
#define klingel_topic "fritzbox/klingel"

extern "C" {
#include "user_interface.h" //os_timer
}

//////////////////////////  MQTT Config  ///////////////////////////////////////////
//If you have a MQTT-Server set value to 1, otherwise
#define mqttenabled 1
//enter MQTT Server-IP
IPAddress mqttserver(<ip like 192, 168, 10, 1);

#if mqttenabled == 1
WiFiClient espClient;
PubSubClient client(espClient);
#endif
 
//////////////////////////  Fritzbox Config  ///////////////////////////////////////
//Username of your Fritzbox. If you didn'T change default user, then it's "admin".
const char* fuser = "<user>";
//Password for Fritzbox
const char* fpass = "<password>";
/IP-Address of your Fritzbox. Default is 192.168.178.1.
const char* IP = "<ip>";
const int PORT = 49000;
TR064 connection(PORT, IP, fuser, fpass);

//////////////////////////  Telegram Config  //////////////////////////////////////////


// Telegram Settings
// First create a bot with telegram's botfather and write down the bot settings. 
// Findout your own telegramID (this is the adminID to communicate with the bot).
// If you create a channel, findout the channelID (chatID).

#define botName "<botName>"  // for Bot use
#define botUserName "<botUserName>" // for Bot use
#define botToken "<like 123456789:AABBccDDeeFfgGHHiIjjKklLMmnNooPPqQrR" // for Bot use
//#define adminID "like 12345678" // your ID, use this ID if you want do talk directly to the bot
#define chatID "like -123456789" // channelID, use this if you want to talk to the bot via channel, (leading "-" needed)

/////////////////////////  Wifi Config  //////////////////////////////////////////
static char ssid[] = "<your-SSID";
static char password[] = "your-password";
static char hostname[] = "<your-hostname-for-ESP"; /////////////////////////////////

int state = 0;

TelegramBOT bot(botToken, botName, botUserName);
// ---------------------------------------------------------------------------------------------------------------

#define NB_TRYWIFI    10

const char* deviceName = hostname;

void setup()
     {
     rst_info *rinfo = ESP.getResetInfoPtr();
     Serial.println(String("\nResetInfo.reason = ") + (*rinfo).reason + ": " + ESP.getResetReason()); 
     Serial.begin(115200);
         
     #if mqttenabled == 1
     client.setServer(mqttserver, 1883);
     #endif
     
     //Serial.println(""); Serial.print("Reason for startup :");Serial.println(ESP.getResetReason());
     wifi_station_set_hostname(hostname);
     WiFi.begin(SSID, password);
     Serial.print("\n  Connecting to " + String(SSID) + " ");
     int _try = 0;
     // Wait until it is connected to the network
     while (WiFi.status() != WL_CONNECTED) 
           {
           delay(300);
           Serial.print(".");
           }
     delay(300);
     _try++;
     if ( _try >= NB_TRYWIFI ) 
        {
        Serial.println("Can't connect to wifi accesspoint, going into deepsleep!");
        delay(500);
        delay(60000);
        }
     Serial.println();       
     Serial.println("Wifi connected.");  
     Serial.println("IP-Adresse: " + WiFi.localIP().toString());
     Serial.println("Hostname:  " + String(hostname));
     bot.begin();          // initialize telegram bot
     Serial.println("Doorbell online.");
     Serial.println("- - - - - - - - - - - - - - - - - - -\n");
     //delay(300);
        
     // Fritzbox connection
     connection.init();
     #if mqttenabled == 1
     client.connect(deviceName);
     #endif

     //Second value is internal call number. **9 is for broadcast.
     //String params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**9"}};  //default
     String params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**610"}};
 
     String req[][2] = {{}};
     String params1[][2] = {{}};
     connection.action("urn:dslforum-org:service:X_VoIP:1","X_AVM-DE_DialNumber", params, 1, req, 0);
 
     #if mqttenabled ==1
     client.publish("fritzbox/klingel", "Ring Ring");
     #endif
 
     //Enter Ringingtime (Milliseconds)
     delay(3000);
     connection.action("urn:dslforum-org:service:X_VoIP:1","X_AVM-DE_DialHangup", params1, 1, req, 0);
      
     Serial.print("Doorbell rings!");
     bot.sendMessage(chatID, String(hostname) + ", " + "Doorbell rings!", "");  // Bot <-> Channel        
     Serial.println();
     delay(200);
   
     ESP.deepSleep(0);
}

//////////////////////////////////////////////////////////////////
void loop()
{
 
}

//////////////////////////////////////////////////////////////////

// 28.4.2019 works perfekt, 25µA in deepSleep, 1,04A for some Milliseconds at startup.
