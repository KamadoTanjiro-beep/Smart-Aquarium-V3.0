/*********
ORIGINAL:
  Rui Santos
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

Modified:
Aniket Patra
24/12/2022 (started earlier) V1.0.0.0 

Button | Code
-------|------
Auto   |   1
-------|------
Chooset|   2
-------|------
Manual |   3
-------|------
OFF    |   4
-------|------
ON     |   5
-------|------
PSO    |   6
-------|------
PSOFF  |   7
-------|------
PSOT   |   8
-------|------
PSOFFT |   9
-------|------

RELAY  | Count
-------|------
RELAY1 |   1
-------|------
RELAY2 |   2
-------|------
RELAY3 |   3
-------|------
RELAY4 |   4
-------|------
RELAY5 |   5
-------|------

31/12/22 V1.0.1.0
-Feature Update
*Power Saver Mode can now show remaining time on web server (in notification area)
*Power Saver Mode can now accept ON Period and OFF Period from Webserver.
*Enabled Power Saver for every relay
*Few optimisations
*********/

// Import required libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <FS.h>  //Include File System Headers
// OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DS3231.h>
#include <string.h>

#include "global.h"  //remove this
// ESPNOW
#include <espnow.h>

#ifndef STASSID
#define STASSID "YOUR_SSID"     // WIFI NAME/SSID
#define STAPSK "YOUR_PASSWORD"  // WIFI PASSWORD
#endif

const char *ssid = pssid;      // remove "pssid" and write "STASSID"
const char *password = ppass;  // remove "ppass" and write "STAPSK"

const char *PARAM_INPUT_1 = "count";
const char *PARAM_INPUT_2 = "code";
const char *PARAM_INPUT_3 = "state";

//Clock initialization
DS3231 Clock;
bool century = false;
bool h12Flag = false;
bool pmFlag;
//**********END*************

const String newHostname = "AquaNode";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set your Static IP address
IPAddress local_IP(192, 168, 29, 123);
// Set your Gateway IP address
IPAddress gateway(192, 168, 29, 1);

IPAddress subnet(255, 255, 255, 0);

IPAddress primaryDNS(8, 8, 8, 8);    // optional
IPAddress secondaryDNS(8, 8, 4, 4);  // optional

/*void fileWriter(int data, int count) {
  //The data in integer is a time, ex 06:00 is 600, count is 1 for on time and 2 for off time
  String rem = String(data);
  rem = rem + '\n';
  File file;

  if (count == 1)
    file = SPIFFS.open("/onData.txt", "w");  // Open File for reading
  else if (count == 2)
    file = SPIFFS.open("/offData.txt", "w");  // Open File for reading

  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  int bytesWritten = file.print(rem);
  int len = rem.length();

  if (bytesWritten == len) {
    Serial.println("Data Saved: ");
    Serial.println(bytesWritten);
    Serial.println(rem);
  } else {
    Serial.println("Data Save Failed");
    Serial.println(bytesWritten);
    Serial.println(len);
  }
  file.close();
}

void fileReader(int count) {
  File file;
  String rem;
  if (count == 1)
    file = SPIFFS.open("/onData.txt", "r");  // Open File for reading
  else if (count == 2)
    file = SPIFFS.open("/offData.txt", "r");  // Open File for reading

  if (!file) {
    Serial.println("Error opening file for reading");
    return;
  }
  int line = 0;
  while (file.available()) {
    if (line == 0) {
      rem = file.readStringUntil('\n');
    }
  }

  if (count == 1)
    onTime = rem.toInt();
  else if (count == 2)
    offTime = rem.toInt();
  file.close();
}*/

unsigned long lastTime = 0;
const long timerDelay = 30000;  // check relay status every 30 seconds

unsigned long lastTime1 = 0;
const long timerDelay1 = 5000;  // check wifi status every 900 m.seconds

unsigned long lastTime2 = 0;
const long timerDelay2 = 1000;  // send time every 1000 m.seconds

// utc offset according to India, please change according to your location
const long utcOffsetInSeconds = 19800;

char week[7][20] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET LED_BUILTIN  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C     // 0x3c for 0x78
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// different WiFi icons Full, Medium, Half and Low Signal. ! for no connection
static const unsigned char PROGMEM wifiFull[] = {
  0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x3F, 0xFC, 0x70, 0x0E, 0xE0, 0x07, 0x47, 0xE2, 0x1E, 0x78,
  0x18, 0x18, 0x03, 0xC0, 0x07, 0xE0, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM wifiMed[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x1E, 0x78,
  0x18, 0x18, 0x03, 0xC0, 0x07, 0xE0, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM wifiHalf[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xC0, 0x07, 0xE0, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM wifiLow[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

const unsigned char picture[] PROGMEM = {  // picture of a window (during boot)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// forward declarations

void checkTimeFor(int, int, int);
void showWifiSignal();
void showTime();
void updateRTC();
void relayStatusPrinter();

// status flags

  
byte OLEDConfig = 0;                                      // Config=1 means auto (based on timer),Config=0 means Manual
byte OLEDStatus = 1, updateTime = 0;                                                                              // Status = 1 means OLED DISPLAY is ON and reversed for 0 (works only if Config is 0)

// oled display off at night timings (needs improvement)
#define OLED_OFF 9  // Hour only. In 24 hours mode i.e. 23 for 11 pm
#define OLED_ON 7

// relay gpio pins
const byte relayPin1 = 0;
const byte relayPin2 = 12;
const byte relayPin3 = 13;
const byte relayPin4 = 14;

// Name (to be shown on webserver) as per relay gpio pins
const String relay1Name = "Power Head";
const String relay2Name = "Surface Skimmer";
const String relay3Name = "Filter";
const String relay4Name = "Light";

#define RELAY1ON digitalWrite(relayPin1, HIGH)
#define RELAY1OFF digitalWrite(relayPin1, LOW)
#define RELAY2ON digitalWrite(relayPin2, HIGH)
#define RELAY2OFF digitalWrite(relayPin2, LOW)
#define RELAY3ON digitalWrite(relayPin3, HIGH)
#define RELAY3OFF digitalWrite(relayPin3, LOW)
#define RELAY4ON digitalWrite(relayPin4, HIGH)
#define RELAY4OFF digitalWrite(relayPin4, LOW)

// Relay Timing and definitions
typedef struct relaystruct {
  int onTime,offTime,count;
  byte config, autoTimer;
  bool activate;
  byte flag,powerSave,psAlt;
  String relayState;
  unsigned long lastAutoTimer,autoTimerDelay,pslastTime,psTimerDelay,psTimerDelayOn,psTimerDelayOff;
} struct_relay;

// timer settings for autotimers
 // example if it would have to check after 5 mins i.e. 5*60*1000=300000
// 

/* first number is "ON" time in 24 hours. i.e. 2:35pm would be 1435, 
second one is turn "OFF" time, 
the third one is Relay Number, 
the fourth parameter Config=1 means auto (based on timer),Config=0 means Manual and Config=2 means PowSave. Same for aquarium light,
fifth parameter autoTimer is denoting if timer is on(1) or off(0), 
sixth parameter mark true to activate, and false to deactivate (activation means the timer activation),
seventh parameter flag is variable to store the current output state,
eigth parameter powerSave is denoting if PowerSave is on(1) or off(0), 
psAlt=2 means garbage state, used to alternate on and off during powersaver mode working,
tenth param, relayState is used in webserver to show ON or OFF,
11th, 12th, 13th used to autotimer,
14th is powersaver temp. variable, 15th powersaver on time, 16th powersaver off time

BELOW ARE INITIALISATION OF RELAYS*/

struct_relay relay1={1200,1900,1,0,0,false,0,0,2,"OFF",0,0,0,0,600000,600000};
struct_relay relay2={1200,1900,2,2,0,false,0,1,2,"OFF",0,0,0,0,600000,1800000};
struct_relay relay3={1200,1900,3,0,0,false,0,0,2,"OFF",0,0,0,0,600000,600000};
struct_relay relay4={800,1900,4,1,0,true,0,0,2,"OFF",0,0,0,0,600000,600000};

/*void relayInitialize()  // checks and initializes all relay in case of powerloss (takes effect only if relay is activated above). Default: Turns ON the relay if no timer is set
{
  if (relay1.activate) {
    checkTimeFor(relay1.onTime,relay1.offTime,relay1.count);
  } else {
    RELAY1ON;
    relay1.flag = 1;
    relay1.relayState = "ON";
  }

  if (relay2.activate) {
    checkTimeFor(relay2.onTime,relay2.offTime,relay2.count);
  } else {
    RELAY2ON;
    relay2.flag = 1;
    relay2.relayState = "ON";
  }

  if (relay3.activate) {
    checkTimeFor(relay3.onTime,relay3.offTime,relay3.count);
  } else {
    RELAY3ON;
    relay3.flag = 1;
    relay3.relayState = "ON";
  }

  if (relay4.activate) {
    checkTimeFor(relay4.onTime,relay4.offTime,relay4.count);
  } else {
    RELAY4ON;
    relay4.flag = 1;
    relay4.relayState = "ON";
  }
}*/

// ESPNOW#####################################################
//  REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = { 0xF4, 0xCF, 0xA2, 0xF0, 0x0A, 0xE3 };
// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int time[8];
} struct_message;

struct_message myData;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  // Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0) {
    // Serial.println("Delivery success");
  } else {
    // Serial.println("Delivery fail");
  }
}

// Replaces placeholder with button section in your web page
String processor(const String &var) {
  // Serial.println(var);
  //*****************************BUTTONS GROUP BEGIN**************************
  if (var == "BUTTONGROUP1") {
    String buttons = "";
    if (relay1.config == 0) {
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,3)\" type=\"button\">MANUAL</button>";
      if (relay1.relayState == "OFF") {
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,5)\" type=\"button\">ON</button>";
        buttons += "<select class=\"form-select m-sm-1\" id=\"1\" aria-label=\"Default select example\"><option value=\"0\" selected>Select</option><option value=\"5\">05 Minutes</option><option value=\"10\">10 Minutes</option><option value=\"30\">30 Minutes</option></select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,2)\" type=\"button\">Choose Time</button>";
      } else
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,4)\" type=\"button\">OFF</button><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,6)\" type= \"button\">Power Save ON</button>";
    } else if (relay1.config == 1)
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,1)\" type=\"button\">AUTO</button>";
    else if (relay1.config == 2)
    {
      String timeOn;
      String timeOff;
      byte minOn = (relay1.psTimerDelayOn/1000)/60;
      byte minOff = (relay1.psTimerDelayOff/1000)/60;
      timeOn=String(minOn)+" min";
      timeOff=String(minOff)+" min";
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,7)\" type=\"button\">POWER SAVER OFF</button>\
      <hr>\
              <div class=\"row\">\
                <div class=\"col-sm-6\"><span>ON: "+timeOn+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"1\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,8)\"\
                    type=\"button\">SAVE</button>\
                </div>\
                <div class=\"col-sm-6\"><span>OFF: "+timeOff+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"11\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(1,9)\"\
                    type=\"button\">SAVE</button>\
                </div>\
              </div>";
    }
    return buttons;
  }

  if (var == "NOTIFICATION1") {
    String notify = "";
    if (relay1.autoTimer == 1)
      notify += "TIMER ACTIVE";
    else if (relay1.config == 2)
    {
      unsigned long milli = millis() - relay1.pslastTime;
      unsigned long remainMilli = relay1.psTimerDelay - milli;
      int minI = (remainMilli/1000)/60;
      int secI = (remainMilli/1000)%60;
      String min,sec;
      if(minI<10)
        min = "0"+String(minI);
      else
        min = String(minI);

      if(secI<10)
        sec = "0"+String(secI);
      else
        sec = String(secI);

      notify += "POWER SAVER ACTIVE - "+ min + ":" + sec;

    }
    else if (relay1.config == 1)
      notify += "AUTOMATIC CONTROL ACTIVE";
    return notify;
  }

  if (var == "BUTTONGROUP2") {
    String buttons = "";
    if (relay2.config == 0) {
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,3)\" type=\"button\">MANUAL</button>";
      if (relay2.relayState == "OFF") {
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,5)\" type=\"button\">ON</button>";
        buttons += "<select class=\"form-select m-sm-1\" id=\"2\" aria-label=\"Default select example\"><option value=\"0\" selected>Select</option><option value=\"5\">05 Minutes</option><option value=\"10\">10 Minutes</option><option value=\"30\">30 Minutes</option></select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,2)\" type=\"button\">Choose Time</button>";
      } else
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,4)\" type=\"button\">OFF</button> <button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,6)\" type=\"button\">Power Save ON</button>";
    } else if (relay2.config == 1)
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,1)\" type=\"button\">AUTO</button>";
    else if (relay2.config == 2)
    {
      String timeOn;
      String timeOff;
      byte minOn = (relay2.psTimerDelayOn/1000)/60;
      byte minOff = (relay2.psTimerDelayOff/1000)/60;
      timeOn=String(minOn)+" min";
      timeOff=String(minOff)+" min";
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,7)\" type=\"button\">POWER SAVER OFF</button>\
      <hr>\
              <div class=\"row\">\
                <div class=\"col-sm-6\"><span>ON: "+timeOn+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"2\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,8)\"\
                    type=\"button\">SAVE</button>\
                </div>\
                <div class=\"col-sm-6\"><span>OFF: "+timeOff+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"22\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(2,9)\"\
                    type=\"button\">SAVE</button>\
                </div>\
              </div>";
    }
    return buttons;
  }

  if (var == "NOTIFICATION2") {
    String notify = "";
    if (relay2.autoTimer == 1)
      notify += "TIMER ACTIVE";
    else if (relay2.config == 2)
    {
      unsigned long milli = millis() - relay2.pslastTime;
      unsigned long remainMilli = relay2.psTimerDelay - milli;
      int minI = (remainMilli/1000)/60;
      int secI = (remainMilli/1000)%60;
      String min,sec;
      if(minI<10)
        min = "0"+String(minI);
      else
        min = String(minI);

      if(secI<10)
        sec = "0"+String(secI);
      else
        sec = String(secI);

      notify += "POWER SAVER ACTIVE - "+ min + ":" + sec;

    }
    else if (relay2.config == 1)
      notify += "AUTOMATIC CONTROL ACTIVE";
    return notify;
  }

  if (var == "BUTTONGROUP3") {
    String buttons = "";
    if (relay3.config == 0) {
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,3)\" type=\"button\">MANUAL</button>";
      if (relay3.relayState == "OFF") {
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,5)\" type=\"button\">ON</button>";
        buttons += "<select class=\"form-select m-sm-1\" id=\"3\" aria-label=\"Default select example\"><option value=\"0\" selected>Select</option><option value=\"5\">05 Minutes</option><option value=\"10\">10 Minutes</option><option value=\"30\">30 Minutes</option></select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,2)\" type=\"button\">Choose Time</button>";
      } else
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,4)\" type=\"button\">OFF</button> <button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,6)\" type= \"button\">Power Save ON</button>";
    } else if (relay3.config == 1)
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,1)\" type=\"button\">AUTO</button>";
    else if (relay3.config == 2)
      {
      String timeOn;
      String timeOff;
      byte minOn = (relay3.psTimerDelayOn/1000)/60;
      byte minOff = (relay3.psTimerDelayOff/1000)/60;
      timeOn=String(minOn)+" min";
      timeOff=String(minOff)+" min";
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,7)\" type=\"button\">POWER SAVER OFF</button>\
      <hr>\
              <div class=\"row\">\
                <div class=\"col-sm-6\"><span>ON: "+timeOn+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"3\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,8)\"\
                    type=\"button\">SAVE</button>\
                </div>\
                <div class=\"col-sm-6\"><span>OFF: "+timeOff+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"33\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(3,9)\"\
                    type=\"button\">SAVE</button>\
                </div>\
              </div>";
    }
    return buttons;
  }

  if (var == "NOTIFICATION3") {
    String notify = "";
    if (relay3.autoTimer == 1)
      notify += "TIMER ACTIVE";
    else if (relay3.config == 2)
      {
      unsigned long milli = millis() - relay3.pslastTime;
      unsigned long remainMilli = relay3.psTimerDelay - milli;
      int minI = (remainMilli/1000)/60;
      int secI = (remainMilli/1000)%60;
      String min,sec;
      if(minI<10)
        min = "0"+String(minI);
      else
        min = String(minI);

      if(secI<10)
        sec = "0"+String(secI);
      else
        sec = String(secI);

      notify += "POWER SAVER ACTIVE - "+ min + ":" + sec;

    }
    else if (relay3.config == 1)
      notify += "AUTOMATIC CONTROL ACTIVE";
    return notify;
  }

  if (var == "BUTTONGROUP4") {
    String buttons = "";
    if (relay4.config == 0) {
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,3)\" type=\"button\">MANUAL</button>";
      if (relay4.relayState == "OFF") {
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,5)\" type=\"button\">ON</button>";
        buttons += "<select class=\"form-select m-sm-1\" id=\"4\" aria-label=\"Default select example\"><option value=\"0\" selected>Select</option><option value=\"5\">05 Minutes</option><option value=\"10\">10 Minutes</option><option value=\"30\">30 Minutes</option></select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,2)\" type=\"button\">Choose Time</button>";
      } else
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,4)\" type=\"button\">OFF</button><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,6)\" type=\"button\">Power Save ON</button>";
    } else if (relay4.config == 1)
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,1)\" type=\"button\">AUTO</button>";
    else if (relay4.config == 2)
      {
      String timeOn;
      String timeOff;
      byte minOn = (relay4.psTimerDelayOn/1000)/60;
      byte minOff = (relay4.psTimerDelayOff/1000)/60;
      timeOn=String(minOn)+" min";
      timeOff=String(minOff)+" min";
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,7)\" type=\"button\">POWER SAVER OFF</button>\
      <hr>\
              <div class=\"row\">\
                <div class=\"col-sm-6\"><span>ON: "+timeOn+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"4\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,8)\"\
                    type=\"button\">SAVE</button>\
                </div>\
                <div class=\"col-sm-6\"><span>OFF: "+timeOff+"</span>\
                  <select class=\"form-select m-sm-1\" id=\"44\" aria-label=\"Default select example\">\
                    <option value=\"0\" selected>Select</option>\
                    <option value=\"5\">05 Minutes</option>\
                    <option value=\"10\">10 Minutes</option>\
                    <option value=\"30\">30 Minutes</option>\
                  </select><button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(4,9)\"\
                    type=\"button\">SAVE</button>\
                </div>\
              </div>";
    }
    return buttons;
  }

  if (var == "NOTIFICATION4") {
    String notify = "";
    if (relay4.autoTimer == 1)
      notify += "TIMER ACTIVE";
    else if (relay4.config == 2)
      {
      unsigned long milli = millis() - relay4.pslastTime;
      unsigned long remainMilli = relay4.psTimerDelay - milli;
      int minI = (remainMilli/1000)/60;
      int secI = (remainMilli/1000)%60;
      String min,sec;
      if(minI<10)
        min = "0"+String(minI);
      else
        min = String(minI);

      if(secI<10)
        sec = "0"+String(secI);
      else
        sec = String(secI);

      notify += "POWER SAVER ACTIVE - "+ min + ":" + sec;

    }
    else if (relay4.config == 1)
      notify += "AUTOMATIC CONTROL ACTIVE";
    return notify;
  }

  if (var == "BUTTONGROUP5") {
    String buttons = "";
    if (OLEDConfig == 0) {
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(5,1)\" type=\"button\">MANUAL</button>";
      if (OLEDStatus == 0)
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(5,5)\" type=\"button\">OFF</button>";
      else
        buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(5,4)\" type=\"button\">ON</button>";
    } else
      buttons += "<button class=\"btn btn-secondary m-1\" onclick=\"sendButtState(5,3)\" type=\"button\">AUTO</button>";
    return buttons;
  }

  if (var == "NOTIFICATION5") {
    String notify = "";
    if (OLEDConfig == 0)
      notify += "MANUAL CONTROL ACTIVE";
    else
      notify += "AUTOMATIC CONTROL ACTIVE";
    return notify;
  }
  //*****************************BUTTONS END**************************
  if (var == "TIMEDAY") {
    String timeData = "";
    timeData += "<p class=\"card-text\" id=\"time\">" + String(Clock.getHour(h12Flag, pmFlag)) + ":" + String(Clock.getMinute()) + ", " + String(Clock.getDate()) + "/" + String(Clock.getMonth(century)) + "/" + String(Clock.getYear()) + ", " + String(week[Clock.getDoW()]) + ", " + String(int(Clock.getTemperature())) + "&deg;C</p>";
    return timeData;
  }

  //*****************************STATUS BEGIN**************************
  if (var == "STATE1") {
    String state = "";
    if (relay1.relayState == "OFF")
      state += "<h1 class=\"cardOff\">" + relay1.relayState + "</h1>";
    else
      state += "<h1 class=\"cardOn\">" + relay1.relayState + "</h1>";
    return state;
  }
  if (var == "STATE2") {
    String state = "";
    if (relay2.relayState == "OFF")
      state += "<h1 class=\"cardOff\">" + relay2.relayState + "</h1>";
    else
      state += "<h1 class=\"cardOn\">" + relay2.relayState + "</h1>";
    return state;
  }
  if (var == "STATE3") {
    String state = "";
    if (relay3.relayState == "OFF")
      state += "<h1 class=\"cardOff\">" + relay3.relayState + "</h1>";
    else
      state += "<h1 class=\"cardOn\">" + relay3.relayState + "</h1>";
    return state;
  }
  if (var == "STATE4") {
    String state = "";
    if (relay4.relayState == "OFF")
      state += "<h1 class=\"cardOff\">" + relay4.relayState + "</h1>";
    else
      state += "<h1 class=\"cardOn\">" + relay4.relayState + "</h1>";
    return state;
  }
  if (var == "STATE5") {
    String state = "";
    if (OLEDStatus == 0)
      state += "<h1 class=\"cardOff\">OFF</h1>";
    else if (OLEDStatus == 1)
      state += "<h1 class=\"cardOn\">ON</h1>";
    return state;
  }
  if (var == "NAME1") {
    String name = "";
    name += relay1Name;
    return name;
  }
  if (var == "NAME2") {
    String name = "";
    name += relay2Name;
    return name;
  }
  if (var == "NAME3") {
    String name = "";
    name += relay3Name;
    return name;
  }
  if (var == "NAME4") {
    String name = "";
    name += relay4Name;
    return name;
  }

  //*****************************STATUS END**************************
  return String();
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="icon" type="image/jpg"
    href="https://res.cloudinary.com/dllodumsb/image/upload/v1671543429/Aquarium/images_loidvq.jpg" />
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet"
    integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
  <title>Aquarium Management</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Rajdhani:wght@300&display=swap');

    body {
      background: #009688;
      color: rgb(190, 190, 190);
    }

    .card {
      box-shadow: 2px 2px 5px rgb(0, 0, 0);
      border-radius: 30px;
      background: #000000;

    }

    .card-body {
      background: #000000;
    }

    .card-footer {
      background: #171717;
      border-bottom-left-radius: 30px !important;
      border-bottom-right-radius: 30px !important;
    }

    .card-header {
      border-radius: 30px !important;
      border: 0px;
      background: #000000;
    }

    .card-header img {
      border-radius: 30px;

    }

    .cardOn {
      font-family: 'Rajdhani', sans-serif;
      font-size: calc(200px + 1vw);
      color: greenyellow;
    }

    .cardOff {
      font-family: 'Rajdhani', sans-serif;
      font-size: calc(200px + 1vw);
      color: rgb(255, 61, 47);
    }
  </style>
</head>

<body>
  <div class="container">
    <div class="row d-flex justify-content-center">

      <div class="card text-center col-md-4 mt-3 col-sm-10 p-0"
        style="box-shadow: 5px 5px 50px rgb(0 0 0) !important; border-radius: 0px">

        <div class="card-body">
          <h5 class="card-title">Aquarium Management Console</h5>
          %TIMEDAY%
        </div>
      </div>
    </div>

    <div class="row mt-4">
      <div class="col d-flex justify-content-center"> <button class="btn btn-secondary m-1" type="button" onclick="updateTime()">Update
          Time</button></div>
    </div>

    <div class="row d-flex justify-content-center">   
          
              <div class="card text-center col-md-4 col-sm-10 p-0 m-3">
                <div class="card-header p-0" id="STATE1">
                  %STATE1%
                </div>
                <div class="card-body">
                  <h5 class="card-title">%NAME1%</h5>
                  <div class="row d-flex justify-content-center">
                    <div class="d-grid gap-2 d-md-block" id="BUTTONGROUP1">
                      %BUTTONGROUP1%
                    </div>
                  </div>
                </div>
                <div class="card-footer text-muted" id="NOTIFICATION1">
                  %NOTIFICATION1%
                </div>
              </div>            
          
              <div class="card text-center col-md-4 col-sm-10 p-0 m-3">
                <div class="card-header p-0" id="STATE2">
                  %STATE2%
                </div>
                <div class="card-body">
                  <h5 class="card-title">%NAME2%</h5>
                  <div class="row d-flex justify-content-center">
                    <div class="d-grid gap-2 d-md-block" id="BUTTONGROUP2">
                      %BUTTONGROUP2%
                    </div>
                  </div>
                </div>
                <div class="card-footer text-muted" id="NOTIFICATION2">
                  %NOTIFICATION2%
                </div>
              </div>            
           
              <div class="card text-center col-md-4 col-sm-10 p-0 m-3">
                <div class="card-header p-0" id="STATE3">
                  %STATE3%
                </div>
                <div class="card-body">
                  <h5 class="card-title">%NAME3%</h5>
                  <div class="row d-flex justify-content-center">
                    <div class="d-grid gap-2 d-md-block" id="BUTTONGROUP3">
                      %BUTTONGROUP3%
                    </div>
                  </div>
                </div>
                <div class="card-footer text-muted" id="NOTIFICATION3">
                  %NOTIFICATION3%
                </div>
              </div>            
           
              <div class="card text-center col-md-4 col-sm-10 p-0 m-3">
                <div class="card-header p-0" id="STATE4">
                  %STATE4%
                </div>
                <div class="card-body">
                  <h5 class="card-title">%NAME4%</h5>
                  <div class="row d-flex justify-content-center">
                    <div class="d-grid gap-2 d-md-block" id="BUTTONGROUP4">
                    %BUTTONGROUP4%                      
                    </div>
                  </div>
                </div>
                <div class="card-footer text-muted" id="NOTIFICATION4">
                  %NOTIFICATION4%
                </div>
              </div>
           
              <div class="card text-center col-md-4 col-sm-10 p-0 m-3">
                <div class="card-header p-0" id="STATE5">
                %STATE5%                  
                </div>
                <div class="card-body">
                  <h5 class="card-title">OLED Display</h5>
                  <div class="row d-flex justify-content-center">
                    <div class="d-grid gap-2 d-md-block" id="BUTTONGROUP5">
                    %BUTTONGROUP5%                                           
                    </div>
                  </div>
                </div>
                <div class="card-footer text-muted" id="NOTIFICATION5">
                  %NOTIFICATION5%
                </div>
              </div>
    </div>
  </div>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"
    integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p"
    crossorigin="anonymous"></script>

  <script>
function updateTime(){      
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/updateTime?state=1", true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          alert(`Error ${xhr.status}: ${xhr.statusText}`);
        } else {
          alert(`${xhr.responseText} `);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };              
    }
  </script>
<script>
 var state = ["STATE0","STATE1","STATE2","STATE3","STATE4","STATE5"];
    var buttongroup = ["BUTTONGROUP0","BUTTONGROUP1","BUTTONGROUP2","BUTTONGROUP3","BUTTONGROUP4","BUTTONGROUP5"];
    var notification=["NOTIFICATION0","NOTIFICATION1","NOTIFICATION2","NOTIFICATION3","NOTIFICATION4","NOTIFICATION5"];
    
    function sendButtState(count,code) {
      try{
        if (code == 9)
          var x = document.getElementById(count*11).value;
        else
          var x = document.getElementById(count).value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/apiButt?count="+count+"&code="+code+"&state="+x, true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          console.log(`Error ${xhr.status}: ${xhr.statusText}`);
        } else {
          console.log(`${xhr.responseText} `);
          codeUpdate(state[count]);
            codeUpdate(buttongroup[count]);
            codeUpdate(notification[count]);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };
    }
    catch(err)
    {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/apiButt?count="+count+"&code="+code+"&state="+99, true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          console.log(`Error ${xhr.status}: ${xhr.statusText}`);
        } else {
          console.log(`${xhr.responseText} `);
          codeUpdate(state[count]);
            codeUpdate(buttongroup[count]);
            codeUpdate(notification[count]);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };
    }}
  </script>
  <script>
    var week = ["Sunday", "Monday", "Tueday", "Wednesday", "Thursday", "Friday", "Saturday"]
    function everyTime() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/time?state=1", true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          console.log(`Error ${xhr.status}: ${xhr.statusText}`);

        } else {
          var x = xhr.responseText;
          var myArray = x.split(":");
          if (myArray[0].length == 1)
            myArray[0] = "0" + myArray[0];

          if (myArray[1].length == 1)
            myArray[1] = "0" + myArray[1];

          if (myArray[3].length == 1)
            myArray[3] = "0" + myArray[3];

          if (myArray[4].length == 1)
            myArray[4] = "0" + myArray[4];

          let word = myArray[0] + ':' + myArray[1] + ', ' + myArray[3] + '/' + myArray[4] + '/20' + myArray[5] + ', ' + week[parseInt(myArray[6])] + ', ' + myArray[2] + '&deg;C';
          document.getElementById('time').innerHTML = word;
          //console.log(`Done, got ${xhr.responseText} `);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };
    }

    var myInterval = setInterval(everyTime, 5000);
  </script>
  <script>
  var state = ["STATE0", "STATE1", "STATE2", "STATE3", "STATE4", "STATE5"];
    var buttongroup = ["BUTTONGROUP0", "BUTTONGROUP1", "BUTTONGROUP2", "BUTTONGROUP3", "BUTTONGROUP4", "BUTTONGROUP5"];
    var notification = ["NOTIFICATION0", "NOTIFICATION1", "NOTIFICATION2", "NOTIFICATION3", "NOTIFICATION4", "NOTIFICATION5"];

    function autoUpdate() {
      for (let index = 1; index < 6; index++) {
        codeUpdate(state[index]);
        codeUpdate(buttongroup[index]);
        codeUpdate(notification[index]);
      }
    }
    var myInterval = setInterval(autoUpdate, 5000);

  function codeUpdate(x) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/codeUpdate?state="+x, true);
    xhr.send();
    xhr.onload = function () {
      if (xhr.status != 200) {
        console.log(`Error ${xhr.status}: ${xhr.statusText}`);
      } else {
       //document.getElementById(x).innerHTML=xhr.responseText;
          var curr = document.getElementById(x).innerHTML;
          curr=curr.trim();
          var fresh = xhr.responseText;
          fresh=fresh.trim();
          if (curr != fresh) {            
            //console.log(curr);
            //console.log(fresh);
            document.getElementById(x).innerHTML=fresh;
          }
          else
          console.log("Cycle Complete");
      }
    };
    xhr.onerror = function () {
      console.log("Request failed");
    };
  }
</script>
</body>

</html>
)rawliteral";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  bool success = SPIFFS.begin();
  SPIFFS.gc();  // garbage collection
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }

  Wire.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    delay(3000);
    // ESP.restart();
  }
  display.clearDisplay();
  display.drawBitmap(0, 0, picture, 128, 64, WHITE);
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(30, 25);    // Start at top-left corner
  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text

  display.println(F("Connecting..."));
  display.display();
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname(newHostname.c_str());  // Set new hostname
  WiFi.begin(ssid, password);
  int count = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);  // Start at top-left corner
    display.println(F("Connection\n\nFailed!"));
    display.display();
    delay(5000);
    // ESP.restart();
    count = 1;
    break;
  }
  // OTA UPDATE SETTINGS  !! CAUTION !! PROCEED WITH PRECAUTION
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);  // Start at top-left corner
    display.println(F("OTA Update\n"));
    display.setTextSize(1);
    display.print("Type: ");
    display.print(type);
    display.display();
    delay(2000);
  });
  ArduinoOTA.onEnd([]() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(15, 25);  // Start at top-left corner
    display.println(F("Rebooting"));
    display.display();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);  // Start at top-left corner
    display.print("Progress: ");
    display.setCursor(50, 35);
    display.print((progress / (total / 100)));
    display.setCursor(88, 35);
    display.print("%");
    display.display();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);  // Start at top-left corner
    display.print("Error!");
    display.display();
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);  // Start at top-left corner
      display.print("Auth Failed");
      display.display();
    } else if (error == OTA_BEGIN_ERROR) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);  // Start at top-left corner
      display.print("Begin Failed");
      display.display();
    } else if (error == OTA_CONNECT_ERROR) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);  // Start at top-left corner
      display.print("Connect Failed");
      display.display();
    } else if (error == OTA_RECEIVE_ERROR) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);  // Start at top-left corner
      display.print("Receive Failed");
      display.display();
    } else if (error == OTA_END_ERROR) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);  // Start at top-left corner
      display.print("End Failed");
      display.display();
    }
  });
  ArduinoOTA.begin();
  // OTA UPDATE END

  // checks if wifi connected
  if (count == 0) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);  // Start at top-left corner
    display.println(F("Connected!"));
    display.setTextSize(1);
    display.println("");
    display.println(WiFi.SSID());
    display.print("\nIP address:\n");
    display.println(WiFi.localIP());
    display.display();

    delay(4000);
    display.clearDisplay();
    display.setCursor(0, 0);

    timeClient.begin();  // NTP time
    timeClient.update();

    display.println(F("Started Time Client"));
    display.display();
    delay(2000);
    display.clearDisplay();

    updateRTC();
    display.clearDisplay();
  }

    display.println(F("Setting Web Server"));
    display.display();
    delay(2000);
    display.clearDisplay();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/apiButt?count=<inputMessage1>&code=<inputMessage2>
  server.on("/apiButt", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage1, inputMessage2, inputMessage3;
    int relay = 0, button = 0, value = 0;
    // GET input1 value on <ESP_IP>/apiButt?count=<inputMessage1>&code=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
      relay = inputMessage1.toInt();
      button = inputMessage2.toInt();
      value = inputMessage3.toInt();
    } else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
      inputMessage3 = "No message sent";
    }
    if (relay == 1 && button == 3) {
      relay1.config = 1;
      relay1.autoTimer = 0;
    } else if (relay == 1 && button == 1)
      relay1.config = 0;
    else if (relay == 1 && button == 5) {
      relay1.relayState = "ON";
      relay1.flag = 1;
      RELAY1ON;
      relay1.autoTimer = 0;
    } else if (relay == 1 && button == 4) {
      relay1.relayState = "OFF";
      relay1.flag = 0;
      RELAY1OFF;
    }
    if (relay == 2 && button == 3) {
      relay2.config = 1;
      relay2.autoTimer = 0;
    } else if (relay == 2 && button == 1)
      relay2.config = 0;
    else if (relay == 2 && button == 5) {
      relay2.relayState = "ON";
      relay2.flag = 1;
      RELAY2ON;
      relay2.autoTimer = 0;
    } else if (relay == 2 && button == 4) {
      relay2.relayState = "OFF";
      relay2.flag = 0;
      RELAY2OFF;
    }
    if (relay == 3 && button == 3) {
      relay3.config = 1;
      relay3.autoTimer = 0;
    } else if (relay == 3 && button == 1)
      relay3.config = 0;
    else if (relay == 3 && button == 5) {
      relay3.relayState = "ON";
      relay3.flag = 1;
      RELAY3ON;
      relay3.autoTimer = 0;
    } else if (relay == 3 && button == 4) {
      relay3.relayState = "OFF";
      relay3.flag = 0;
      RELAY3OFF;
    }
    if (relay == 4 && button == 3) {
      relay4.config = 1;
      relay4.autoTimer = 0;
    } else if (relay == 4 && button == 1)
      relay4.config = 0;
    else if (relay == 4 && button == 5) {
      relay4.relayState = "ON";
      relay4.flag = 1;
      RELAY4ON;
      relay4.autoTimer = 0;
    } else if (relay == 4 && button == 4) {
      relay4.relayState = "OFF";
      relay4.flag = 0;
      RELAY4OFF;
    } else if (relay == 5 && button == 1)
      OLEDConfig = 1;
    else if (relay == 5 && button == 3)
      OLEDConfig = 0;
    else if (relay == 5 && button == 5)
      OLEDStatus = 1;
    else if (relay == 5 && button == 4)
      OLEDStatus = 0;
    //PowerSaver Time Set
    else if (relay == 1 && button == 8)
      relay1.psTimerDelayOn=value*60*1000;
    else if (relay == 1 && button == 9)
      relay1.psTimerDelayOff=value*60*1000;
    else if (relay == 2 && button == 8)
      relay2.psTimerDelayOn=value*60*1000;
    else if (relay == 2 && button == 9)
      relay2.psTimerDelayOff=value*60*1000;
    else if (relay == 3 && button == 8)
      relay3.psTimerDelayOn=value*60*1000;
    else if (relay == 3 && button == 9)
      relay3.psTimerDelayOff=value*60*1000;
    else if (relay == 4 && button == 8)
      relay4.psTimerDelayOn=value*60*1000;
    else if (relay == 4 && button == 9)
      relay4.psTimerDelayOff=value*60*1000;
    //***************************************
    else if (relay == 1 && button == 2) {
      relay1.autoTimer = 1;
      relay1.lastAutoTimer = millis();
      relay1.autoTimerDelay = value * 60 * 1000;
    } else if (relay == 2 && button == 2) {
      relay2.autoTimer = 1;
      relay2.lastAutoTimer = millis();
      relay2.autoTimerDelay = value * 60 * 1000;
    } else if (relay == 3 && button == 2) {
      relay3.autoTimer = 1;
      relay3.lastAutoTimer = millis();
      relay3.autoTimerDelay = value * 60 * 1000;
    } else if (relay == 4 && button == 2) {
      relay4.autoTimer = 1;
      relay4.lastAutoTimer = millis();
      relay4.autoTimerDelay = value * 60 * 1000;
    } 
    //POWER SAVER MODE ENABLE DISABLE
    else if (relay == 1 && button == 6) {
      relay1.autoTimer = 0;  // turn off timer (if any)
      relay1.powerSave = 1;
      relay1.config = 2;
    } else if (relay == 1 && button == 7) 
      relay1.config = 1;  // revert to auto
    else if (relay == 2 && button == 6) {
      relay2.autoTimer = 0;  // turn off timer (if any)
      relay2.powerSave = 1;
      relay2.config = 2;
    } else if (relay == 2 && button == 7) 
      relay2.config = 1;  // revert to auto
    else if (relay == 3 && button == 6) {
      relay3.autoTimer = 0;  // turn off timer (if any)
      relay3.powerSave = 1;
      relay3.config = 2;
    } else if (relay == 3 && button == 7) 
      relay3.config = 1;  // revert to auto
    else if (relay == 4 && button == 6) {
      relay4.autoTimer = 0;  // turn off timer (if any)
      relay4.powerSave = 1;
      relay4.config = 2;
    } else if (relay == 4 && button == 7) 
      relay4.config = 1;  // revert to auto
    //***************************************************
    request->send(200, "text/plain", "OK");
  });

  // Send a TIME request to <ESP_IP>/time?state=<inputMessage1>
  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage1;
    // GET input1 value on <ESP_IP>/time?state=<inputMessage1>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_3)->value();
      int x = inputMessage1.toInt();
      if (x == 1) {
        String y = String(Clock.getHour(h12Flag, pmFlag)) + ":" + String(Clock.getMinute()) + ":" + String(int(Clock.getTemperature())) + ":" + String(Clock.getDate()) + ":" + String(Clock.getMonth(century)) + ":" + String(Clock.getYear()) + ":" + String(Clock.getDoW());
        request->send(200, "text/plain", y);
      }
    } else
      inputMessage1 = "No message sent";
  });

  // Send an update time request to <ESP_IP>/updateTime?state=<inputMessage1>
  server.on("/updateTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage1;
    int x;
    // <ESP_IP>/updateTime?state=<inputMessage1>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_3)->value();
      x = inputMessage1.toInt();
    }
    if (x == 1) {
      updateTime = 1;
    }
    request->send(200, "text/plain", "Time will be updated. ID: " + String(x));
  });

  // Send an update time request to <ESP_IP>/updateTime?state=<inputMessage1>
  server.on("/codeUpdate", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage1,data="";
    // <ESP_IP>/updateTime?state=<inputMessage1>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_3)->value();
      data = processor(inputMessage1);
    }    
    request->send(200, "text/plain", data);
  });
 
  // Start server
  server.begin();


  // Init ESP-NOW
  if (esp_now_init() != 0) {
    display.println(F("Error in ESP-NOW"));
    display.display();
    delay(6000);
    display.clearDisplay();
    return;
  }

  display.println(F("START ESP-NOW"));
    display.display();
    delay(2000);
    display.clearDisplay();

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  display.println(F("SETUP RELAYS"));
  display.display();
  delay(2000);
  display.clearDisplay();

  pinMode(relayPin1, OUTPUT);
  delay(500);
  pinMode(relayPin2, OUTPUT);
  delay(500);
  pinMode(relayPin3, OUTPUT);
  delay(500);
  pinMode(relayPin4, OUTPUT);
  delay(500);
  //relayInitialize();
}

void loop() {

  ArduinoOTA.handle();
  //****************************************RELAY POWERSAVE FEATURE********************************
  // RELAY 1
  if (relay1.powerSave == 1) {
    relay1.psTimerDelay = relay1.psTimerDelayOn;  // 10 mins
    relay1.pslastTime = millis();
    RELAY1ON;
    relay1.powerSave = 0;
    relay1.psAlt = 1;

    relay1.flag = 1;
    relay1.relayState = "ON";
    relay1.config = 2;
  }

  if (relay1.psAlt == 1 && relay1.config == 2) {
    if ((millis() - relay1.pslastTime) > relay1.psTimerDelay) {
      RELAY1OFF;
      relay1.psAlt = 0;
      relay1.psTimerDelay = relay1.psTimerDelayOff;
      relay1.pslastTime = millis();

      relay1.flag = 0;
      relay1.relayState = "OFF";
    }
  } else if (relay1.psAlt == 0 && relay1.config == 2) {
    if ((millis() - relay1.pslastTime) > relay1.psTimerDelay) {
      RELAY1ON;
      relay1.psAlt = 1;
      relay1.psTimerDelay = relay1.psTimerDelayOn;
      relay1.pslastTime = millis();

      relay1.flag = 1;
      relay1.relayState = "ON";
    }
  }

  // RELAY 2
  if (relay2.powerSave == 1) {
    relay2.psTimerDelay = relay2.psTimerDelayOn;  
    relay2.pslastTime = millis();
    RELAY2ON;
    relay2.powerSave = 0;
    relay2.psAlt = 1;

    relay2.flag = 1;
    relay2.relayState = "ON";
    relay2.config = 2;
  }

  if (relay2.psAlt == 1 && relay2.config == 2) {
    if ((millis() - relay2.pslastTime) > relay2.psTimerDelay) {
      RELAY2OFF;
      relay2.psAlt = 0;
      relay2.psTimerDelay = relay2.psTimerDelayOff;
      relay2.pslastTime = millis();

      relay2.flag = 0;
      relay2.relayState = "OFF";
    }
  } else if (relay2.psAlt == 0 && relay2.config == 2) {
    if ((millis() - relay2.pslastTime) > relay2.psTimerDelay) {
      RELAY2ON;
      relay2.psAlt = 1;
      relay2.psTimerDelay = relay2.psTimerDelayOn;
      relay2.pslastTime = millis();

      relay2.flag = 1;
      relay2.relayState = "ON";
    }
  }

  // RELAY 3
  if (relay3.powerSave == 1) {
    relay3.psTimerDelay = relay3.psTimerDelayOn;  
    relay3.pslastTime = millis();
    RELAY3ON;
    relay3.powerSave = 0;
    relay3.psAlt = 1;

    relay3.flag = 1;
    relay3.relayState = "ON";
    relay3.config = 2;
  }

  if (relay3.psAlt == 1 && relay3.config == 2) {
    if ((millis() - relay3.pslastTime) > relay3.psTimerDelay) {
      RELAY3OFF;
      relay3.psAlt = 0;
      relay3.psTimerDelay = relay3.psTimerDelayOff;
      relay3.pslastTime = millis();

      relay3.flag = 0;
      relay3.relayState = "OFF";
    }
  } else if (relay3.psAlt == 0 && relay3.config == 2) {
    if ((millis() - relay3.pslastTime) > relay3.psTimerDelay) {
      RELAY3ON;
      relay3.psAlt = 1;
      relay3.psTimerDelay = relay3.psTimerDelayOn;
      relay3.pslastTime = millis();

      relay3.flag = 1;
      relay3.relayState = "ON";
    }
  }

  // RELAY 4
  if (relay4.powerSave == 1) {
    relay4.psTimerDelay = relay4.psTimerDelayOn;  
    relay4.pslastTime = millis();
    RELAY4ON;
    relay4.powerSave = 0;
    relay4.psAlt = 1;

    relay4.flag = 1;
    relay4.relayState = "ON";
    relay4.config = 2;
  }

  if (relay4.psAlt == 1 && relay4.config == 2) {
    if ((millis() - relay4.pslastTime) > relay4.psTimerDelay) {
      RELAY4OFF;
      relay4.psAlt = 0;
      relay4.psTimerDelay = relay4.psTimerDelayOff;
      relay4.pslastTime = millis();

      relay4.flag = 0;
      relay4.relayState = "OFF";
    }
  } else if (relay4.psAlt == 0 && relay4.config == 2) {
    if ((millis() - relay4.pslastTime) > relay4.psTimerDelay) {
      RELAY4ON;
      relay4.psAlt = 1;
      relay4.psTimerDelay = relay4.psTimerDelayOn;
      relay4.pslastTime = millis();

      relay4.flag = 1;
      relay4.relayState = "ON";
    }
  }
  //**************************************END RELAY POWER SAVE FEATURE****************************************

  if (updateTime == 1) {
    updateRTC();
    updateTime = 0;
  }

  display.clearDisplay();

  // checking timers for relays
  if ((millis() - lastTime) > timerDelay) {

    if (relay1.config == 0) {
    } else if (relay1.config == 1) {
      if (relay1.activate) {
        checkTimeFor(relay1.onTime,relay1.offTime,relay1.count);
      }
    }
    if (relay2.config == 0) {
    } else if (relay2.config == 1) {
      if (relay2.activate) {
        checkTimeFor(relay2.onTime,relay2.offTime,relay2.count);
      }
    }
    if (relay3.config == 0) {
    } else if (relay3.config == 1) {
      if (relay3.activate) {
        checkTimeFor(relay3.onTime,relay3.offTime,relay3.count);
      }
    }
    if (relay4.config == 0) {
    } else if (relay4.config == 1) {
      if (relay4.activate) {
        checkTimeFor(relay4.onTime,relay4.offTime,relay4.count);
      }
    }
    lastTime = millis();
  }

  // autoTimer checkings
  if (relay1.autoTimer == 1) {
    if ((millis() - relay1.lastAutoTimer) > relay1.autoTimerDelay) {
      relay1.flag = 1;
      RELAY1ON;
      relay1.relayState = "ON";
      relay1.autoTimer = 0;
    }
  }
  if (relay2.autoTimer == 1) {
    if ((millis() - relay2.lastAutoTimer) > relay2.autoTimerDelay) {
      relay2.flag = 1;
      RELAY2ON;
      relay2.relayState = "ON";
      relay2.autoTimer = 0;
    }
  }
  if (relay3.autoTimer == 1) {
    if ((millis() - relay3.lastAutoTimer) > relay3.autoTimerDelay) {
      relay3.flag = 1;
      RELAY3ON;
      relay3.relayState = "ON";
      relay3.autoTimer = 0;
    }
  }
  if (relay4.autoTimer == 1) {
    if ((millis() - relay4.lastAutoTimer) > relay4.autoTimerDelay) {
      relay4.flag = 1;
      RELAY4ON;
      relay4.relayState = "ON";
      relay4.autoTimer = 0;
    }
  }

  // checking wifi strength
  if ((millis() - lastTime1) > timerDelay1) {
    showTime();
    showWifiSignal();  // at (112,0), wifi sampling once in 900 m.seconds
    relayStatusPrinter();

    if (OLEDConfig == 0)  // Automatic/Manual Control of OLED Display
    {

      if (OLEDStatus == 0) {
        display.clearDisplay();
        display.display();
      } else
        display.display();
    } else if (OLEDConfig == 1) {
      byte currTime = Clock.getHour(h12Flag, pmFlag);  // stores current hour temporarily
      if (OLED_OFF > OLED_ON) {
        if ((currTime >= OLED_ON) && (currTime <= OLED_OFF))
          {display.display();
          OLEDStatus=1;}
        else {
          OLEDStatus=0;
          display.clearDisplay();
          display.display();
        }
      } else {
        if (((currTime >= OLED_ON) && (currTime >= OLED_OFF)) || ((currTime <= OLED_ON) && (currTime <= OLED_OFF)))  // oled display turn of at night
          {display.display();
          OLEDStatus=1;}
        else {
           OLEDStatus=0;
          display.clearDisplay();
          display.display();
        }
      }
    }

    lastTime1 = millis();
  }

  // ESP NOW Send time
  if ((millis() - lastTime2) > timerDelay2) {
    // Set values to send
    myData.time[0] = Clock.getHour(h12Flag, pmFlag);
    myData.time[1] = Clock.getMinute();
    myData.time[2] = Clock.getSecond();
    myData.time[3] = int(Clock.getTemperature());
    myData.time[4] = Clock.getDate();
    myData.time[5] = Clock.getMonth(century);
    myData.time[6] = Clock.getYear();
    myData.time[7] = Clock.getDoW();

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    lastTime2 = millis();
  }
}

void showWifiSignal() {
  int x = WiFi.RSSI();
  if (WiFi.status() != WL_CONNECTED) {
    display.setCursor(112, 0);
    // display.setTextSize(2);
    display.println(F("!"));
  } else {
    if (x <= (-80)) {
      display.drawBitmap(112, 0, wifiLow, 16, 16, WHITE);  // worst signal
    } else if ((x <= (-70)) && (x > (-80))) {
      display.drawBitmap(112, 0, wifiHalf, 16, 16, WHITE);  // poor signal
    } else if ((x <= (-60)) && (x > (-70)))                 // good signal
    {
      display.drawBitmap(112, 0, wifiMed, 16, 16, WHITE);  // best signal
    } else if (x > (-60)) {
      display.drawBitmap(112, 0, wifiFull, 16, 16, WHITE);  // excellent signal
    }
  }
}

void showTime() {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  if (Clock.getHour(h12Flag, pmFlag) < 10)
    display.print(0, DEC);

  display.print(Clock.getHour(h12Flag, pmFlag));  // Time
  display.print(":");
  if (Clock.getMinute() < 10)
    display.print(0, DEC);
  display.print(Clock.getMinute());
  // display.print(":");
  // if (Clock.getSecond() < 10)
  // display.print(0, DEC);
  // display.println(Clock.getSecond());
  display.println(" ");

  display.setTextSize(1);
  display.print(week[Clock.getDoW()]);
  display.print(", ");
  display.print(Clock.getDate());
  display.print("-");
  display.print(Clock.getMonth(century));
  display.print("-20");
  display.println(Clock.getYear());

  display.setCursor(90, 25);
  display.print(Clock.getTemperature(), 1);
  display.println(" C");

  if (Clock.getYear() == 70)
    updateRTC();
}

void updateRTC() {
  display.clearDisplay();
  display.setCursor(30, 25);    // Start at top-left corner
  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text

  display.println(F("Updating Time"));
  display.display();
  delay(2000);
  timeClient.update();
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti;
  ti = localtime(&rawtime);

  uint16_t year = ti->tm_year + 1900;
  uint8_t x = year % 10;
  year = year / 10;
  uint8_t y = year % 10;
  year = y * 10 + x;

  uint8_t month = ti->tm_mon + 1;

  uint8_t day = ti->tm_mday;

  uint8_t hours = ti->tm_hour;

  uint8_t minutes = ti->tm_min;

  uint8_t seconds = ti->tm_sec;

  uint8_t dow = ti->tm_wday;

  Clock.setClockMode(false);  // set to 24h
  // setClockMode(true); // set to 12h

  Clock.setYear(year);
  Clock.setMonth(month);
  Clock.setDate(day);
  Clock.setDoW(dow);
  Clock.setHour(hours);
  Clock.setMinute(minutes);
  Clock.setSecond(seconds);
}

void checkTimeFor(int onTime, int offTime, int number) {
  int h = Clock.getHour(h12Flag, pmFlag);
  int m = Clock.getMinute();

  int timeString = h * 100 + m;  // if h=12 and m=23 then 12*100 + 23 = 1223 hours

  if (offTime > onTime)  // when off timing is greater than on timing
  {
    if ((timeString > onTime) && (timeString < offTime)) {
      if (number == 1) {
        relay1.flag = 1;
        RELAY1ON;
        relay1.relayState = "ON";
        relay1.autoTimer = 0;
      } else if (number == 2) {
        relay2.flag = 1;
        RELAY2ON;
        relay2.relayState = "ON";
        relay2.autoTimer = 0;
      } else if (number == 3) {
        relay3.flag = 1;
        RELAY3ON;
        relay3.relayState = "ON";
        relay3.autoTimer = 0;
      } else if (number == 4) {
        relay4.flag = 1;
        RELAY4ON;
        relay4.relayState = "ON";
        relay4.autoTimer = 0;
      }
    } else {
      if (number == 1) {
        relay1.flag = 0;
        RELAY1OFF;
        relay1.relayState = "OFF";
      } else if (number == 2) {
        relay2.flag = 0;
        RELAY2OFF;
        relay2.relayState = "OFF";
      } else if (number == 3) {
        relay3.flag = 0;
        RELAY3OFF;
        relay3.relayState = "OFF";
      } else if (number == 4) {
        relay4.flag = 0;
        RELAY4OFF;
        relay4.relayState = "OFF";
      }
    }
  } else {
    if (((timeString > onTime) && (timeString > offTime)) || ((timeString < onTime) && (timeString < offTime))) {
      if (number == 1) {
        relay1.flag = 1;
        RELAY1ON;
        relay1.relayState = "ON";
      } else if (number == 2) {
        relay2.flag = 1;
        RELAY2ON;
        relay2.relayState = "ON";
      } else if (number == 3) {
        relay3.flag = 1;
        RELAY3ON;
        relay3.relayState = "ON";
      } else if (number == 4) {
        relay4.flag = 1;
        RELAY4ON;
        relay4.relayState = "ON";
      }
    } else {
      if (number == 1) {
        relay1.flag = 0;
        RELAY1OFF;
        relay1.relayState = "OFF";
      } else if (number == 2) {
        relay2.flag = 0;
        RELAY2OFF;
        relay2.relayState = "OFF";
      } else if (number == 3) {
        relay3.flag = 0;
        RELAY3OFF;
        relay3.relayState = "OFF";
      } else if (number == 4) {
        relay4.flag = 0;
        RELAY4OFF;
        relay4.relayState = "OFF";
      }
    }
  }
}

void relayStatusPrinter() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  if (relay1.flag) {
    display.setCursor(0, 57);
    display.print("R1");
  }
  if (relay2.flag) {
    display.setCursor(20, 57);
    display.print("R2");
  }

  if (relay3.flag) {
    display.setCursor(40, 57);
    display.print("R3");
  }

  if (relay4.flag) {
    display.setCursor(60, 57);
    display.print("R4");
  }
}
