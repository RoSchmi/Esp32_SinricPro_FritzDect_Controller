#include <Arduino.h>
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
#include "WiFiClientSecure.h"

#include "WiFiClient.h"
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

// if a queue should be used to store commands see:
//https://techtutorialsx.com/2017/08/20/esp32-arduino-freertos-queues/

bool powerState1 = false;
bool powerState2 = false;

typedef struct
{
   bool actState = false;
   bool lastState = true;
}ButtonState;

//RoSchmi
#define GPIOPin 0
#define BUTTON_1 GPIOPin
#define BUTTON_2 GPIOPin

#define BootButton GPIOPin

bool buttonPressed = false;

volatile ButtonState bootButtonState;

#define LED_BUILTIN 2

int LED1_PIN = LED_BUILTIN;

typedef struct 
{      
  // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;
  int index;
} deviceConfig_t;

// Sinric Pro
// this is the main configuration
// please put in your deviceId, the PIN for Relay and PIN for flipSwitch and an index to address the entry
// this can be up to N devices...depending on how much pin's available on your device ;)
// right now we have 2 devicesIds going to 1 LED and 2 flip switches (set to the same button)

std::map<String, deviceConfig_t> devices =
{
  //{deviceId, {relayPIN,  flipSwitchPIN, index}}
  // You have to set the pins correctly.
  // In this App we used -1 when the relay pin shall be ignored 

  { SWITCH_ID_1, {  (int)LED1_PIN,  (int)BUTTON_1, 0}},
  { SWITCH_ID_2, {  (int)-1, (int)BUTTON_2, 1}} 
};

uint32_t millisAtLastAction;
uint32_t millisBetweenActions = 10000;

//X509Certificate myX509Certificate = baltimore_root_ca;
X509Certificate myX509Certificate = myfritzbox_root_ca;

#if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
    Protocol protocol = Protocol::useHttps;
  #else
    static WiFiClient wifi_client;
    Protocol protocol = Protocol::useHttp;
  #endif

HTTPClient http;
static HTTPClient * httpPtr = &http;

FritzApi fritz((char *)FRITZ_USER, (char *)FRITZ_PASSWORD, FRITZ_IP_ADDRESS, protocol, wifi_client, httpPtr, myX509Certificate);

// not used
void GPIOPinISR()
{
  buttonPressed = true;
}

// forward declarations
void print_reset_reason(RESET_REASON reason);
bool onPowerState(String deviceId, bool &state);
bool onPowerState2(const String &deviceId, bool state);
bool onPowerState1(const String &deviceId, bool state);
void setupSinricPro(bool restoreStates);
void handleButtonPress();

void setup() {
  Serial.begin(BAUD_RATE);
  //while(!Serial);

  pinMode(BootButton, INPUT_PULLUP);
  //attachInterrupt(BootButton, GPIOPinISR, FALLING);  // not used

  pinMode(LED1_PIN, OUTPUT);

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
  //If wanted, printout SID
  //Serial.printf("Actual SID is: %s\r\n", String(fritz.testSID()).c_str());

  bool restoreStatesFromServer = false;
  setupSinricPro(restoreStatesFromServer);

  delay(1000);

  if (!restoreStatesFromServer)
  { 
    bool actualSocketState = fritz.getSwitchState(FRITZ_DEVICE_AIN_01);
    
    onPowerState1(SWITCH_ID_1, actualSocketState);  // once more switch Fritz!Dect socket to actual state  

    // get Switch device back
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID_1];
    // send powerstate event      
    mySwitch.sendPowerStateEvent(actualSocketState); // send the actual powerState to SinricPro server
    Serial.println("State from Fritz!Dect was transmitted to server"); 
  }
  
  // Set time interval for repeating commands
  millisAtLastAction = millis();
  millisBetweenActions = 15000;
}

void loop() 
{ 
  if ((millis() - millisAtLastAction) > millisBetweenActions) // time interval expired?
  {
     millisAtLastAction = millis();
     String switchname = fritz.getSwitchName(FRITZ_DEVICE_AIN_01); 
    Serial.printf("Name of device is: %s", switchname.c_str());
  }
  SinricPro.handle();
  delay(200);
  handleButtonPress();
}

void handleButtonPress()
{
  if (digitalRead(BootButton) == LOW)
  {   
    bootButtonState.lastState = bootButtonState.actState;    
    bootButtonState.actState = true;
    if (bootButtonState.actState != bootButtonState.lastState)  // if has toggled
    {     
      if (onPowerState1(SWITCH_ID_1, !powerState1))             // switch Fritz!Dect socket
      {       
        // get Switch device back
        SinricProSwitch& mySwitch = SinricPro[SWITCH_ID_1];
        // send powerstate event      
        mySwitch.sendPowerStateEvent(powerState1); // send the new powerState to SinricPro server
        Serial.println("(Switched manually via flashbutton)");        
      }    
    }     
  }
  else
  {
    bootButtonState.actState = false;
  }
}

bool onPowerState(String deviceId, bool &state)
{
  bool returnResult = false;
  //Serial.println( String(deviceId) + String(state ? " on" : " off")); 
  switch (devices[deviceId].index)
  {
    case 0:
    {
      returnResult = onPowerState1(deviceId, state);
    }
    break;
    case 1:
    {
      returnResult = onPowerState2(deviceId, state);
    }
    break; 
    default:
    {}
  }
  return returnResult;
}

bool onPowerState1(const String &deviceId, bool state)
{
  bool returnResult = false;
  if (state == true)
  {
    returnResult = fritz.setSwitchOn(FRITZ_DEVICE_AIN_01);
  }
  else
  {
    returnResult = !fritz.setSwitchOff(FRITZ_DEVICE_AIN_01);
  }
  if (returnResult == true)
  {
    int relayPIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device
    if (relayPIN != -1)
    {
      digitalWrite(relayPIN, state);      // set the new relay state
    }
    Serial.printf("Device 1 turned %s\r\n", state ? "on" : "off");
    powerState1 = state;
    bootButtonState.actState = state;    
  }
  else
  {
    Serial.printf("Failed to turn Device 1 %s\r\n", state ? "on" : "off");
    //powerState1 = state;
    //bootButtonState.actState = state;  
  }    
  return returnResult; // request handled properly
}

bool onPowerState2(const String &deviceId, bool state)
{
  Serial.printf("Device 2 turned %s\r\n", state ? "on" : "off");
  powerState2 = state;
  return true; // request handled properly
}

// Create devices in Sinric Pro
// restoreStates: true means:
// restore the last states from the Sinric Server to this local device
void setupSinricPro(bool restoreStates)
{
  for (auto &device : devices)
  {
    // for each switch device defined in the map devices
    // create a SinricProSwitch instance with its deviceId
    const char *deviceId = device.first.c_str();
    // doesn't matter that the name is the same for all
    SinricProSwitch& mySwitch = SinricPro[deviceId];
    // we take the same callback for all and distinguish according to the index in the map    
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET); 
  // if true, restore the last states from the Sinric Server to this local device
  
  SinricPro.restoreDeviceStates(restoreStates);
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