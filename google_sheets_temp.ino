/*
   ESP822 temprature logging to Google Sheet
   CircuitDigest(www.circuitdigest.com)
*/

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <Wire.h>
#define DHTPIN D4                                                           // what digital pin we're connected to
#define DHTTYPE DHT11                                                       // select dht type as DHT 11 or DHT22
DHT dht(DHTPIN, DHTTYPE);
const long utcOffsetInSeconds = 10800;// saat için +3 tr için
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
float h;
float t;
String sheetHumid = "";
String sheetTemp = "";
///kendi yazdığım şeyler saat tarih için/////
String sheetDate = "";
String sheetTime = "";

const char* ssid = " your wifi ssid";                //replace with our wifi ssid
const char* password = "your wifi password";         //replace with your wifi password

const char* host = "script.google.com";
const char *GScriptId = "your own google script id"; // Replace with your own google script id
const int httpsPort = 443; //the https port is same

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "";

//const uint8_t fingerprint[20] = {};

String url = String("/macros/s/") + GScriptId + "/exec?value=SICAKLIK";  // Write Teperature to Google Spreadsheet at cell A1
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";  // Write to Cell A continuosly

//replace with sheet name not with spreadsheet file name taken from google
String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"TempSheet\", \
                       \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;

// used to store the values of free stack and heap before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets upon instantiation

const  unsigned long zamandongu = 1000;
const  unsigned long derecedongu = 3000;
const  unsigned long datadongu = 600000;
unsigned long oncekiZaman1 = 0; // zaman
unsigned long oncekiZaman2 = 0; // derece
unsigned long oncekiZaman3 = 0; //data

//unsigned long lastTime1 = 0;
void setup() {
  delay(1000);
  Serial.begin(115200);
  lcd.begin();
 lcd.backlight();
 lcd.print("   Hosgeldiniz");  
 lcd.setCursor(0,1);
 lcd.print("Sistem Basliyor");
 delay(1500);
lcd.clear();
  dht.begin();     //initialise DHT11
  Serial.println();
  Serial.print("Connecting to wifi: ");
  lcd.setCursor(0,0);
 lcd.print("wifi baglaniyor");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  lcd.clear();
   lcd.setCursor(0,0);
 lcd.print("wifi baglandi...");
 delay(1000);
 lcd.clear();
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  Serial.print("Connecting to ");
  Serial.println(host);          //try to connect with "script.google.com"

  // Try to connect for a maximum of 5 times then exit
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }
  // Finish setup() function in 1s since it will fire watchdog timer and will reset the chip.
  //So avoid too many requests in setup()

  Serial.println("\nWrite into cell 'A1'");
  Serial.println("------>");
  // fetch spreadsheet data
  client->GET(url, host);

  Serial.println("\nGET: Fetch Google Calendar Data:");
  Serial.println("------>");
  // fetch spreadsheet data
  client->GET(url2, host);

  Serial.println("\nStart Sending Sensor Data to Google Spreadsheet");


  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}
////////////////////////////
char Time[] = "  :  :  ";
char Date[] = "  -  -20  ";
byte last_second, last_minute, second_, minute_, hour_, wday, day_, month_, year_;
///////////////////////////////////

void loop() {
 unsigned long simdikiZaman = millis();

if (simdikiZaman - oncekiZaman1 >= zamandongu )
{
  zaman();
  oncekiZaman1 = simdikiZaman;
  
}
  if (simdikiZaman - oncekiZaman2 >= derecedongu )
  {
    tempHum();
    oncekiZaman2 = simdikiZaman;
  }

  if (simdikiZaman - oncekiZaman3 >= datadongu )
 {
    googleSend();
    oncekiZaman3 = simdikiZaman;
  }

  

 
}

void googleSend()
{
   static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;
   sheetHumid = String(h) + String("%");                                         //convert integer humidity to string humidity
 // Serial.print("%  Temperature: ");  Serial.print(t);  Serial.println("°C ");
  sheetTemp = String(t) + String("°C");
  sheetDate = Date;
  sheetTime = Time;

  //payload = payload_base + "\"" + sheetTemp + "," + sheetHumid + "\" + }";
  payload = payload_base + "\"" + sheetTemp + "," + sheetHumid + "," + sheetDate + "," + sheetTime + "\"}";

  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(host, httpsPort);
      client->POST(url2, host, payload, false);
      Serial.print("Sent : ");  Serial.println("Temp and Humid");
    }
  }
  else {
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }

  if (connect_count > MAX_CONNECT) {
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

  Serial.println("POST or SEND Sensor data to Google Spreadsheet:");
  if (client->POST(url2, host, payload)) 
  {
    ;
  }
  else {
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }

//  if (error_count > 3) {
//    Serial.println("Halting processor...");
//    delete client;
//    client = nullptr;
//    Serial.printf("Final free heap: %u\n", ESP.getFreeHeap());
//    Serial.printf("Final stack: %u\n", ESP.getFreeContStack());
//    Serial.flush();
//    ESP.deepSleep(0);
//  }
  delay (5000);
}
void tempHum()

{  h = dht.readHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
  t = dht.readTemperature();                                           // Read temperature as Celsius (the default)
  if (isnan(h) || isnan(t)) {                                                // Check if any reads failed and exit early (to try again).
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
    Serial.print("Humidity: ");  Serial.print(h);
  sheetHumid = String(h) + String("%");                                         //convert integer humidity to string humidity
  Serial.print("%  Temperature: ");  Serial.print(t);  Serial.println("°C ");
  sheetTemp = String(t) + String("°C");
  sheetDate = Date;
  sheetTime = Time;
   lcd.setCursor(0,1);
  lcd.print("S:");
   lcd.setCursor(2,1);
  lcd.print(t);
   lcd.setCursor(4,1);
   lcd.print((char)223);
   lcd.setCursor(5,1);
    lcd.print("C ");
  lcd.setCursor(8,1);
  lcd.print("N:");
  lcd.setCursor(10,1);
   lcd.print("%"); 
   lcd.setCursor(11,1);
  lcd.print(h);
  lcd.setCursor(13,1);
   lcd.print("   "); 

}

void zaman()
{
 

   timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time

  second_ = second(unix_epoch);        // get seconds from the UNIX Epoch time
  if (last_second != second_)          // update time & date every 1 second
  {
    minute_ = minute(unix_epoch);      // get minutes (0 - 59)
    hour_   = hour(unix_epoch);        // get hours   (0 - 23)
    wday    = weekday(unix_epoch);     // get minutes (1 - 7 with Sunday is day 1)
    day_    = day(unix_epoch);         // get month day (1 - 31, depends on month)
    month_  = month(unix_epoch);       // get month (1 - 12 with Jan is month 1)
    year_   = year(unix_epoch) - 2000; // get year with 4 digits - 2000 results 2 digits year (ex: 2018 --> 18)
    Time[7] = second_ % 10 + '0';
   Time[6] = second_ / 10 + '0';
    Time[4] = minute_ % 10 + '0';
    Time[3] = minute_ / 10 + '0';
    Time[1] = hour_   % 10 + '0';
    Time[0] = hour_   / 10 + '0';
    Date[9] = year_   % 10 + '0';
    Date[8] = year_   / 10 + '0';
    Date[4] = month_  % 10 + '0';
    Date[3] = month_  / 10 + '0';
    Date[1] = day_    % 10 + '0';
    Date[0] = day_    / 10 + '0';
    Serial.println(Time);
    Serial.println(Date);
   
     lcd.setCursor(0,0);
     lcd.print(Date);
// lcd.setCursor(10,0);
// lcd.print(" /");
 lcd.setCursor(11,0);
  lcd.print(Time); 
  // delay(1000);
  }

}
