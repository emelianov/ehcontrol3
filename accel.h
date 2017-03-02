//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2017, a.m.emelianov@gmail.com
// I2C ADXL345 2017.1

#pragma once
#include <Wire.h>
#include <ADXL345.h>
#include "helper.h" 

#define CFG_ADXL "/accel.xml"
#define ADXL_INTR   D6
ADXL345* adxl;
uint16_t adxlIntrPin = ADXL_INTR;
bool adxlTapEnable = false;
bool adxlDoubleTapEnable = false;
void ICACHE_RAM_ATTR adxlIntr() {
  event.adxl++;
}
uint32_t adxlTap() {
  byte interrupts = adxl->getInterruptSource();
  if(adxlDoubleTapEnable && adxl->triggered(interrupts, ADXL345_DOUBLE_TAP)){
    event.doubleTap++;
  } else if(adxlTapEnable && adxl->triggered(interrupts, ADXL345_SINGLE_TAP)){
    event.tap++;
  }
  return RUN_NEVER;
}

bool initAccel() {
  CfgEntry cfg[] = {CfgEntry(F("/pin"),      &adxlIntrPin),
                    CfgEntry(F("/tap"),  &adxlTapEnable),
                    CfgEntry(F("/doubletap"),  &adxlDoubleTapEnable)
                   };
  cfgParse(F(CFG_ADXL), cfg, sizeof(cfg)/sizeof(cfg[0]));
  pinMode(adxlIntrPin, INPUT);
  attachInterrupt(adxlIntrPin, adxlIntr, RISING);
  Wire.begin();
  adxl = new ADXL345();
  adxl->powerOn();
  adxl->setActivityX(0);
  adxl->setActivityY(0);
  adxl->setActivityZ(0);
  adxl->setInactivityX(0);
  adxl->setInactivityY(0);
  adxl->setInactivityZ(0);
  if (adxlTapEnable || adxlDoubleTapEnable) {
  //look of tap movement on this axes - 1 == on; 0 == off
    adxl->setTapDetectionOnX(1);
    adxl->setTapDetectionOnY(1);
    adxl->setTapDetectionOnZ(1);
  //set values for what is a tap, and what is a double tap (0-255)
    adxl->setTapThreshold(50); //62.5mg per increment
    adxl->setTapDuration(15); //625μs per increment
    adxl->setDoubleTapLatency(80); //1.25ms per increment
    adxl->setDoubleTapWindow(200); //1.25ms per increment
  //register interupt actions - 1 == on; 0 == off
  } else {
    adxl->setTapDetectionOnX(0);
    adxl->setTapDetectionOnY(0);
    adxl->setTapDetectionOnZ(0);    
  }
  if (adxlTapEnable) {
    adxl->setInterruptMapping( ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN );
    adxl->setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 1);
  }
  if (adxlDoubleTapEnable) {
    adxl->setInterruptMapping( ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN );
    adxl->setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 1);
  }
  adxl->getInterruptSource();
  taskAddWithSemaphore(adxlTap, &event.adxl);
  return true;
}
