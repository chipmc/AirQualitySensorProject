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

Updated for v2 - added a GPS "Context" the the battery voltage so Ubidots will 
display the location of your LinkIt ONE on a map widget.

Added a basic temperature loggersection based on the Microchip TC74 - I2C sensor
Don't forget the 4.7k pull-ups for SDA and SCL.  Used 3.3V logic

Enjoy! And comments welcome on the TriEmbed Blog above
***********************************************************************************/
#include <LGPS.h>            // LinkIt GPS Library
#include <LBattery.h>        // Want to be able to read the battery charge level
#include <LGPRS.h>           //include the base GPRS library
#include <LGPRSClient.h>     //include the ability to Post and Get information using HTTP
#include <Wire.h>            //Include Wire I2C library
int temp_address = 72;       //1001000 written in decimal for the TC74 Sensor

// These are the variables you will want to change based on your IOT data streaming account / provider
char action[] = "POST ";  // Edit to build your command - "GET ", "POST ", "HEAD ", "OPTIONS " - note trailing space
char server[] = "things.ubidots.com";
char path[] = "/api/v1.6/variables/";  // Common path
char battkey[] = "xxxxxxxxxxxxxxx";  // Battery API Key
char tempkey[] = "xxxxxxxxxxxxxxx"; // Temp API Key
char token[] = "xxxxxxxxxxxxx";  // Edit to insert you API Token
int port = 80; // HTTP

// Here are the program variables
unsigned long ReportingInterval = 20000;  // How often do you want to update the IOT site in milliseconds
unsigned long LastReport = 0;             // When was the last time you reported
const int ledPin = 13;                    // Light to blink when program terminates
String Location = "";                     // Will build the Location string here

// Create instantiations of the GPRS and GPS functions
LGPRSClient globalClient;  // See this support topic from Mediatek - http://labs.mediatek.com/forums/posts/list/75.page
gpsSentenceInfoStruct info;  // instantiate

void setup()
{
  Serial.begin(19200);             // setup Serial port
  Wire.begin();                    // Start Wire
  LGPS.powerOn();                  // Start the GPS first as it takes time to get a fix
  Serial.println("GPS Powered on, and waiting ..."); 
  Serial.println("Attach to GPRS network");   // Attach to GPRS network - need to add timeout
  while (!LGPRS.attachGPRS("internet2.voicestream.com","","")) { //attachGPRS(const char *apn, const char *username, const char *password);
    delay(500);
  } 
  LGPRSClient client;    //Client has to be initiated after GPRS is established with the correct APN settings - see above link
  globalClient = client;  // Again this is a temporary solution described in support forums
}

void loop()
{
  if (globalClient.available()) {// if there are incoming bytes available from the server
    char c = globalClient.read(); 	// read them and print them:
    Serial.print(c);
  }    
  if (millis() >= LastReport + ReportingInterval) {  // Section to report - will convert to a function on next rev
    SendToUbidots(BatteryPayload(), battkey);    // Send it to Ubidots
    SendToUbidots(TempPayload(), tempkey);    // Send it to Ubidots    
    LastReport = millis();
  }
}

void SendToUbidots(String payload, String apikey)
{
  int num;                          // part of the length calculation
  String le;                        // length of the payload in characters
  num=payload.length();           // How long is the payload
  le=String(num);                 //this is to calcule the length of var
  Serial.print("Connect to ");    // For the console - show you are connecting
  Serial.println(server);
  if (globalClient.connect(server, port)){  // if you get a connection, report back via serial:
    Serial.println("connected");  // Console monitoring
    globalClient.print(action);                   // These commands build a JSON request for Ubidots but fairly standard
    globalClient.print(path);                     // specs for this command here: http://ubidots.com/docs/api/index.html
    globalClient.print(apikey);                  // Prints the battery key
    globalClient.println("/values HTTP/1.1");
    globalClient.println(F("Content-Type: application/json"));
    globalClient.print(F("Content-Length: "));
    globalClient.println(le);
    globalClient.print(F("X-Auth-Token: ")); 
    globalClient.println(token);               
    globalClient.print(F("Host: ")); 
    globalClient.println(server);
    globalClient.println();
    globalClient.println(payload);  // The payload defined above
    globalClient.println();
    globalClient.println((char)26); //This terminates the JSON SEND with a carriage return
  }
}

String TempPayload()
{
  String payload;
  String value = String(GetTemp());   // Read the Temperature
  payload="{\"value\":"+ value + "}"; // Build the JSON packet without GPS info
  return payload;
}

String BatteryPayload()
{
  String payload;
  String value = String(LBattery.level());   // Read the battery level
  LGPS.getData(&info);                       // Get a GPS fix
  if (ParseLocation((const char*)info.GPGGA)) {  // This is where we break out needed location information
    Serial.print("Location is: ");
    Serial.println(Location);                 // This is the format needed by Ubidots
  }
  if (Location.length()==0) {
    payload="{\"value\":"+ value + "}"; //Build the JSON packet without GPS info
    }
  else {
    payload="{\"value\":"+ value + ", \"context\":"+ Location + "}";  // with GPS info
  }
  return payload;
}


boolean ParseLocation(const char* GPGGAstr) 
// Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
// Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
{
  char latarray[6];
  char longarray[6];
  int index = 0;
  Serial.println(GPGGAstr);
  Serial.print("Fix Quality: ");
  Serial.println(GPGGAstr[43]);
  if (GPGGAstr[43]=='0') {        //  This is the place in the sentence that shows Fix Quality 0 means no fix
    Serial.println("No GPS Fix");
    Location = "";           // No fix then no Location string
    return 0;
  }
  String GPSstring = String(GPGGAstr);
  for (int i=20; i<=26; i++) {         // We have to jump through some hoops here
    latarray[index] = GPGGAstr[i];     // we need to pick out the minutes from the char array
    index++;
  }
  float latdms = atof(latarray);        // and convert them to a float
  float lattitude = latdms/60;          // and convert that to decimal degrees
  String lattstring = String(lattitude);// Then put back into a string
  Location = "{\"lat\":";
  if(GPGGAstr[28] == 'S') Location = Location + "-";
  Location += GPSstring.substring(18,20) + "." + lattstring.substring(2,4);
  index = 0;
  for (int i=33; i<=38; i++) {         // And do the same thing for longitude
    longarray[index] = GPGGAstr[i];     // the good news is that the GPS data is fixed column
    index++;
  }
  float longdms = atof(longarray);        // and convert them to a float
  float longitude = longdms/60;          // and convert that to decimal degrees
  String longstring = String(longitude);// Then put back into a string
  Location += " ,\"lng\":";
  if(GPGGAstr[41] == 'W') Location = Location + "-";
  if(GPGGAstr[30] == '0') {
    Location = Location + GPSstring.substring(31,33) + "." + longstring.substring(2,4) + "}";
  }
  else {
    Location = Location + GPSstring.substring(30,33) + "." + longstring.substring(2,4) + "}";
  }
  return 1;
}

int GetTemp() 
{
  // Datasheet at http://ww1.microchip.com/downloads/en/DeviceDoc/21462c.pdf
  Wire.beginTransmission(temp_address); // Begin using i2c device at address
  Wire.write(0); //Send a bit asking for register zero, the data register 
  Wire.endTransmission();   //Complete Transmission with pointer on TC74 Register 0
  Wire.requestFrom(temp_address, 1);   //Request 1 Byte from the specified address
  while(Wire.available() == 0);   //wait for response 
  int c = Wire.read();   // Get the temp and read it into a variable
  int f = round(c*9.0/5.0 +32.0);  //Convert the Celsius to Fahrenheit
  return f;  // Return the Temperature
}
