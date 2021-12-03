# Esp32_SinricPro_FritzDect_Controller

Switching and monitoring of the switch states of a switchable Fritz!Dect 200 power socket ( for FritzBox ) via Sinric Pro

This is a simple and quite easy to understand appliction providing only switching of the socket.

 A more complex application which can be used to switch the power socket and read the power consumption is provided here:
 
 https://github.com/RoSchmi/Esp32_SinricPro_FritzDect_Switch_PowerReader

### What is Sinric Pro?
Sinric Pro is an internet based ecosystem mainly consisting of an internet portal, a cloud service, mobile Apps and SDKs to run connected applications on different microcontrollers and mini computers (like Esp32, Arduino, Raspberry Pi) Smart home systems like Alexa or Google Home can be integrated.

### What is FritzBox and Fritz!Dect 200 switchable power socket?
Fritz!Dect 200 is a switchable power socket, which can be switched remotely through radio transmission of the DECT telefone system integrated in FritzBox devices. FritzBox devices, produced by the german company AVM, are mostly combinations of Internet Router (WLAN and Ethernet) with a DECT telefone system. 'FritzBox'es are very popular in Germany.

### This App accomplishes a way to switch the power socket remotely over the internet from a Sinric Pro Phone App via the Sinric Pro cloud service and the FritzBox (Router/DECT-Phone combination).

### Before you can start:
Define WiFi-Credentials, FritzBox-Credentials and Sinric Pro Credentials in the file include/config_secrete.h (take 'config_secret_template.h' as a template)

Define 'TRANSPORT_PROTOCOL' (http or https) in file config.h When you begin to work with this App set TRANSPORT_PROTOCOL = 0 (http) The https secured connection will not work before you include the specific certificate of your personal FritzBox in include/config.h. Instructions how to get the certificate are given in the file 'config.h' When you have included the correct certificate, set TRANSPORT_PROTOCOL = 1 More configurations can be made in config.h (details are explained in config.h).

The FRITZ_DEVICE_AIN can be found on your Fritz!Dect 200 powersocket. 

To get the Sinric Pro Credentials have a look at:

https://sinric.pro/de-index.html

https://sinricpro.github.io/esp8266-esp32-sdk/index.html

https://github.com/sinricpro 




