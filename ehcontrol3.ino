//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2016, a.m.emelianov@gmail.com
//
// ESP8266-based Home automation solution

#define REVISION "2016.8"

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
#define BUSY          if (use.led) digitalWrite(PIN_ACT, LOW);
#define IDLE          if (use.led) digitalWrite(PIN_ACT, HIGH);
#define ALERT         digitalWrite(PIN_ALERT, LOW);
#define NOALERT       digitalWrite(PIN_ALERT, HIGH);
#define DEFAULT_NAME  "New"
#define DEFAULT_ADMIN "admin"
#define DEFAULT_PASS  "password3"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <time.h>

//NTP settings
#define NTP_CHECK_DELAY 300000;
#define NTP_MAX_COUNT 3
String timeServer[NTP_MAX_COUNT];         // NTP servers
int8_t timeZone = 0;

//Controller settings
#define SSID_MAX_LENGTH 16
#define PASS_MAX_LENGTH 24
String name(DEFAULT_NAME);
String adminUsername(DEFAULT_ADMIN);
String adminPassword(DEFAULT_PASS);

struct features {
  bool led;       // Activity LED
  bool apSwitch;  // AP-mode button default behavior
  bool apAuto;    // AP-mode if can't connect 
  bool apDefault;
  bool sensors;   // DS18S20 support
  bool lcd;       // LCD1620 support
  bool heater;    // 2 Zones heating controller support
  bool partners;  // Inter-ESP data exchange support
  bool ap;        // Force AP mode
  bool accel;     // ADXL345 support
  bool bmp;       // BMP280 support
};
features use = {true, true, true, false, false, false, false, false, false, false, false};

struct events {
  uint16_t webReady;
  uint16_t ntpReady;
  uint16_t startReady;
  uint16_t wifiReady;
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
#include "bmp280.h"
#include "web.h"

String pull[PARTNER_MAX_COUNT];
String push[PARTNER_MAX_COUNT];
String allow[PARTNER_MAX_COUNT];
#define APSCAN_MAX 16
String aps[APSCAN_MAX];

//Update time from NTP server
uint32_t initNtp() {
  if (time(NULL) == 0) {
    configTime(timeZone * 3600, 0, timeServer[0].c_str(), timeServer[1].c_str(), timeServer[2].c_str());
    return NTP_CHECK_DELAY;
  }
  event.ntpReady++;
  return RUN_DELETE;
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

uint32_t start() {
  BUSY
  String ssid("");
  String password("");
  IPAddress ip(0, 0, 0, 0);
  IPAddress mask(255, 255, 255, 0);
  IPAddress gw(192, 168, 20, 2);
  IPAddress ns(192, 168, 20, 2);
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

  //taskAdd(initMisc);
  //taskAdd(initWeb);
  if (!use.ap && ssid != "" && password != "") { 
   if (ip != INADDR_NONE) {
    WiFi.config(ip, gw, mask, ns);
   }
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid.c_str(), password.c_str());
   taskAdd(waitWiFi);
  } else {
   use.ap = true;
   WiFi.mode(WIFI_AP);
   if (ssid != "" && password != "") {
    WiFi.softAP(ssid.c_str(), password.c_str());
   } else {
    //WiFi.begin();
   }
   //taskAddWithDelay(startWeb, 2000);
   taskAdd(buttonLongPressLedOn);
   event.wifiReady++;
   IDLE
  }
  event.startReady++;
  return RUN_DELETE;
}
//Wait for wireless connection and start network-depended services as connection established
uint32_t waitWiFi() {
     if(WiFi.status() == WL_CONNECTED){
       //taskAdd(initNtp);
       //taskAddWithDelay(startWeb, 2000);
       event.wifiReady++;
       IDLE
       return RUN_DELETE;
     }
     return WIFI_RETRY_DELAY;  
}

//Start Open Access Point mode on button long press
uint32_t startWiFiAP() {
   if (!use.apDefault) {
    use.ap = true;
    taskDel(waitWiFi);
    WiFi.disconnect();
    adminUsername = F(DEFAULT_ADMIN);
    adminPassword = F(DEFAULT_PASS);
    server.stop();
    taskDel(handleWeb);
    //WiFi.softAP(name.c_str());
    WiFi.mode(WIFI_AP);
    //WiFi.begin();
    startWeb();
    //taskAddWithDelay(startWeb, 2000);
    IDLE
    use.apDefault = true;
   }
   return RUN_DELETE;
}
uint32_t scanWiFi() {
  WiFi.mode(WIFI_STA); 
  WiFi.disconnect(); 
  int16_t n = WiFi.scanNetworks(); 
  for (uint16_t i = 0; i < n && i < APSCAN_MAX; ++i) { 
       // Print SSID and RSSI for each network found 
       Serial.print(i + 1); 
       Serial.print(": "); 
       Serial.print(WiFi.SSID(i)); 
       Serial.print(" ("); 
       Serial.print(WiFi.RSSI(i)); 
       Serial.print(")"); 
       Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*"); 
       delay(10); 
  }
  if (use.apDefault) {
    startWiFiAP();
    return RUN_DELETE;
  }
  if (use.ap) {
    
  }  
}


#define LONGPRESS       2000
#define LONGPRESSLEDON  3000
#define LONGPRESSLEDOFF 1000

uint32_t buttonLongPress() {
  ALERT
  taskAddWithDelay(buttonLongPressLedOff, LONGPRESSLEDON);
  startWiFiAP();
  return RUN_DELETE;
}
uint32_t buttonLongPressLedOn() {
  ALERT
  taskAddWithDelay(buttonLongPressLedOff, LONGPRESSLEDON);
  return RUN_DELETE;
}
uint32_t buttonLongPressLedOff() {
  NOALERT
  taskAddWithDelay(buttonLongPressLedOn, LONGPRESSLEDOFF);
  return RUN_DELETE;
}
uint32_t buttonAction() {
  taskDel(buttonLongPress);
  if (item[0]->current == LOW) {
    taskAddWithDelay(buttonLongPress, LONGPRESS);
  }
  return RUN_NEVER;
}

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
  if (use.sensors) {
    taskAdd(initTSensors);
  } else {
    readSensors();
  }
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
 //  taskAddWithSemaphore(tapOn, &(event.tap));
//   taskAddWithSemaphore(tapOff, &(event.doubleTap));
  }
  if (use.bmp) {
    if (!bmpInit()) {
      use.bmp = false;
   }
  }
//  taskAdd(ager);
  if (use.apSwitch) {
    taskAddWithSemaphore(buttonAction, &(item[0]->signal));
  }
  return RUN_DELETE;
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
  pinMode (PIN_ACT, OUTPUT);
  //gmode (D3, INPUT);
  pinMode (PIN_ALERT, OUTPUT);
  IDLE
  NOALERT
  WiFi.mode(WIFI_OFF);
  SPIFFS.begin();
  xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  taskAdd(start);
  taskAddWithSemaphore(initMisc, &event.startReady);
  taskAddWithSemaphore(initWeb, &event.startReady);
  taskAddWithSemaphore(initNtp, &event.wifiReady);
  //taskAddWithSemaphore(startWeb, &event.wifiReady);
} 
void loop(void){
  taskExec();
  wdt_reset();
  yield();
} 
