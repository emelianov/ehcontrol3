//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2017, a.m.emelianov@gmail.com
// Inputs\Outputs definitions, constants and routines

#pragma once
#include <pcf8574_esp.h>
#include "helper.h"

#define I2C_OFFSET 100
#define CFG_INPUTS "/inputs.xml"
#define INPUTS_SLEEP 100

PCF857x * i2cGpio = NULL;
uint8_t i2cAddr = 0;
uint8_t i2cBase = I2C_OFFSET;
uint32_t i2cSleep = INPUTS_SLEEP;
uint8_t gread(uint8_t pin) {
  if (pin >= I2C_OFFSET) {
    return i2cGpio->read(pin - I2C_OFFSET);
  } else {
    return digitalRead(pin);
  }
}
void gwrite(uint8_t pin, uint8_t value) {
  if (pin >= I2C_OFFSET) {
    if (i2cGpio != NULL) {
      i2cGpio->write(pin - I2C_OFFSET, value);
    }
  } else {
    digitalWrite(pin, value);
  }
}
void gmode(uint8_t pin, uint8_t mode) {
  if (pin >= I2C_OFFSET) {
    if (i2cGpio != NULL) {
      if (mode == INPUT) {
        i2cGpio->read(pin - I2C_OFFSET);
      } else {
        i2cGpio->write(pin - I2C_OFFSET, HIGH);
      }
    }
  } else {
    pinMode(pin, mode);
  }
}
struct inputCfg {
  uint16_t gid;
  uint16_t index;
  String name;
  uint16_t pin;
};
#define INPUT_MAX 16
typedef void (*cb)();
uint16_t gpsig[INPUT_MAX];
uint16_t gppin[INPUT_MAX];
void ICACHE_RAM_ATTR intr(uint8_t i) { item[gpsig[i]]->signal++; item[gpsig[i]]->current = digitalRead(gppin[i]); }
void ICACHE_RAM_ATTR intr0() { intr(0); }
void ICACHE_RAM_ATTR intr1() { intr(1); }
void ICACHE_RAM_ATTR intr2() { intr(2); }
void ICACHE_RAM_ATTR intr3() { intr(3); }
void ICACHE_RAM_ATTR intr4() { intr(4); }
void ICACHE_RAM_ATTR intr5() { intr(5); }
cb gpintr[INPUT_MAX] = {intr0, intr1, intr2, intr3, intr4, intr5 };

bool readInputs() {
   uint8_t i2cInit = 0;
  inputCfg inps[INPUT_MAX];
  for (uint8_t i = 0; i < INPUT_MAX; i++) {
    inps[i].gid = 0;
    inps[i].index = 0xFFFF;
    inps[i].name = "";
    inps[i].pin = 0;
  }
  if (use.apSwitch) {
    inps[1].gid = 0;
    inps[1].index = 0;
  }
  inps[0].pin = D0;
  inps[1].pin = D3;
  inps[2].pin = D5;
  inps[3].pin = D6;
  inps[4].pin = D7;
  inps[5].pin = D8;
  CfgEntry cfg[] = {CfgEntry(F("/D0/gid"),   &inps[0].gid),
                    CfgEntry(F("/D0/index"), &inps[0].index),
                    CfgEntry(F("/D0/name"),  &inps[0].name),
                    CfgEntry(F("/D3/gid"),   &inps[1].gid),
                    CfgEntry(F("/D3/index"), &inps[1].index),
                    CfgEntry(F("/D3/name"),  &inps[1].name),
                    CfgEntry(F("/D5/gid"),   &inps[2].gid),
                    CfgEntry(F("/D5/index"), &inps[2].index),
                    CfgEntry(F("/D5/name"),  &inps[2].name),
                    CfgEntry(F("/D6/gid"),   &inps[3].gid),
                    CfgEntry(F("/D6/index"), &inps[3].index),
                    CfgEntry(F("/D6/name"),  &inps[3].name),
                    CfgEntry(F("/D7/gid"),   &inps[4].gid),
                    CfgEntry(F("/D7/index"), &inps[4].index),
                    CfgEntry(F("/D7/name"),  &inps[4].name),
                    CfgEntry(F("/D8/gid"),   &inps[5].gid),
                    CfgEntry(F("/D8/index"), &inps[5].index),
                    CfgEntry(F("/D8/name"),  &inps[5].name),
                    CfgEntry(F("/i2c/addr"), &i2cAddr),
                    CfgEntry(F("/i2c/init"), &i2cInit),
                    CfgEntry(F("/i2c/addr"), &i2cBase),
                    CfgEntry(F("/sleeptime"),&i2cSleep)
                   };
  if (!cfgParse(F(CFG_INPUTS), cfg, sizeof(cfg)/sizeof(cfg[0]))) {
 //   return false;
  }
  for (uint8_t i = 0; i < INPUT_MAX; i++) {
    if (inps[i].index != 0xFFFF) {
      if (item[inps[i].index] == NULL) {
        item[inps[i].index] = new Item();
      }
      pinMode(inps[i].pin, INPUT);
      gpsig[i] = inps[i].index;
      gppin[i] = inps[i].pin;
      attachInterrupt(inps[i].pin, gpintr[i], CHANGE);
    }
  }
   if (i2cAddr > 0) {
    Wire.begin();
    i2cGpio = new PCF857x(i2cAddr, &Wire);
    i2cGpio->begin();
    i2cGpio->write8(i2cInit);
   }
   return true;
  }

bool initInputs() {
  bool result = readInputs();
  return result;
}
/*
uint32_t updateInputs() {
  uint8_t i;
	for (i = 0; i < INPUTS_COUNT; i++) {
		if (inputs[i].pin >= 0) {
      inputs[i].age = 0;
			inputs[i].on = (gread(inputs[i].pin)==LOW);
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
//  }
//  return inputsSleep;
//}

