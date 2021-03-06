//////////////////////////////////////////////////////
// EHControl3 2016.4 (c)2016, a.m.emelianov@gmail.com
// Partners interaction

#pragma once
#include <ESP8266HTTPClient.h>

#define PARTNER_SLEEP 30000
#define PARTNER_NEXT 5000

#define PARTNER_SUFFIX "/short"
#define PARTNER_MAX_COUNT 16

int16_t lookSensorByGid(uint16_t gid) {
	uint8_t i;
	for (i = 0; i < DEVICE_MAX_COUNT && sens[i].gid != gid; i++) { }
	return (i >= DEVICE_MAX_COUNT)?-1:i; 
}
int16_t lookInputByGid(uint16_t gid) {
  uint8_t i;
  for (i = 0; i < INPUTS_COUNT && inputs[i].gid != gid; i++) { }
  return (i >= INPUTS_COUNT)?-1:i;   
}

uint8_t currentPartner = 0;
extern TinyXML xml;
extern String xmlOpen;
extern String xmlTag;
extern String xmlData;
extern String xmlAttrib;

void parsePartner(const String textXml) {
  	sensor partner;
    partner.gid = 0;
    partner.tCurrent = DEVICE_DISCONNECTED_C;
  	//memset(&partner, 0, sizeof(partner));
   	//	char c;
        xml.reset();
        xmlTag = "";
        xmlOpen = "";
        uint16_t textLength = textXml.length();
    	for (uint16_t i = 0; i < textLength; i++) {
    				xml.processChar(textXml.charAt(i));
    				if (xmlTag != "" || xmlOpen != "") {
       					if 
      				  (xmlOpen.endsWith(F("/sensor"))) {
      					//xmlOpen = "";
        				if (partner.gid != 0) {
          					int8_t i = lookSensorByGid(partner.gid);
          					if (i >= 0) {
        						 sens[i].tCurrent = partner.tCurrent;
        						 sens[i].age = 0;
          					}
        				}
        				//memset(&partner, 0, sizeof(partner));
                		partner.gid = 0;
                		partner.tCurrent = DEVICE_DISCONNECTED_C;
       					} else if
      				  (xmlTag.endsWith(F("/sensor/gid"))) {
        				partner.gid = xmlData.toInt();
       					} else if
      				  (xmlTag.endsWith(F("/sensor/t"))) {
        				partner.tCurrent = xmlData.toFloat();
       					}
                	  xmlTag = "";
                	  xmlOpen = "";
    				}
   		  }
          if (partner.gid != 0) {
            int8_t i = lookSensorByGid(partner.gid);
            if (i >= 0) {
              sens[i].tCurrent = partner.tCurrent;
              sens[i].age = 0;
            }
          }
}

bool pullPartner(String address) {
    sensor partner;
    partner.gid = 0;
    partner.tCurrent = DEVICE_DISCONNECTED_C;
    HTTPClient http;
    http.begin("http://" + address + PARTNER_SUFFIX);
    int httpCode = http.GET();
    if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
        String body = http.getString();
        if (body != "") {
          parsePartner(body);
        }
    }
  }
  http.end();
  return false;
}

extern String pull[];

uint32_t queryPartners() {
  if (WiFi.status() == WL_CONNECTED) {
    if (currentPartner < PARTNER_MAX_COUNT && pull[currentPartner] != "") {
      BUSY
      pullPartner(pull[currentPartner]);
      currentPartner++;
      IDLE
      return PARTNER_NEXT;
    } else {
      currentPartner = 0;
    }
  }
  return PARTNER_SLEEP;
}
