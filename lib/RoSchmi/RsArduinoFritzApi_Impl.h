// Copyright RoSchmi 2021, License MIT
// https://github.com/RoSchmi/Teensy/tree/master/Proj/Teens41_FritzBox_FritzDect

// This is an adaption/modification of:
// https://github.com/planetk/ArduinoFritzApi 

// For the MD5 hash calculation the used way was taken from
// https://github.com/schuppeste/Arduino-Fritzbox-Login

#include <Arduino.h>
#include "RsArduinoFritzApi.h"
//RoSchmi
//#include "EthernetHttpClient_SSL.h"

//RoSchmi
//FritzApi::FritzApi(const char* user, const char* password, const char * ip, Protocol protocol, EthernetClient * pClient, EthernetSSLClient * pSslClient, EthernetHttpClient * pHttp) {
  FritzApi::FritzApi(const char* user, const char* password, const char * ip, Protocol protocol, WiFiClient * pClient, WiFiClientSecure * pSslClient, HTTPClient * pHttp) {
  _user = user;
  _pwd = password;
  _ip = ip;
  _protocol = protocol; 
  _port = protocol == Protocol::useHttp ? 80 : 443;
  http = pHttp;
  client = pClient;
  sslClient = pSslClient;
}

FritzApi::~FritzApi(){
  _user="";
  _pwd="";
  _ip="";
  _protocol = Protocol::useHttps;
  _port = 443;
  http = nullptr;
  client = nullptr;
  sslClient = nullptr;
}

bool FritzApi::init() 
{
  // Gets challenge and MD5 encoded Password hash
  // response is <Challenge>-<MD5-Hash>
  String response = getChallengeResponse(); 
  if (response == "")
  {
    return false;
  }
  _sid = getSID(response);
  Serial.println("SID: " + _sid);
  if (_sid == "") 
  {
    Serial.println("FRITZ_ERR_EMPTY_SID");
    return false;
	  //throw FRITZ_ERR_EMPTY_SID;
  } 
  else if (_sid == "0000000000000000") 
  {
    Serial.println("FRITZ_ERR_INVALID_SID");
    return false;
  	//throw FRITZ_ERR_INVALID_SID;   	
  }
  return true;  
}

String FritzApi::getChallengeResponse() 
{
  http->connectionKeepAlive();
  char port[10] {0};
  itoa(_port, port, 10);
  Serial.printf("%s%s%s", "Port = ", port, "\r\n");
  
  if (!(http->connect((char *)_ip, _port)))
  {
    Serial.println(F("Not connected"));
    return "";
  }  
    int retCode = http->get("/login_sid.lua");
    if (retCode != 0)
    {
       Serial.println(F("Error on http communication"));
       return "";
    }
    int statusCode = http->responseStatusCode(); 
    String result = "";
    if (statusCode != 200)
    {
      Serial.println(F("FRITZ_ERR_NO_CHALLENGE"));
      result = http->responseBody();     
      Serial.println(result);
      return "";
    }
    else
    {
      result = http->responseBody();
      //Serial.print("Challenge response: ");
      //Serial.println(result.substring(0, 300 < (result.length() -1) ? 300 :  result.length() -1));
      //Serial.println();
      String szblockTime = result.substring(result.indexOf("<BlockTime>") + 11, result.indexOf("</BlockTime>"));
      int blockTime = atoi(szblockTime.c_str());     
      if (blockTime > 0)
      {
        delay(blockTime * 1000);
      }
      String challenge = result.substring(result.indexOf("<Challenge>") + 11, result.indexOf("</Challenge>"));     
      
      // Serial.printf("%s%s%s", "Challenge = ", challenge.c_str(), "\r\n");
      
      String challengeResponse = challenge + "-" + String(_pwd);
      
      //Serial.printf("%s%s%s", "<Challenge>-<PWD>: ", challengeResponse.c_str(), "\r\n");
      
      String responseHash = "";     
           
      char meinMD5String[150] {0};

      challengeResponse.toCharArray(meinMD5String, challengeResponse.length() + 1);
      //int challengeResponseLength = strlen(meinMD5String);
          
      //Nach 16Bit Konvertieren
      int i = 0;
      size_t x = 0;
      while ( x < strlen(meinMD5String))
      {
        mynewbytes[i] = meinMD5String[x];
        i++;
        mynewbytes[i] = 0x00;
        i++;
        x++;
      }       
      http->stop();

      //MD5 Verarbeitung mit chid+passwort MD5
      //https://github.com/tzikis/ArduinoMD5/blob/master/examples/MD5_Hash/MD5_Hash.ino
      //https://github.com/tzikis/ArduinoMD5/
        
      unsigned char* hash = MD5::make_hash((char*)mynewbytes, (size_t)(strlen((char *)meinMD5String) * 2));
      char *md5str = MD5::make_digest(hash, 16);
      free(hash);

      return challenge + "-" + md5str; 
   } 
}
  
String FritzApi::getSID(String response) 
{
  
  Serial.printf("%s%s%s", F("ConnectionState: "), http->connected(), "\r\n");
   
  char augUrlPath[140] {0};
  sprintf((char *)augUrlPath, "%s%s%s%s", "/login_sid.lua?username=", _user, "&response=", (char *)response.c_str());
  
  //Serial.printf("%s%s%s", "SID-Request: ", augUrlPath, "\r\n");
  
    // Try 3 times to connect
  if (!(http->connected()))
  {
     for (int i = 0; i < 3; i++)
     {
        if (http->connect((char *)_ip, _port))
        {      
           break; 
        }
        else
        {
           Serial.println("Couldn't connect");
           return "";
        }
     }
  }

  http->connectionKeepAlive();

  int retCode = http->get(augUrlPath); 
  if (retCode != 0)
  {
    Serial.println(F("Error on http communication"));
    return "";
  }
  int statusCode = http->responseStatusCode();
  
  if (statusCode != 200) 
  {
    Serial.println(F("FRITZ_ERR_NO_SID"));
    return "";
  }
  String result = http->responseBody();
  
  //Serial.print("SID-Response: ");
  //Serial.println(result.substring(0, 500 < (result.length() -1) ? 500 :  result.length() -1));
  
  http->stop();

  // TODO: check indexOf > 0
  String sid = result.substring(result.indexOf("<SID>") + 5,  result.indexOf("</SID>"));
  return sid;
}









String FritzApi::testSID()
{ 
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s", "sid=", (char *)_sid.c_str());
  String result = executeRequest(login_sidService, cmdSuffix);
  String sid = result.substring(result.indexOf("<SID>") + 5,  result.indexOf("</SID>"));
  return sid;
} 

String FritzApi::getSwitchName(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=getswitchname&sid=", (char *)_sid.c_str()); 
  return executeRequest(homeautoswitchService, cmdSuffix);
}


boolean FritzApi::getSwitchPresent(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=getswitchpresent&sid=", (char *)_sid.c_str()); 

  String result = executeRequest(homeautoswitchService, cmdSuffix);
  return (atoi(result.c_str()) == 1);
}

boolean FritzApi::setSwitchOn(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=setswitchon&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService, cmdSuffix);
  return (atoi(result.c_str()) == 1);
}

boolean FritzApi::setSwitchOff(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=setswitchoff&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService, cmdSuffix); 
  return (atoi(result.c_str()) == 1);
}

boolean FritzApi::setSwitchToggle(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=setswitchtoggle&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService, cmdSuffix);
  return (atoi(result.c_str()) == 1);
}

boolean FritzApi::getSwitchState(String ain) 
{  
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=getswitchstate&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService , cmdSuffix); 
  return (atoi(result.c_str()) == 1);
}

double FritzApi::getSwitchPower(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=getswitchpower&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService , cmdSuffix);
  if (result == "inval") {
  	//throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return atof(result.c_str()) / 1000;
}


int FritzApi::getSwitchEnergy(String ain) {
  char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=getswitchenergy&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService , cmdSuffix);
  if (result == "inval") {
    return 0;
  	//throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return atoi(result.c_str());
}


double FritzApi::getTemperature(String ain) {
 char cmdSuffix[100] {0};
  sprintf((char *)cmdSuffix, "%s%s%s%s", "ain=", (char *)ain.c_str(), "&switchcmd=gettemperature&sid=", (char *)_sid.c_str()); 
  String result = executeRequest(homeautoswitchService , cmdSuffix);


 //String result = executeRequest("gettemperature&sid=" + _sid + "&ain=" + String(ain));
  if (result == "inval") {
  	//throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return atof(result.c_str()) / 10;
}

String FritzApi::executeRequest(String service, String command) 
{
  String result;
  char aUrlPath[140] {0};
  int httpStatus = -1;
  
  //sprintf((char *)aUrlPath, "%s%s%s%s", "http://", _ip, (char *)service.c_str(), (char *)command.c_str());
  sprintf((char *)aUrlPath, "%s%s", (char *)service.c_str(), (char *)command.c_str());        
  //Serial.println(aUrlPath);

   
  //http->addHeader
  http->connectionKeepAlive();
  http->noDefaultRequestHeaders();
    
    // Try 3 times to connect
  if (!(http->connected()))
  {
     for (int i = 0; i < 3; i++)
     {
        if (http->connect((char *)_ip, _port))
        {      
           break; 
        }
        else
        {
           Serial.println("Couldn't connect");
        }
     }
  }
  
  if (http->connected())
  {
    http->beginRequest();
    http->get(aUrlPath);
    http->sendHeader("Host", _ip);
    http->endRequest();
    
    httpStatus = http->responseStatusCode();
    //Serial.println("Status Code is: ");
    //Serial.println(httpStatus);
    result = http->responseBody();

    http->stop();
    
    if (httpStatus == 200) 
    {     
      return result;
    } 
    else 
    {
      Serial.println("Trying to get new session");
      // Try to get new Session
      init();
      return result;     
    }
  }
  else
  {
    Serial.println("not connected");
    return "Not connected";
  } 
}

// Not implemented/adapted functions for thermostat
/*
double FritzApi::getThermostatNominalTemperature(String ain) {
  String result = executeRequest("gethkrtsoll&sid=" + _sid + "&ain=" + String(ain));
  if (result == "inval") {
  	throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return convertTemperature(result);
}
*/

/*
double FritzApi::getThermostatComfortTemperature(String ain) {
  String result = executeRequest("gethkrkomfort&sid=" + _sid + "&ain=" + String(ain));
  if (result == "inval") {
  	throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return convertTemperature(result);
}
*/

/*
double FritzApi::getThermostatReducedTemperature(String ain) {
  String result = executeRequest("gethkrabsenk&sid=" + _sid + "&ain=" + String(ain));
  if (result == "inval") {
  	throw FRITZ_ERR_VALUE_NOT_AVAILABLE;
  }
  return convertTemperature(result);
}
*/

/*
double FritzApi::setThermostatNominalTemperature(String ain, double temp) {
  int param;
  if (temp == FRITZ_OFF_TEMPERATURE) {
    param=253;
  } else if (temp == FRITZ_ON_TEMPERATURE) {
    param=254;
  } else if (temp < 8.0f) {
    param=16;
  } else if (temp > 28.0f) {
    param=56;
  } else {
    param = 2*temp;
  }
  String result = executeRequest("sethkrtsoll&sid=" + _sid + "&ain=" + String(ain) + "&param=" + param);
  return temp;
}
*/

/*
double FritzApi::convertTemperature(String result) {
  double temp = atof(result.c_str());
  if (temp == 254) {
    return FRITZ_ON_TEMPERATURE;
  } else if (temp == 253 ){
    return FRITZ_OFF_TEMPERATURE;
  }
  return temp/2;
}
*/
