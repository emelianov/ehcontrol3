//////////////////////////////////////////////////////
// EHControl3 2016.5 (c)2016, a.m.emelianov@gmail.com
// I2C ADXL345 2016.5

#pragma once
#include <Wire.h>
#include <ADXL345.h>

#define CFG_LCD "/accel.xml"
#define ACCEL_SLEEP 100
#define ACCEL_BUFFER 600
#define ACCEL_DRIFT 2
#define ACCEL_FLUSH 3600000UL
#define ACCEL_FILE "/accel.csv"
struct accelXYZ {
int16_t x;
int16_t y;
int16_t z;
time_t tm;
};
ADXL345 * adxl;
uint32_t accelSleep	= ACCEL_SLEEP;
accelXYZ accelBuffer[ACCEL_BUFFER];
uint16_t accelCount = ACCEL_BUFFER;
uint16_t accelCurrent = 0;
uint8_t accelDrift = ACCEL_DRIFT;
String filename = ACCEL_FILE;

bool readAccel() {
  File configFile = SPIFFS.open(F(CFG_LCD), "r");
  if (configFile) {
   char c;
   xml.reset();
   xmlTag = "";
   xmlOpen = "";
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
       if
      (xmlTag != "" || xmlOpen != "") {
       if 
      (xmlTag.endsWith(F("/drift"))) {
        accelDrift  = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/filename"))) {
        filename  = xmlData;
       } else if
      (xmlTag.endsWith(F("/sleeptime"))) {
        accelSleep  = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/id"))) {
//        accelId  = xmlData.toInt();
       }
     }
    xmlTag = "";
    xmlOpen = "";
   }
   configFile.close();

   //accelBuffer = (accelXYZ*)malloc(accelCount * sizeof(accelXYZ));
   //if (accelBuffer == NULL) ALERT;
   accelCurrent = 0;
   accelBuffer[0].x = 0;
   accelBuffer[0].y = 0;
   accelBuffer[0].z = 0;
   accelBuffer[0].tm = 0;
  }
  return true;  
}

bool accelFlush() {
    char buf[100];
    bool result = true;
    File accelFile = SPIFFS.open(filename, "a");
    uint16_t j = (accelCurrent >= accelCount)?accelCount-1:accelCurrent-1;
    if (accelFile) {
     for (uint16_t i = 0; i < j; i++) {
      sprintf(buf, "%d;%d;%d;%d\n", accelBuffer[i].x, accelBuffer[i].y, accelBuffer[i].z, accelBuffer[i].tm);
      if (!accelFile.write((uint8_t*)buf, strlen(buf))) {
        ALERT
        result = false;
      }
     }
     accelFile.close();
    } else {
      result = false;
    }
    accelBuffer[0].x = accelBuffer[j+1].x;
    accelBuffer[0].y = accelBuffer[j+1].y;
    accelBuffer[0].z = accelBuffer[j+1].z;
    accelBuffer[0].tm = accelBuffer[j+1].tm;
    accelCurrent = 0;
    return result;
}
uint32_t accelForceFlush() {
  accelFlush();
  return ACCEL_FLUSH;
}
uint32_t accelUpdate() {
  	int16_t x,y,z; 
	accelCurrent++;
	if (accelCurrent >= accelCount) {
    accelFlush();
	}
	adxl->readAccel(&x, &y, &z);
  if (abs(accelBuffer[accelCurrent-1].x - x) > accelDrift || abs(accelBuffer[accelCurrent-1].y - y) > accelDrift || abs(accelBuffer[accelCurrent-1].y - y) > accelDrift) {
 	  accelBuffer[accelCurrent].x = x;
	  accelBuffer[accelCurrent].y = y;
	  accelBuffer[accelCurrent].z = z;
    accelBuffer[accelCurrent].tm = time(NULL);
  } else {
    if (accelCurrent > 1) accelCurrent--;
  }
	return accelSleep;
}

bool initAccel() {
  readAccel();
  Wire.begin();
  adxl = new ADXL345();
  adxl->powerOn();
  adxl->setRangeSetting(2);
  taskAdd(accelUpdate);
  taskAddWithDelay(accelForceFlush, ACCEL_FLUSH);
  return true;
}
