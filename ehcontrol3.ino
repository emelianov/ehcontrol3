/////////////////////////////////////////////////////
// EHControl 2016.3 (c)2016, a.m.emelianov@gmail.com
//
// 

#define REVISION "2016.3.1"

#define CFG_GLOBAL "/global.xml"
#define CFG_SECURE "/secure.xml"

#define ONEWIRE_PIN   D6  //12

#define AGER_INTERVAL 15000
#define AGER_EXPIRE   20
#define WIFI_RETRY_DELAY 1000

#define SYSTEM_PIN    D2
#define PIN_ACT       D4  //Net LED
#define PIN_ALERT     D0  //16
#define BUSY          digitalWrite(PIN_ACT, 0);
#define IDLE          digitalWrite(PIN_ACT, 1);
#define ALERT         digitalWrite(PIN_ALERT, LOW);
#define NOALERT       digitalWrite(PIN_ALERT, HIGH);

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Run.h>
#include <FS.h>
#include <TinyXML.h>
#include "lcd2.h"
#include "sensors.h"
#include "inputs.h"
#include "relays.h"
#include "partners.h"
#include "control.h"

//Syslog settings
#define UDP_PORT 33666
IPAddress sysLogServer(192, 168, 30, 30);    // SysLog server
//NTP settings
#define NTP_CHECK_DELAY 300000;
#define NTP_MAX_COUNT 3
//WiFiUDP udp;
String timeServer[NTP_MAX_COUNT];          // NTP servers
int8_t timeZone = 0;
//Controller settings
#define SSID_MAX_LENGTH 16
#define PASS_MAX_LENGTH 24
String name("New");
String adminUsername("admin");
String adminPassword("password3");
String pull[PARTNER_MAX_COUNT];
String push[PARTNER_MAX_COUNT];
String allow[PARTNER_MAX_COUNT];
struct features {
  bool sensors;
  bool lcd;
  bool heater;
};
features use;
//XML processor settings
String xmlOpen;
String xmlTag;
String xmlData;
String xmlAttrib;
#include "web.h"
TinyXML xml;
uint8_t buffer[150];
void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
  //Serial.println(tagName);
  if
  (statusflags & STATUS_TAG_TEXT) {
    xmlTag = String(tagName);
    xmlData = String(data);
  } else if
  (statusflags & STATUS_START_TAG) {
    //Serial.println(tagName);
    xmlOpen = String(tagName);
  }
}

//Update time from NTP server
uint32_t initNtp() {
  if (time(NULL) == 0) {
    configTime(timeZone * 3600, 0, timeServer[0].c_str());
    return NTP_CHECK_DELAY;
  }
  return 0;
}

//Read global config and start network
uint32_t startWiFi() {
  BUSY
  String ssid("");
  String password("");
  IPAddress ip(192, 168, 20, 124);             // IP
  IPAddress mask(255, 255, 255, 0);            // MASK
  IPAddress gw(192, 168, 20, 2);               // GW
  IPAddress ns(192, 168, 20, 2);               // DNS
  bool ipIsOk = false;
  uint8_t i;
  for (i = 0; i < NTP_MAX_COUNT; i++) {
    timeServer[i] = "";
  }
  for (i = 0; i < PARTNER_MAX_COUNT; i++) {
  	pull[i] = "";
    push[i] = "";
    allow[i] = "";
  }
//Read global config options
  xml.reset();
  xmlTag = "";
  xmlOpen = "";
  File configFile = SPIFFS.open(CFG_GLOBAL, "r");
  if (configFile) {
   char c;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlTag != "") {
       if 
      (xmlTag.endsWith("/ntps")) {
        for (i = 0; i < NTP_MAX_COUNT; i++) {
          if (timeServer[i] == "") {
            timeServer[i] = xmlData;
          }
        }
       } else if 
      (xmlTag.endsWith("/timezone")) {
        timeZone = xmlData.toInt();
       } else if 
      (xmlTag.endsWith("/ssid")) {
        ssid = xmlData;
       } else if 
      (xmlTag.endsWith("/ssidpass")) {
        password = xmlData; 
       } else if 
      (xmlTag.endsWith("/ip")) {
        ipIsOk = ip.fromString(xmlData);
       } else if 
      (xmlTag.endsWith("/mask")) {
        mask.fromString(xmlData);
       } else if 
      (xmlTag.endsWith("/gw")) {
        gw.fromString(xmlData);
       } else if 
      (xmlTag.endsWith("/dns")) {
        ns.fromString(xmlData);
       } else if
      (xmlTag.endsWith("/syslog")) {
        sysLogServer.fromString(xmlData);
       } else if
      (xmlTag.endsWith("/name")) {
        name = xmlData;
       } else if
      (xmlTag.endsWith("/partner") || xmlTag.endsWith("/pull")) {
      	for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (pull[i] == "") {
			pull[i] = xmlData;
            break;
          }
		}
       } else if
      (xmlTag.endsWith("/push")) {
        for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (push[i] == "") {
            push[i] = xmlData;
            break;
          }
        }
       } else if
      (xmlTag.endsWith("/allow")) {
        for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (allow[i] == "") {
            allow[i] = xmlData;
            break;
          }
        }
       }
      xmlTag = "";
      xmlData = "";
    }
   }
   configFile.close();
  }
//Read secure config options
  xml.reset();
  xmlTag = "";
  xmlOpen = "";
  configFile = SPIFFS.open(CFG_SECURE, "r");
  if (configFile) {
   char c;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlTag != "") {
       if 
      (xmlTag.endsWith("/admin")) {
        adminUsername = xmlData;
       } else if 
      (xmlTag.endsWith("/adminpass")) {
        adminPassword = xmlData;
       } else if 
      (xmlTag.endsWith("/ssid")) {
        ssid = xmlData;
       } else if 
      (xmlTag.endsWith("/ssidpass")) {
        password = xmlData; 
       }
      xmlTag = "";
      xmlData = "";
    }
   }
   configFile.close();
  }
  if (ssid != "" && password != "") { 
   if (ipIsOk) {
    WiFi.config(ip, gw, mask, ns);
   }
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid.c_str(), password.c_str());
   taskAdd(waitWiFi);
   return 0;
  } else {
   WiFi.mode(WIFI_AP);
   WiFi.begin();
   taskAddWithDelay(startWeb, 2000);
   taskAdd(buttonLongPressLedOn);
   return 0;
  }
}
//Wait for wireless connection and start network-depended services as connection established
uint32_t waitWiFi() {
     if(WiFi.status() == WL_CONNECTED){
       taskAdd(initNtp);
       taskAdd(startWeb);
       taskAdd(queryPartners);
       IDLE
       return 0;
     }
     return WIFI_RETRY_DELAY;  
}

#define LONGPRESS       2000
#define LONGPRESSLEDON  5000
#define LONGPRESSLEDOFF 2000

//Start Open Access Point mode on button long press
uint32_t startWiFiAP() {
   WiFi.mode(WIFI_AP);
   WiFi.begin();
   return 0;
}
uint32_t buttonLongPress() {
  ALERT
  taskAddWithDelay(buttonLongPressLedOff, LONGPRESSLEDON);
  startWiFiAP();
  return 0;
}
uint32_t buttonLongPressLedOn() {
  ALERT
  taskAddWithDelay(buttonLongPressLedOff, LONGPRESSLEDON);
  return 0;
}
uint32_t buttonLongPressLedOff() {
  NOALERT
  taskAddWithDelay(buttonLongPressLedOn, LONGPRESSLEDOFF);
  return 0;
}
void buttonPress() {
  taskDel(buttonLongPress);
  taskAddWithDelay(buttonLongPress, LONGPRESS);
}
void buttonRelease() {
  taskDel(buttonLongPress);
}

uint32_t initMisc() {
  initLcd();
  taskAdd(updateLcd);
  initInputs();
  defaultInput(0); //Override input at position 0 to NodeMCU button
  taskAdd(updateInputs);
  inputEvent(0, ON_ON, buttonPress);
  inputEvent(0, ON_OFF, buttonRelease);
  initRelays();
  taskAdd(switchSchedule);
  taskAdd(switchRelays);
  taskAdd(lazyRelays);
  return 0;
}

// Increase age of last update for all sensors and inputs having .gid or connected localy
uint32_t ager() {
  uint8_t i;
  DeviceAddress zerro;
  memset(zerro, 0, sizeof(DeviceAddress));

  for (i = 0; i < INPUTS_COUNT; i++) {
    if (inputs[i].gid != 0 || inputs[i].pin != -1) {
      inputs[i].age += AGER_INTERVAL;
    }
  }

  for (i = 0; i < ANALOG_COUNT; i++) {
    if (analogs[i].gid != 0 || analogs[i].pin != -1) {
      analogs[i].age += AGER_INTERVAL;
    }
  }

  for (i = 0; i < DEVICE_MAX_COUNT; i++) {
    if (sens[i].gid != 0 || memcmp(sens[i].device, zerro, sizeof(DeviceAddress)) != 0) {
      sens[i].age += AGER_INTERVAL;
    }
  }
	return AGER_INTERVAL;
}
void setup(void){
  Serial.begin(74880);
  pinMode (PIN_ACT, OUTPUT);
  pinMode (D3, INPUT);
  pinMode (PIN_ALERT, OUTPUT);
  IDLE
  NOALERT
  SPIFFS.begin();
  xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  taskAdd(startWiFi);
  taskAdd(initTSensors);
  taskAdd(initMisc);
  taskAdd(initWeb);
  //taskAdd(ager);
} 
void loop(void){
  TASKEXEC
  yield();
} 
