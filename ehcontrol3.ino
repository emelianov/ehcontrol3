//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2016, a.m.emelianov@gmail.com
//
// ESP8266-based Home automation solution

#define REVISION "2016.7"

#define CFG_GLOBAL "/global.xml"
#define CFG_SECURE "/secure.xml"

#define AUTOSAVE_DELAY 10000

#define PIN_ONEWIRE   D6  //12

#define AGER_INTERVAL 15
#define AGER_EXPIRE   90
#define AGER_MAX      30000
#define WIFI_RETRY_DELAY 1000

#define SYSTEM_PIN    0   //0 Config mode pin
#define PIN_ACT       D4  //Net LED
#define PIN_ALERT     D0  //16
#define BUSY          if (use.led) gwrite(PIN_ACT, 0);
#define IDLE          if (use.led) gwrite(PIN_ACT, 1);
#define ALERT         gwrite(PIN_ALERT, LOW);
#define NOALERT       gwrite(PIN_ALERT, HIGH);
#define DEFAULT_NAME  "New"
#define DEFAULT_ADMIN "admin"
#define DEFAULT_PASS  "password3"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <WiFiUdp.h>
#include <time.h>

//Syslog settings
//#define UDP_PORT 33666
//IPAddress sysLogServer(192, 168, 30, 30); // SysLog server
//NTP settings
#define NTP_CHECK_DELAY 300000;
#define NTP_MAX_COUNT 3
//WiFiUDP udp;
String timeServer[NTP_MAX_COUNT];         // NTP servers
int8_t timeZone = 0;
//Controller settings
#define SSID_MAX_LENGTH 16
#define PASS_MAX_LENGTH 24
String name(DEFAULT_NAME);
String adminUsername(DEFAULT_ADMIN);
String adminPassword(DEFAULT_PASS);
struct features {
  bool led;
  bool sensors;
  bool lcd;
  bool heater;
  bool partners;
  bool ap;
  bool syslog;
  bool accel;
  bool bmp;
};
features use = {true, false, false, false, false, false, false, false, false};

struct events {
  uint16_t webStart;
  uint16_t adxl;
  uint16_t tap;
  uint16_t doubleTap;
};
events event = {0, 0, 0, 0};

class Item {
  public:
  String    name;
  float     current;
  uint16_t  gid;
  uint16_t  age;
  uint16_t  signal;
  Item() {
    name    = "";
    current = 0;
    gid     = 0;
    age     = 0;
    signal  = 0;
  }
};
#define MAX_ITEMS 32
Item* item[MAX_ITEMS] = {NULL};

uint16_t pinOneWire = PIN_ONEWIRE;

#include <Run.h>
#include <FS.h>
#include <TinyXML.h>
#include "lcd2.h"
#include "sensors.h"
#include "inputs.h"
#include "control.h"
#include "relays.h"
#include "partners.h"
#include "accel.h"
//#include "bmp280.h"
#include "web.h"

String pull[PARTNER_MAX_COUNT];
String push[PARTNER_MAX_COUNT];
String allow[PARTNER_MAX_COUNT];

//Update time from NTP server
uint32_t initNtp() {
  if (time(NULL) == 0) {
    configTime(timeZone * 3600, 0, timeServer[0].c_str(), timeServer[1].c_str(), timeServer[2].c_str());
    return NTP_CHECK_DELAY;
  }
  //done.ntp = true;
  return 0;
}

//Read global config and start network
void entryNtps(String s, void* v=NULL) {
  for (uint8_t i = 0; i < NTP_MAX_COUNT; i++) {
    if (timeServer[i] == "") {
      timeServer[i] = s;
      break;
    }
  }
}
void entryIp(String s, void* v) {
  IPAddress* t = (IPAddress*)v;
  t->fromString(s);
}
void entryPull(String s, void* v=NULL) {
  for (uint8_t i = 0; i < PARTNER_MAX_COUNT; i++) {
    if (pull[i] == "") {
      pull[i] = s;
      break;
    }
  }
}
void entryPush(String s, void* v=NULL) {
  for (uint8_t i = 0; i < PARTNER_MAX_COUNT; i++) {
    if (push[i] == "") {
      push[i] = s;
      break;
    }
  }
}
void entryAllow(String s, void* v=NULL) {
  for (uint8_t i = 0; i < PARTNER_MAX_COUNT; i++) {
    if (allow[i] == "") {
      allow[i] = s;
      break;
    }
  }
}
void entryProtect(String s, void* v=NULL) {
  if (!s.startsWith("/")) {
    s = "/" + s;
  }
  if (s != CFG_SECURE) {
    server.on(s.c_str(), HTTP_GET, handleProtectedFile);
  }
}

uint32_t startWiFi() {
  BUSY
  String ssid("");
  String password("");
  IPAddress ip(0, 0, 0, 0);             // IP
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
  CfgEntry cfg[] = {CfgEntry(F("/ntps"),      &entryNtps),
                    CfgEntry(F("/timezone"),  &timeZone),
                    CfgEntry(F("/ssid"),  &ssid),
                    CfgEntry(F("/ssidpass"),  &password),
                    CfgEntry(F("/ip"),  &entryIp, &ip),
                    CfgEntry(F("/mask"),  &entryIp, &mask),
                    CfgEntry(F("/gw"),  &entryIp, &gw),
                    CfgEntry(F("/dns"),  &entryIp, &ns),
                    CfgEntry(F("/name"),  &name),
                    CfgEntry(F("/pin1wire"),  &pinOneWire),
                    CfgEntry(F("/pull"),  &entryPull),
                    CfgEntry(F("/partner"),  &entryPull),
                    CfgEntry(F("/push"),  &entryPush),
                    CfgEntry(F("/allow"),  &entryAllow),
                    CfgEntry(F("/feature/led"),  &use.led),
                    CfgEntry(F("/feature/sensors"),  &use.sensors),
                    CfgEntry(F("/feature/lcd"),  &use.lcd),
                    CfgEntry(F("/feature/heater"),  &use.heater),
                    CfgEntry(F("/feature/partners"),  &use.partners),
                    CfgEntry(F("/feature/ap"),  &use.ap),
                    CfgEntry(F("/feature/accel"),  &use.accel),
                    CfgEntry(F("/feature/bmp"),  &use.bmp),
  };
  cfgParse(F(CFG_GLOBAL), cfg, sizeof(cfg)/sizeof(cfg[0]));
//Read secure config options
  CfgEntry priv[] = {CfgEntry(F("/admin"),      &adminUsername),
                    CfgEntry(F("/adminpass"),  &adminPassword),
                    CfgEntry(F("/ssid"),      &ssid),
                    CfgEntry(F("/ssidpass"),      &password),
                    CfgEntry(F("/protect"),      &entryProtect),
  };
  cfgParse(F(CFG_SECURE), priv, sizeof(cfg)/sizeof(cfg[0]));

  taskAdd(initMisc);
  taskAdd(initWeb);
  if (!use.ap && ssid != "" && password != "") { 
   WiFi.mode(WIFI_OFF);
   delay(250);
   if (ip != INADDR_NONE) {
    WiFi.config(ip, gw, mask, ns);
   }
   WiFi.mode(WIFI_OFF);
   delay(250);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid.c_str(), password.c_str());
   taskAdd(waitWiFi);
   return 0;
  } else {
   use.ap = true;
   WiFi.mode(WIFI_AP);
   WiFi.begin();
   taskAddWithDelay(startWeb, 2000);
//   taskAdd(buttonLongPressLedOn);
   IDLE
   return 0;
  }
}
//Wait for wireless connection and start network-depended services as connection established
uint32_t waitWiFi() {
     if(WiFi.status() == WL_CONNECTED){
       taskAdd(initNtp);
       taskAddWithDelay(startWeb, 2000);
       IDLE
       return 0;
     }
     return WIFI_RETRY_DELAY;  
}

#define LONGPRESS       2000
#define LONGPRESSLEDON  3000
#define LONGPRESSLEDOFF 1000

//Start Open Access Point mode on button long press
uint32_t startWiFiAP() {
   use.ap = true;
   adminUsername = F(DEFAULT_ADMIN);
   adminPassword = F(DEFAULT_PASS);
   WiFi.mode(WIFI_AP);
   WiFi.begin();
   server.stop();
   taskDel(handleWeb);
   taskAddWithDelay(startWeb, 2000);
   return 0;
}
/*
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
*/
bool tapBlinkState = false;
uint32_t tapOn() {
    ALERT
  return RUN_NEVER;
}
uint32_t tapOff() {
    NOALERT
  return RUN_NEVER;
}
uint32_t initMisc() {
  initInputs();
//  defaultInput(0); //Override input at position 0 to NodeMCU button
//  taskAdd(updateInputs);
//  inputEvent(0, ON_ON, buttonPress);
//  inputEvent(0, ON_OFF, buttonRelease);
  if (use.sensors) {
    taskAdd(initTSensors);
  } else {
    readSensors();
  }
  //taskAdd(readTSensors);
  if (use.heater) {
    initRelays();
  }
  if (use.lcd) {
    if (!initLcd()) {
      use.lcd = false;
    }
    //taskAdd(updateLcd);
  }
  if (use.partners) {
    taskAdd(queryPartners);
  }
  if (use.accel) {
  	if (!initAccel()) {
  		use.accel = false;
  	}
   taskAddWithSemaphore(tapOn, &(event.tap));
   taskAddWithSemaphore(tapOff, &(event.doubleTap));
  }
//  if (use.bmp) {
//    if (!bmpInit()) {
//      use.bmp = false;
//   }
//  }
  taskAdd(ager);
  return 0;
}

// Increase age of last update for all sensors and inputs having .gid or connected localy
uint32_t ager() {
  uint8_t i;
  DeviceAddress zerro;
  memset(zerro, 0, sizeof(DeviceAddress));
/*
  for (i = 0; i < INPUTS_COUNT; i++) {
    if (inputs[i].gid != 0 || inputs[i].pin != -1) {
      if (inputs[i].age < AGER_MAX) inputs[i].age += AGER_INTERVAL;
    }
  }

  for (i = 0; i < ANALOG_COUNT; i++) {
    if (analogs[i].gid != 0 || analogs[i].pin != -1) {
      if (analogs[i].age < AGER_MAX) analogs[i].age += AGER_INTERVAL;
    }
  }
*/
  if (use.sensors || use.partners) {
   for (i = 0; i < DEVICE_MAX_COUNT; i++) {
    if (sens[i].gid != 0 || memcmp(sens[i].device, zerro, sizeof(DeviceAddress)) != 0) {
      if (sens[i].age < AGER_MAX) sens[i].age += AGER_INTERVAL;
    }
   }
  }
	return AGER_INTERVAL * 1000;
}
/*
uint32_t monitorGw() {
  return IP_MONITOR_DELAY;
}
*/

void setup(void){
  wdt_enable(0);
  Serial.begin(74880);
  gmode (PIN_ACT, OUTPUT);
  gmode (D3, INPUT);
  gmode (PIN_ALERT, OUTPUT);
  IDLE
  NOALERT
  SPIFFS.begin();
  xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  taskAdd(startWiFi);
  //taskAdd(initMisc);
  //taskAdd(initWeb);
  //taskAdd(ager);
} 
void loop(void){
  taskExec();
  wdt_reset();
  yield();
} 
