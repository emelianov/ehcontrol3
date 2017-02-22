//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2016, a.m.emelianov@gmail.com
//
// ESP8266-based Home automation solution

#define CFG_GLOBAL "/global.xml"
#define CFG_SECURE "/secure.xml"
#include <Run.h>
#include <FS.h>
#include <TinyXML.h>

//XML processor settings
String xmlOpen;
String xmlTag;
String xmlData;
String xmlAttrib;
TinyXML xml;
uint8_t buffer[150];
void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
  if
  (statusflags & STATUS_TAG_TEXT) {
    xmlTag = String(tagName);
    xmlData = String(data);
  } else if
  (statusflags & STATUS_START_TAG) {
    xmlOpen = String(tagName);
  }
}

typedef void (*callback)(String value);
struct cfgLine {
  String entry;
  callback cb;
};

cfgLine cfgs[] = {"123",[](String _s){String s=_s;},"123",[](String _s){String s=_s;}};
bool cfgParse(String cfgName) {
  xml.reset();
  xmlTag = "";
  xmlOpen = "";
  File configFile = SPIFFS.open(cfgName, "r");
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
            break;
          }
        }
       } else if 
      (xmlTag.endsWith(F("/timezone"))) {
        timeZone = xmlData.toInt();
       } else if 
      (xmlTag.endsWith(F("/ssid"))) {
        ssid = xmlData;
       } else if 
      (xmlTag.endsWith(F("/ssidpass"))) {
        password = xmlData; 
       } else if 
      (xmlTag.endsWith(F("/ip"))) {
        ipIsOk = ip.fromString(xmlData);
       } else if 
      (xmlTag.endsWith(F("/mask"))) {
        mask.fromString(xmlData);
       } else if 
      (xmlTag.endsWith(F("/gw"))) {
        gw.fromString(xmlData);
       } else if 
      (xmlTag.endsWith(F("/dns"))) {
        ns.fromString(xmlData);
       } else if
      (xmlTag.endsWith(F("/syslog"))) {
        sysLogServer.fromString(xmlData);
       } else if
      (xmlTag.endsWith(F("/name"))) {
        name = xmlData;
       } else if
      (xmlTag.endsWith(F("/pin1wire"))) {
        pinOneWire = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/partner")) || xmlTag.endsWith(F("/pull"))) {
      	for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (pull[i] == "") {
			      pull[i] = xmlData;
            break;
          }
		    }
       } else if
      (xmlTag.endsWith(F("/push"))) {
        for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (push[i] == "") {
            push[i] = xmlData;
            break;
          }
        }
       } else if
      (xmlTag.endsWith(F("/allow"))) {
        for (i = 0; i < PARTNER_MAX_COUNT; i++) {
          if (allow[i] == "") {
            allow[i] = xmlData;
            break;
          }
        }
       } else if
      (xmlTag.endsWith(F("/feature/led"))) {
        use.led = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/sensors"))) {
        use.sensors = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/lcd"))) {
        use.lcd = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/heater"))) {
        use.heater = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/partners"))) {
        use.partners = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/ap"))) {
        use.ap = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith(F("/feature/accel"))) {
        use.accel = (xmlData.toInt() == 1);
       }
      xmlTag = "";
      xmlData = "";
    }
    yield();
   }
   configFile.close();
  }
//Read secure config options
  xml.reset();
  xmlTag = "";
  xmlOpen = "";
  configFile = SPIFFS.open(F(CFG_SECURE), "r");
  if (configFile) {
   char c;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlTag != "") {
       if 
      (xmlTag.endsWith(F("/admin"))) {
        adminUsername = xmlData;
       } else if 
      (xmlTag.endsWith(F("/adminpass"))) {
        adminPassword = xmlData;
       } else if 
      (xmlTag.endsWith(F("/ssid"))) {
        ssid = xmlData;
       } else if 
      (xmlTag.endsWith(F("/ssidpass"))) {
        password = xmlData;
       } else if 
      (xmlTag.endsWith(F("/protect"))) {
        if (!xmlData.startsWith("/")) {
          xmlData = "/" + xmlData;
        }
        if (xmlData != "/secure.xml") {
          server.on(xmlData.c_str(), HTTP_GET, handleProtectedFile);
        }
       }
      xmlTag = "";
      xmlData = "";
    }
   }
   configFile.close();
  }
  taskAdd(initMisc);
  taskAdd(initWeb);
  if (!use.ap && ssid != "" && password != "") { 
   WiFi.mode(WIFI_OFF);
   delay(250);
   if (ipIsOk) {
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
   taskAdd(buttonLongPressLedOn);
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
  initInputs();
  defaultInput(0); //Override input at position 0 to NodeMCU button
  taskAdd(updateInputs);
  inputEvent(0, ON_ON, buttonPress);
  inputEvent(0, ON_OFF, buttonRelease);
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
  }
  taskAdd(ager);
  return 0;
}

// Increase age of last update for all sensors and inputs having .gid or connected localy
uint32_t ager() {
  uint8_t i;
  DeviceAddress zerro;
  memset(zerro, 0, sizeof(DeviceAddress));

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
