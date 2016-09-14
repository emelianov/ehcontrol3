/////////////////////////////////////////////////////
// EHControl 2016.3 (c)2016, a.m.emelianov@gmail.com
//
// 

#define REVISION "2016.3.1"
#define CFG_GLOBAL "/global.xml"
#define CFG_SECURE "/secure.xml"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Run.h>
#include <FS.h>
#include <TinyXML.h>
#include "_options.h"
#include "lcd2.h"
#include "noclock.h"
#include "sensors.h"
#include "inputs.h"
#include "relays.h"
#include "partners.h"
#include "control.h"

IPAddress sysLogServer(192, 168, 30, 30);    // SysLog server
IPAddress timeServer(192,168,0,1);          // NTP server
int8_t timeZone = 0;
String name("New");
String adminUsername("admin");
String adminPassword("password3");
String partners[PARTNER_MAX_COUNT];
struct features {
  bool sensors;
  bool lcd;
  bool heater;
};
features use;
//#ifndef ESP8266
//WiFiUDP udp;
//#endif

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
#define SSID_MAX_LENGTH 16
#define PASS_MAX_LENGTH 24

uint32_t startWiFiAP() {
   WiFi.mode(WIFI_AP);
   WiFi.begin();
   return 0;
}
uint32_t startWiFi() {
  BUSY
  String ssid("");
  String password("");
  IPAddress ip(192, 168, 20, 124);             // IP
  IPAddress mask(255, 255, 255, 0);            // MASK
  IPAddress gw(192, 168, 20, 2);               // GW
  IPAddress ns(192, 168, 20, 2);               // DNS
  bool ipIsOk = false;
  uint8_t i = 0;
  for (uint8_t j = 0; j < PARTNER_MAX_COUNT; j++) {
  	partners[j] = "";
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
        timeServer.fromString(xmlData);
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
      	if (i < PARTNER_MAX_COUNT) {
			    partners[i] = xmlData;
			    i++;
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
   return 0;
  }
}
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
#define BUTTON 3
#define LONGPRESS 2000
#define LONGPRESSDURATION 30000

uint32_t buttonLongPress() {
  ALERT
  startWiFiAP();
  taskDel(buttonLongPressEnd);
  taskAddWithDelay(buttonLongPressEnd, LONGPRESSDURATION);
  return 0;
}
uint32_t buttonLongPressEnd() {
  NOALERT
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
  taskAdd(updateInputs);
  inputEvent(BUTTON, ON_ON, buttonPress);
  inputEvent(BUTTON, ON_OFF, buttonRelease);
  initRelays();
  taskAdd(switchSchedule);
  taskAdd(switchRelays);
  taskAdd(lazyRelays);
  return 0;
}
uint32_t ager() {
	return AGER_INTERVAL;
}
void setup(void){
  Serial.begin(74880);
  pinMode (PIN_ACT, OUTPUT);
  pinMode (D3, INPUT);
  pinMode (PIN_ALERT, OUTPUT);
  NOALERT
  SPIFFS.begin();
  xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  taskAdd(startWiFi);
  taskAdd(initTSensors);
  taskAdd(initMisc);
  taskAdd(initWeb);
//  taskAdd(updateLcd);
//  taskAdd(switchSchedule);
//  taskAdd(switchRelays);
//  taskAdd(lazyRelays);
//  taskAdd(updateInputs);
  taskAdd(ager);
} 
void loop(void){
  TASKEXEC
  yield();
} 
