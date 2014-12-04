/* This is a simple program to connect the LinkIt ONE to my favorite Internet of Things (IOT) data streaming service - Ubitdots.
Chip McClelland - Cellular Data Logger
BSD license, Please keep my name in any redistribution
Requirements: 
  - Account on Ubidots.  http://www.ubidots.com
  - Seedstudio LinkIt ONE - http://www.seeedstudio.com/wiki/LinkIt_ONE
Please note this is just the start - uploading only a single data point.  More to come but as your project may
diverge from mine, this may be all you need to get started.  Good luck.

This code will be part of a larger project to build a connected Air Quality Sensor - 
  - Project documentation here: http://triembed.org/blog/?page_id=736
  - GitHub repo here: https://github.com/chipmc/AirQualitySensorProject

Enjoy! And comments welcome on the TriEmbed Blog above
***********************************************************************************

#include <LBattery.h>  // Want to be able to read the battery charge level
#include <LGPRS.h>      //include the base GPRS library
#include <LGPRSClient.h>  //include the ability to Post and Get information using HTTP

// These are the variables you will want to change based on your IOT data streaming account / provider
char action[] = "POST ";  // Edit to build your command - "GET ", "POST ", "HEAD ", "OPTIONS " - note trailing space
char server[] = "things.ubidots.com";
char path[] = "/api/v1.6/variables/xxxxxxxxxxxxxxxxxxxxxxxxxxx/values";  // Edit Path to include you source key
char token[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  // Edit to insert you API Token
int port = 80; // HTTP

// Here are the program variables
int num;                            // part of the length calculation
String le;                         // length of the payload in characters
String var;                        // This is the payload or JSON request
unsigned long ReportingInterval = 20000;  // How often do you want to update the IOT site in milliseconds
unsigned long LastReport = 0;      // When was the last time you reported
const int ledPin = 13;                 // Light to blink when program terminates

LGPRSClient globalClient;  // See this support topic from Mediatek - http://labs.mediatek.com/forums/posts/list/75.page

void setup()
{
  Serial.begin(19200);   // setup Serial port
  Serial.println("Attach to GPRS network");   // Attach to GPRS network - need to add timeout
  while (!LGPRS.attachGPRS("internet2.voicestream.com","","")) { //attachGPRS(const char *apn, const char *username, const char *password);
    delay(500);
  } 
  LGPRSClient client;    //Client has to be initiated after GPRS is established with the correct APN settings - see above link
  globalClient = client;  // Again this is a temporary solution described in support forums
}

void loop(){
  if (globalClient.available()) {// if there are incoming bytes available from the server
    char c = globalClient.read(); 	// read them and print them:
    Serial.print(c);
  }
  if (millis() >= LastReport + ReportingInterval) {  // Section to report - will convert to a function on next rev
    String value = String(LBattery.level());
    var="{\"value\":"+ value + "}"; //Build the JSON packet - Batter + GPS Context
    num=var.length();               // How long is the payload
    le=String(num);                 //this is to calcule the length of var
    Serial.print("Connect to ");    // For the console - show you are connecting
    Serial.println(server);
    if (globalClient.connect(server, port)){  // if you get a connection, report back via serial:
      Serial.println("connected");  // Console monitoring
      globalClient.print(action);                   // These commands build a JSON request for Ubidots but fairly standard
      globalClient.print(path);                     // specs for this command here: http://ubidots.com/docs/api/index.html
      globalClient.println(" HTTP/1.1");
      globalClient.println(F("Content-Type: application/json"));
      globalClient.print(F("Content-Length: "));
      globalClient.println(le);
      globalClient.print(F("X-Auth-Token: ")); 
      globalClient.println(token);               
      globalClient.print(F("Host: ")); 
      globalClient.println(server);
      globalClient.println();
      globalClient.println(var);  // The payload defined above
      globalClient.println();
      globalClient.println((char)26); //This terminates the JSON SEND with a carriage return
      LastReport = millis();
    }
  }
}

