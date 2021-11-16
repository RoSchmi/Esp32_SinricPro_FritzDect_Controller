#include <Arduino.h>
//#include "WiFiWebServer.h"
#include "ESPAsyncWebServer.h"
#include "defines.h"
#include "config_secret.h"
#include "config.h"
#include "SinricPro_Generic.h"
#include "SinricProSwitch.h"

#include "FreeRTOS.h"
#include "Esp.h"
#include "esp_task_wdt.h"
#include <rom/rtc.h>

#include "HTTPClient.h"
#include "RsArduinoFritzApi.h"

// Default Esp32 stack size of 8192 byte is not enough for some applications.
// --> configure stack size dynamically from code to 16384
// https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/4
// Patch: Replace C:\Users\thisUser\.platformio\packages\framework-arduinoespressif32\cores\esp32\main.cpp
// with the file 'main.cpp' from folder 'patches' of this repository, then use the following code to configure stack size
#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif


RESET_REASON resetReason_0;
RESET_REASON resetReason_1;

uint8_t lastResetCause = 0;

const char *ssid = IOT_CONFIG_WIFI_SSID;
const char *password = IOT_CONFIG_WIFI_PASSWORD;

bool powerState1 = false;
bool powerState2 = false;
bool powerState3 = false;

typedef struct
{
   bool actState = false;
   bool lastState = true;
}ButtonState;

//RoSchmi
#define GPIOPin 0
#define BUTTON_1 GPIOPin
#define BUTTON_2 GPIOPin

#define WIO_KEY_A GPIOPin
#define WIO_KEY_B GPIOPin
#define WIO_KEY_C GPIOPin

bool buttonPressed = false;

ButtonState buttonState1;
ButtonState buttonState2;
ButtonState buttonState3;

#define LED_BUILTIN 2

int LED1_PIN = LED_BUILTIN;
int LED2_PIN = LED_BUILTIN;
int LED3_PIN = LED_BUILTIN;

typedef struct 
{      
  // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;
  int index;
} deviceConfig_t;



// this is the main configuration
// please put in your deviceId, the PIN for Relay and PIN for flipSwitch and an index to address the entry
// this can be up to N devices...depending on how much pin's available on your device ;)
// right now we have 2 devicesIds going to 1 LED and 2 flip switches (Wio Terminal left and middle Button)

std::map<String, deviceConfig_t> devices =
{
  //{deviceId, {relayPIN,  flipSwitchPIN, index}}
  // You have to set the pins correctly. 

  { SWITCH_ID_1, {  (int)LED_BUILTIN,  (int)BUTTON_1, 0}},
  { SWITCH_ID_2, {  (int)LED_BUILTIN, (int)BUTTON_2, 1}} 
};

bool onPowerState1(const String &deviceId, bool &state)
{
  Serial.printf("Device 1 turned %s\r\n", state ? "on" : "off");
  // RoSchmi
  //int LED1_PIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device
  //digitalWrite(LED1_PIN, state);             // set the new relay state

  powerState1 = state;
  //RoSchmi
  //digitalWrite(LED1_PIN, powerState1 ? HIGH : LOW);
  /*
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique12pt7b);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("WIFI :", 10 , 5);
  spr.setTextColor(TFT_GREEN, TFT_BLACK);
  spr.drawString("connected", 100 , 5);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("Device1: ", 10, 65);
  spr.drawString("Device2: ", 10, 105);
  //spr.drawString("Device3: ", 10, 145);
  tft.setTextColor(state ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
  tft.drawString(state ? " turned on" : " turned off", 120 , 70, 4);
  */
  return true; // request handled properly
}

bool onPowerState2(const String &deviceId, bool &state)
{
  Serial.printf("Device 2 turned %s\r\n", state ? "on" : "off");
  //RoSchmi
  //int LED2_PIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device
  //digitalWrite(LED2_PIN, state);             // set the new relay state

  powerState2 = state;
  //RoSchmi
  //digitalWrite(LED2_PIN, powerState2 ? HIGH : LOW);
  /*
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique12pt7b);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("WIFI :", 10 , 5);
  spr.setTextColor(TFT_GREEN, TFT_BLACK);
  spr.drawString("connected", 100 , 5);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("Device1: ", 10, 65);
  spr.drawString("Device2: ", 10, 105);
  //spr.drawString("Device3: ", 10, 145);
  tft.setTextColor(state ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
  tft.drawString(state ? " turned on" : " turned off", 120 , 110, 4);
  */
  return true; // request handled properly
  
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.println( String(deviceId) + String(state ? " on" : " off"));
  
  //RoSchmi
  //int relayPIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device
  //digitalWrite(relayPIN, state);             // set the new relay state
  /*
  switch (devices[deviceId].index)
  {
    case 0:
    {
      onPowerState1(deviceId, state);
    }
    break;
    case 1:
    {
      onPowerState2(deviceId, state);
    }
    break;
    case 2:
    {
      //onPowerState3(deviceId, state);
    }
    break;
    case 3:
    {
      //onPowerState4(deviceId, state);
    }
    break;
    default:
    {}
  }
  */
  return true;
}


void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];    //temp
    mySwitch1.onPowerState(onPowerState1);

    SinricProSwitch& mySwitch2 = SinricPro[SWITCH_ID_2];    //light
    mySwitch2.onPowerState(onPowerState2);

    //SinricProSwitch& mySwitch3 = SinricPro[SWITCH_ID_1];    //humi
    //mySwitch3.onPowerState(onPowerState3);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void GPIOPinISR()
{
  buttonPressed = true;
}

void handleButtonPress()
{
  
  if (digitalRead(WIO_KEY_C) == LOW)
  {    
     buttonState1.lastState = buttonState1.actState;
     buttonState1.actState = true;
     if (buttonState1.actState != buttonState1.lastState)
     {      
        powerState1 = !powerState1;
        /*
        spr.fillSprite(TFT_BLACK);
        spr.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.setTextColor(powerState1 ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);    
        tft.drawString(powerState1 ? " turned on" : " turned off", 120 , 70, 4);
        */
        digitalWrite(LED1_PIN, powerState1 ? HIGH : LOW);
        // get Switch device back
        SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
        // send powerstate event      
        mySwitch1.sendPowerStateEvent(powerState1); // send the new powerState to SinricPro server
        Serial.print("Device ");
        Serial.print(mySwitch1.getDeviceId().toString());
        Serial.print(powerState1 ? "turned on" : "turned off");
        Serial.println(" (manually via flashbutton)");
      }
      else
      {
        buttonState1.actState = false;
      }
  }

  if (digitalRead(WIO_KEY_B) == LOW)
  {   
     buttonState2.lastState = buttonState2.actState;
     buttonState2.actState = true;
     if (buttonState2.actState != buttonState2.lastState)
     {    
        powerState2 = !powerState2;
        /*
        spr.fillSprite(TFT_BLACK);
        spr.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.setTextColor(powerState2 ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
        tft.drawString(powerState2 ? " turned on" : " turned off", 120 , 110, 4);
        */
        digitalWrite(LED2_PIN, powerState2 ? HIGH : LOW);
        // get Switch device back
        SinricProSwitch& mySwitch2 = SinricPro[SWITCH_ID_2];
        // send powerstate event     
        mySwitch2.sendPowerStateEvent(powerState2); // send the new powerState to SinricPro server
        Serial.print("Device ");
        Serial.print(mySwitch2.getDeviceId().toString());
        Serial.print(powerState2 ? "turned on" : "turned off");
        Serial.println(" (manually via flashbutton)");
      }
  }
  else
  {
    buttonState2.actState = false;
  }
  
}

#if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
  #else
    static WiFiClient wifi_client;
  #endif



HTTPClient http;
static HTTPClient * httpPtr = &http;

//Protocol protocol = Protocol::useHttps;
Protocol protocol = Protocol::useHttp;

/*
#if TRANSPORT_PROTOCOL == 1    
    static EthernetHttpClient  httpClient(sslClient, (char *)FRITZ_IP_ADDRESS, (TRANSPORT_PROTOCOL == 0) ? 80 : 443);
    Protocol protocol = Protocol::useHttps;
  #else
    static EthernetHttpClient  httpClient(client, (char *)FRITZ_IP_ADDRESS, (TRANSPORT_PROTOCOL == 0) ? 80 : 443);
    Protocol protocol = Protocol::useHttp;
  #endif
  */  

  //FritzApi fritz((char *)FRITZ_USER, (char *)FRITZ_PASSWORD, FRITZ_IP_ADDRESS, protocol, &client, &sslClient, &httpClient);
   //FritzApi fritz((char *)FRITZ_USER, (char *)FRITZ_PASSWORD, FRITZ_IP_ADDRESS, protocol, wifi_client, wifi_client, http);
   FritzApi fritz((char *)FRITZ_USER, (char *)FRITZ_PASSWORD, FRITZ_IP_ADDRESS, protocol, wifi_client, wifi_client, httpPtr);

// forward declarations
void print_reset_reason(RESET_REASON reason);

void setup() {
  Serial.begin(BAUD_RATE);
  //while(!Serial);
  
  
  
  
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);

  // Wait some time (3000 ms)
  uint32_t start = millis();
  while ((millis() - start) < 3000)
  {
    delay(10);
  }
  //RoSchmi
  Serial.println("\r\nStarting");
  



  resetReason_0 = rtc_get_reset_reason(0);
  resetReason_1 = rtc_get_reset_reason(1);
  lastResetCause = resetReason_1;
  Serial.printf("Last Reset Reason: CPU_0 = %u, CPU_1 = %u\r\n", resetReason_0, resetReason_1);
  Serial.println("Reason CPU_0: ");
  print_reset_reason(resetReason_0);
  Serial.println("Reason CPU_1: ");
  print_reset_reason(resetReason_1);

  delay(3000);

  Serial.print(F("\nStarting ConnectWPA on "));
  Serial.print(BOARD_NAME);
  Serial.print(F(" with "));
  Serial.println(SHIELD_TYPE); 
  //Serial.println(WIFI_WEBSERVER_VERSION);

  // Wait some time (3000 ms)
  start = millis();
  while ((millis() - start) < 3000)
  {
    delay(10);
  }
  
  //Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  Serial.println(F("First disconnecting, then\r\nConnecting to WiFi-Network"));
  
  while (WiFi.status() != WL_DISCONNECTED)
  {
    WiFi.disconnect();
    delay(200); 
  }
  WiFi.begin(ssid, password);

if (!WiFi.enableSTA(true))
{
  while (true)
  {
    // Stay in endless loop to reboot through Watchdog
    Serial.println("Connect failed.");
    delay(1000);
    }
}

#if USE_STATIC_IP == 1
  if (!WiFi.config(presetIp, presetGateWay, presetSubnet, presetDnsServer1, presetDnsServer2))
  {
    while (true)
    {
      // Stay in endless loop
    lcd_log_line((char *)"WiFi-Config failed");
      delay(3000);
    }
  }
  else
  {
    lcd_log_line((char *)"WiFi-Config successful");
    delay(1000);
  }
  #endif


  uint32_t tryConnectCtr = 0;
  while (WiFi.status() != WL_CONNECTED)
  {  
    delay(100);
    Serial.print((tryConnectCtr++ % 40 == 0) ? "\r\n" : "." );  
  }

  Serial.print(F("\r\nGot Ip-Address: "));
  Serial.println(WiFi.localIP());

  //setupSinricPro();

  /*
  String _ip = "fritz.box";
  Serial.printf("http://%s%s\r\n", _ip, "/login_sid.lua");
  Serial.print(_ip);
  Serial.println("/login_sid.lua");

  http.begin("http://" + String(_ip) + "/login_sid.lua");
  */

  Serial.println("Before init");


  if (fritz.init())
  {
    Serial.println("Initialization for FritzBox is done");
  }
  else
  {
    Serial.println("Initialization for FritzBox failed");
    while (true)
    {
      delay(200);
    }    
  }
  
  delay(1000);

  
  Serial.println("Before testSID");
  String actualSID = fritz.testSID();


  Serial.print("Actual SID is: ");
  
  Serial.println(actualSID);
  
  
  
}

void loop() { 
  String switchname = fritz.getSwitchName(FRITZ_DEVICE_AIN_01); 
  Serial.printf("%s%s", F("Name of device is: "), switchname.c_str());
  while (true)
  {
    Serial.println("Looping");
    delay(3000);
  }
  SinricPro.handle();
  delay(200);
  handleButtonPress();
}

void print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1, Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET");break;               /**<3, Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4, Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5, Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6, Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}