# Aquarium Automation using NodeMCU ESP-12E V3.0

### What's New?
* Completely New WebServer UI
* Uses new Asynchronous Webserver
* Real time page update using JS XMLHTTPREQUEST
* Uses ESP-NOW to broadcast time to other ESPs without dedicated RTC modules
* Better organised code

FOR ANY KIND OF QUESTIONS OR QUERIES PLEASE CONTACT ME HERE: https://thingsbypatra.pythonanywhere.com/contact

Also, this code needs some optimisations (I believe that) so, if you have updated something and it works better than mine, then please let me know.

Using NodeMCU for aquarium lights, filter, skimmer, power head/wavemaker control with OTA updates, timers, auto time update, visual feedbacks and WiFi signal level icons.

Disclaimer: I take absolutely no responsibility for the use of this code. Use it with your own risk.

##It works as "smart switches" which turns something on or off automatically (by time or manually). This can be applied to varied of things and can be expanded as automatic dosers, automatic water changes, etc (limited by your imagination).
## Libraries (Specific few)
1. For DS3231 Library use https://github.com/NorthernWidget/DS3231
2. https://github.com/arduino-libraries/NTPClient

Rest of the libraries can be downloaded within Arduino IDE Libraries Manager, and some of them are already included (within Arduino). Please google for "how-to" if you cannot figure it.

Please add http://arduino.esp8266.com/stable/package_esp8266com_index.json in File --> Preferences --> Additional Boards Manager URLs to get support for all ESP Boards.

## Pin Configuration:

DS3231 and 128x64 OLED using I2C and are connected to

NodeMCU --> Device <br/>
D1 --> SCL <br/>
D2 --> SDA <br/>
3.3 --> VCC <br/>
G --> GND <br/>

Yes, connect both the I2C devices to the same pin (purpose of I2C). If you are facing I2C device address related issues then please Google it. It is very common and very easy to fix. You just need to adjust one or two resistor value.

For the 4 Channel relay board

NodeMCU --> Device <br/>
D3 --> In1 <br/>
D5 --> In2 <br/>
D6 --> In3 <br/>
D7 --> In4 <br/>

For GND and VCC, use appropriate separate power supply (don't take power from NodeMCU, may burn). If you are using separate powersupplies for NodeMCU and Relay, then make sure to connect both the GNDs of Node and Powersupply together, else the relay module won't work. If confused, take help from google or contact me.

## MAIN Parts: <br/>
<img src="https://m.media-amazon.com/images/I/71TWos73PrL._SL1100_.jpg" alt="Relay Board" width="200" height="200"> <br/>
4 Channel 5V Relay Board Module with Optocouplers <br/><br/>

<img src="https://m.media-amazon.com/images/I/41RP9FjC+jL.jpg" alt="DS3231" width="200" height="200"> <br/>
DS3231 AT24C32 IIC Precision RTC <br/><br/>

<img src="https://m.media-amazon.com/images/I/51lIrI5vnQL.jpg" alt="NodeMCU" width="200" height="200"> <br/>
NodeMCU-ESP8266 Development Board ESP12E <br/><br/>

<img src="https://www.electronicscomp.com/image/cache/catalog/13-inch-i2c-iic-oled-display-module-4pin-white-800x800.jpg" alt="OLED" width="200" height="200"> <br/>
1.3 Inch I2C IIC 128x64 OLED Display Module 4 Pin - White <br/><br/>

Other parts as required

## WEB SERVER

<img src="https://github.com/chikne97/Smart-Aquarium-V3.0/blob/main/demo2.png" alt="OLED" width="400" height="800"> <br/>
