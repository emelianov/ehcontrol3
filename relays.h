//////////////////////////////////////////////////////
// EHControl2 2016.3 (c)2016, a.m.emelianov@gmail.com
// Relays definitions, constants and routines

#pragma once
#include "inputs.h"
#define RELAY_COUNT 4
#define TGRAPH_COUNT 8
#define CFG_RELAYS "/relays.xml"

#define DAY 0
#define NIGHT 1
#define ECO 2
#define OTHER 3

#define ECO_IN 1

extern bool ecoMode;
uint32_t switchSchedule();
uint32_t lazyRelays();
uint32_t switchRelays();

struct relay {
  bool on;
  int16_t pin;
  bool inv;
  uint16_t gid;
  float tHi;
  float tLow;
  float t[4];
  bool isT2;
  uint16_t onT2;  	// Nigth mode on time (minutes from midnight)
  uint16_t offT2; 	// Night mode off time (minutes from midnight)
  String name;
};
struct graph {
  float x;
  float y;
};

relay relays[RELAY_COUNT];
graph tGraph[TGRAPH_COUNT];

#define OFF(RELAY) relays[RELAY].on=false;
#define ON(RELAY)  relays[RELAY].on=true;
#define SWITCH _switch();

extern TinyXML xml;

float eqTemp(float t) { //Look t in temperature graph function and return corresponding value
  uint8_t i;
  for (i = 0; i < TGRAPH_COUNT - 1 && t < tGraph[i].x; i++) {
    //Nothing
  }
  return tGraph[i].y;
}

void _on(uint8_t i) {
  if (relays[i].inv) {digitalWrite(relays[i].pin,HIGH);} else {digitalWrite(relays[i].pin,LOW);};
  relays[i].on = true;
}
void _off(uint8_t i) { 
  if (relays[i].inv) {digitalWrite(relays[i].pin,LOW);} else {digitalWrite(relays[i].pin,HIGH);};
  relays[i].on=false;
}
void _switch() {
  uint8_t i;
  for (i = 0; i < RELAY_COUNT; i++) {
    if (relays[i].inv) {digitalWrite(relays[i].pin,relays[i].on?HIGH:LOW);} else {digitalWrite(relays[i].pin,relays[i].on?LOW:HIGH);};
  }
}

bool saveRelays() {
   File configFile = SPIFFS.open(CFG_RELAYS, "w");
   if (configFile) {
    char buf[200];
    sprintf_P(buf, PSTR("<?xml version = \"1.0\" ?>\n<relays>\n"));
    configFile.write((uint8_t*)buf, strlen(buf));
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      sprintf_P(buf, PSTR("<relay>\n<pin></pin><gid>%d</gid>\n<inverse>%d</inverse><name>%s</name><t0>%s</t0><t1>%s</t1><t2>%s</t2><t3>%s</t3></relay>\n<ton>%d</ton><toff>%d</toff>\n</relay>\n"),
                 relays[i].pin, relays[i].gid, relays[i].inv, relays[i].name.c_str(),
                 String(relays[i].t[0]).c_str(), String(relays[i].t[1]).c_str(), String(relays[i].t[2]).c_str(), String(relays[i].t[3]).c_str(),
                 relays[i].onT2, relays[i].offT2
               );
      configFile.write((uint8_t*)buf, strlen(buf));
    }
    for (uint8_t i = 0; i < TGRAPH_COUNT; i++) {
      sprintf_P(buf, PSTR("<t%d><e>%d</e><h>%d</h></t%d>\n"),
                i, tGraph[i].x, tGraph[i].y, i
               );
      configFile.write((uint8_t*)buf, strlen(buf));
    }
    sprintf_P(buf, PSTR("</relays>"));
    configFile.write((uint8_t*)buf, strlen(buf));
    configFile.close();
    return true;
   }
   return false;  
}

bool readRelays() {
  //uint8_t i;
  //memset(relays, 0, sizeof(relays));
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    relays[i].on    = false;
    relays[i].pin   = -1;
    relays[i].inv   = false;
    relays[i].gid   = 0;
    relays[i].tHi   = 0;
    relays[i].tLow  = 0;
    relays[i].t[0]  = 20;
    relays[i].t[1]  = 20;
    relays[i].t[2]  = 20;
    relays[i].t[3]  = 20;
    relays[i].isT2  = false;
    relays[i].onT2  = 0;
    relays[i].offT2 = 360;
    relays[i].name = String(i);
  };
  File configFile = SPIFFS.open(CFG_RELAYS, "r");
  if (configFile) {
   //TinyXML xml;
   //uint8_t buffer[150];
   //xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
   char c;
   //xmlTag = "";
   //xmlOpen = "";
   xml.reset();
   xmlTag = "";
   xmlOpen = "";
   int16_t i = 0;
   int16_t j = 0;
   bool relayOpen = false;
   bool tOpen = false;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlTag != "" || xmlOpen != "") {
      //Serial.println(xmlTag);
      //Serial.println(i);
       if 
      (xmlOpen.endsWith("/relay")) {
        //xmlOpen = "";
        if (i < RELAY_COUNT - 1) {
          if (relayOpen) {
            i++;
          } else {
            relayOpen = true;
          }
        }
       } else if
      (xmlOpen.endsWith("/t")) {
        //xmlOpen = "";
        if (j < TGRAPH_COUNT) {
          if (tOpen) {
            j++;
          } else {
            tOpen = true;
          }
        }        
       } else if
      (xmlTag.endsWith("/pin")) {
       relays[i].pin = xmlData.toInt();
       } else if
      (xmlTag.endsWith("/gid")) {
        relays[i].gid = xmlData.toInt();
       } else if
      (xmlTag.endsWith("/t0")) {
        relays[i].t[0] = xmlData.toFloat();
       } else if
      (xmlTag.endsWith("/t1")) {
        relays[i].t[1] = xmlData.toFloat();
       } else if
      (xmlTag.endsWith("/t2")) {
        relays[i].t[2] = xmlData.toFloat();
       } else if
      (xmlTag.endsWith("/t3")) {
        relays[i].t[3] = xmlData.toFloat();
       } else if
      (xmlTag.endsWith("/ton")) {
        relays[i].onT2 = xmlData.toInt();
       } else if
      (xmlTag.endsWith("/toff")) {
        relays[i].offT2 = xmlData.toInt();
       } else if
      (xmlTag.endsWith("/name")) {
        relays[i].name = xmlData;
       } else if
      (xmlTag.endsWith("/inverse")) {
        relays[i].inv = (xmlData.toInt() == 1);
       } else if
      (xmlTag.endsWith("/e")) {
        tGraph[j].x = xmlData.toFloat();
       } else if
      (xmlTag.endsWith("/h")) {
        tGraph[j].y = xmlData.toFloat();
     }
    }
    xmlTag = "";
    xmlOpen = "";
   }
   configFile.close();
   return true;
  }
  return false;  
}
void ecoOn() {
  ecoMode = true;
}
void ecoOff() {
  ecoMode = false;
}
void initRelays() {
 if (readRelays()) {
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
   if (relays[i].pin != -1) {
    relays[i].tHi  = relays[i].t[ECO] + 0.5;
    relays[i].tLow = relays[i].t[ECO] - 0.5;
    pinMode(relays[i].pin,OUTPUT);
    _off(i);
   }
  }
  taskAdd(switchSchedule);
  taskAdd(switchRelays);
  taskAdd(lazyRelays); 
 } else {
  use.heater = false;
 }
 inputEvent(ECO_IN, ON_ON,  ecoOn);
 inputEvent(ECO_IN, ON_OFF, ecoOff);
}
