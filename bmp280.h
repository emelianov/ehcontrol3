//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2017, a.m.emelianov@gmail.com
// I2C BMP280 2017.1

#pragma once
#include <Wire.h>
#include <BMP280.h>
#include "helper.h" 

#define CFG_BMP "/bmp.xml"
#define BMP_SLEEP 60000
BMP280 * bmp;
uint32_t bmpSleep = BMP_SLEEP;
uint16_t bmpTIndex = 0xFFFF;
uint16_t bmpPIndex = 0xFFFF;
bool bmpMmHg = false;

// get measurment result
uint32_t bmpUpdateGet() {
  double bmpT;
  double bmpP;
  uint8_t result = bmp->getTemperatureAndPressure(bmpT,bmpP);
    if(result != 0) {
      if (bmpTIndex != 0xFFFF) {
        if (item[bmpTIndex] == NULL) {
          item[bmpTIndex] = new Item();
        }
        item[bmpTIndex]->current = bmpT;
      }
      if (bmpPIndex != 0xFFFF) {
        if (item[bmpPIndex] == NULL) {
          item[bmpPIndex] = new Item();
        }
        if (bmpMmHg) {
          bmpP /= 1.33322;
        }
        item[bmpPIndex]->current = bmpP;
      }
    }
  return RUN_DELETE;
}
// start measusment
uint32_t bmpUpdate() {
	uint8_t result = bmp->startMeasurment();
	if(result != 0) {
     taskAddWithDelay(bmpUpdateGet, result);
  }
  return BMP_SLEEP;
}

bool bmpInit() {
  uint16_t bmpTGid = 0;
  uint16_t bmpPGid = 0;
  String bmpTName = "T";
  String bmpPName = "P";
  CfgEntry cfg[] = {CfgEntry("/t/gid",   &bmpTGid),
                    CfgEntry("/t/index",  &bmpTIndex),
                    CfgEntry("/t/name",  &bmpTName),
                    CfgEntry("/p/gid",	&bmpPGid),
                    CfgEntry("/p/index",  &bmpPIndex),
                    CfgEntry("/p/name",  &bmpPName),
                    CfgEntry("/p/mmhg",  &bmpMmHg),
                    CfgEntry("/sleeptime",  &bmpSleep)
                   };
  if (!cfgParse(CFG_BMP, cfg, sizeof(cfg)/sizeof(cfg[0]))) {
    return false;
  }
  Wire.begin();
  bmp = new BMP280();
  if(!bmp->begin()){
  	return false;
  }
  bmp->setOversampling(4);
  if (bmpTIndex > MAX_ITEMS) {
    bmpTIndex = 0xFFFF;
  }
  if (bmpTIndex > MAX_ITEMS) {
    bmpPIndex = 0xFFFF;
  }
  if (bmpTIndex != 0xFFFF) {
    item[bmpTIndex] = new Item();
  	item[bmpTIndex]->gid = bmpTGid;
    item[bmpTIndex]->name = bmpTName;
  }
  if (bmpPIndex != 0xFFFF) {
    item[bmpPIndex] = new Item();
    item[bmpPIndex]->gid = bmpPGid;
    item[bmpPIndex]->name = bmpPName;
  }
  taskAdd(bmpUpdate);
  return true;
}
