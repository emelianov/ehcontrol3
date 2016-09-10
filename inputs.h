//////////////////////////////////////////////////////
// EHControl2 2016.3 (c)2016, a.m.emelianov@gmail.com
// Inputs\Outputs definitions, constants and routines

#pragma once

#define INPUTS_COUNT 16
#define ANALOG_COUNT 4
#define CFG_INPUTS "/inputs.xml"
//Min & Max times of signal is changed to detect as click event
#define CLICK_TIME_MIN 100
#define CLICK_TIME_MAX 150
//Max time to detect signal change as long click event 
#define CLICK_LONG_TIME_MAX 500
#define ON_ON 1
#define ON_OFF 2

typedef void (*INcallback)();

struct gpio {
  bool on;
  bool old;
  int16_t pin;
  bool inv;
  uint16_t gid;
  String name;
  INcallback onOn;
  INcallback onOff;
  INcallback onClick;		//Not implemented
  INcallback onLongClick;	//Not implemented
  uint16_t age;
};
struct aio {
  bool on;
  int16_t pin;
  bool inv;
  uint16_t gid;
  String name;
  int16_t value;
  int16_t old;
  int16_t highMark;
  int16_t lowMark;
  INcallback onLower;
  INcallback onWithin;
  INcallback onHigher;
  uint16_t age;
};


gpio inputs[INPUTS_COUNT];
aio analogs[ANALOG_COUNT];
extern TinyXML xml;

void NIempty() {
}

bool readInputs() {
  int16_t i;

  for (i = 0; i < INPUTS_COUNT; i++) {
    inputs[i].name = "";
    inputs[i].onClick = NULL;
    inputs[i].onLongClick = NULL;
    inputs[i].onOff = NULL;
    inputs[i].onOn = NULL;
    inputs[i].pin = -1;
    inputs[i].age = DEVICE_AGE_DEF;
    inputs[i].gid = 0;
    inputs[i].old = false;
  };
  for (i = 0; i < ANALOG_COUNT; i++) {
    analogs[i].name = "";
    analogs[i].value = 0;
    analogs[i].pin = -1;
    analogs[i].gid = 0;
    analogs[i].age = DEVICE_AGE_DEF;
    analogs[i].old = 0;
    analogs[i].highMark = 0;
    analogs[i].lowMark = 0;
    analogs[i].onLower = NULL;
    analogs[i].onWithin = NULL;
    analogs[i].onHigher = NULL;
  }

  File configFile = SPIFFS.open(CFG_INPUTS, "r");
  if (configFile) {
   char c;
   xml.reset();
   xmlTag = "";
   xmlOpen = "";
   i = 0;
   int8_t j = 0;
   bool inputOpen = false;
   bool analogOpen = false;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlTag != "" || xmlOpen != "") {
       if 
      (xmlOpen.endsWith("/input")) {
        if (i < INPUTS_COUNT - 1) {
          if (inputOpen) {
            i++;
          } else {
            inputOpen = true;
          }
        } else {
          //Config error
        }
       } else if
      (xmlOpen.endsWith("/analog")) {
        if (j < ANALOG_COUNT - 1) {
          if (analogOpen) {
            j++;
          } else {
            analogOpen = true;
          }
        } else {
          //Config error
        }
       } else if
      (xmlTag.endsWith("analog/pin")) {
         analogs[j].pin = 1;
       } else if
      (xmlTag.endsWith("input/pin")) {
         inputs[i].pin = xmlData.toInt();
       } else if
      (xmlTag.endsWith("analog/gid")) {
        analogs[j].gid = xmlData.toInt();
       } else if
      (xmlTag.endsWith("input/gid")) {
        inputs[i].gid = xmlData.toInt();
       } else if
      (xmlTag.endsWith("analog/name")) {
        analogs[j].name = xmlData;
       } else if
      (xmlTag.endsWith("input/name")) {
        inputs[i].name = xmlData;
       } else if
      (xmlTag.endsWith("analog/low")) {
        analogs[j].lowMark = xmlData.toInt();
       } else if
      (xmlTag.endsWith("analog/high")) {
        analogs[j].highMark = xmlData.toInt();
       } else if
      (xmlTag.endsWith("input/inverse")) {
        inputs[i].inv = (xmlData.toInt() == 1);
       } else {
        // Config syntax error
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

void cbOn() {
  NOALERT
}
void cbOff() {
  ALERT
}
void inputEvent(uint8_t i, uint8_t tp, INcallback cb) {
  switch (tp) {
    case ON_ON:
      inputs[i].onOn = cb;
    break;
    case ON_OFF:
       inputs[i].onOff = cb;
    break;
  }
}


bool initInputs() {
  bool result = readInputs();
//  inputEvent(3, ON_ON, cbOn);
//  inputEvent(3, ON_OFF, cbOff);
  return result;
}

uint32_t updateInputs() {
  uint8_t i;
	for (i = 0; i < INPUTS_COUNT; i++) {
		if (inputs[i].pin >= 0) {
      inputs[i].age = 0;
			inputs[i].on = (digitalRead(inputs[i].pin)==LOW)?true:false;
			if (inputs[i].on != inputs[i].old) {
				inputs[i].old = inputs[i].on;
				if (inputs[i].on) {
				  if (inputs[i].onOn != NULL) {
				    inputs[i].onOn();
				  }
				} else {
          if (inputs[i].onOff != NULL) {
               inputs[i].onOff();
          }
				}
			}
		}
	}
  for (i = 0; i < ANALOG_COUNT; i++) {
    if (analogs[i].pin >= 0) {
      analogs[i].value = analogRead(A0);
      analogs[i].age = 0;
    }
    /*
    if
   (analogs[i].value > analogs[i].highMark && analogs[i].old <= analogs[i].highMark) {
      if (analogs[i].onHigher != NULL) analogs[i].onHigher();
    } else if
   (analogs[i].value > analogs[i].lowMark && (analogs[i].old <= analogs[i].lowMark || analogs[i].old >= analogs[i].highMark)) {
     if (analogs[i].onWithin != NULL) analogs[i].onWithin();
    } else if
   (analogs[i].value < analogs[i].highMark && analogs[i].old >= analogs[i].lowMark) {
      if (analogs[i].onLower != NULL) analogs[i].onLower();
    }
    analogs[i].old = analogs[i].value;
    */
  }
  return 100;
}
 
/*
bool saveInputs() {
   File configFile = SPIFFS.open(CFG_INPUTS, "w");
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
*/

