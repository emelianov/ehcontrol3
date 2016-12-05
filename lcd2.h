//////////////////////////////////////////////////////
// EHControl3 2016.4 (c)2016, a.m.emelianov@gmail.com
// I2C LCD 2016.4

#pragma once
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "sensors.h"
#include "relays.h"

#define CFG_LCD "/lcd.xml"
#ifndef LCD_ID
// #define LCD_ID    0x27
 #define LCD_ID    0x3F
#endif
#ifndef LCD_HEIGHT
 #define LCD_HEIGHT 2
#endif
#ifndef LCD_WIDTH
 #define LCD_WIDTH  16
#endif

#define LCD_INTERVAL 2000
#define LCD_BLINK 750
#define LCD_BLOCKS 8
#define LCD_TEXT " --- "
LiquidCrystal_I2C * lcd;

enum lcdBlockType {EMPTY, CLOCK, TEMPSENS, ASIS};
struct lcdBlock {
  uint8_t col;
  uint8_t row;
  lcdBlockType type;
  uint8_t index;
  String text;
  bool blink;
};
lcdBlock block[LCD_BLOCKS];
uint32_t lcdSleep = LCD_INTERVAL;
uint32_t lcdBlink = LCD_BLINK;
bool blinkOn = true;
int8_t wlLevel = 0;
uint32_t updateLcd() {
  blinkOn = !blinkOn;
  bool isAnyBlink = false;
  for (uint8_t i = 0; i < LCD_BLOCKS || block[i].type == EMPTY; i++) {
    lcd->setCursor(block[i].col, block[i].row);
    switch (block[i].type) {
      case CLOCK:
      {
        char  strTime[8];
        uint16_t  minutesFromMidnight = time(NULL) % 86400UL / 60;
        sprintf_P(strTime, PSTR("%02d:%02d"), (uint8_t)(minutesFromMidnight / 60), (uint8_t)(minutesFromMidnight % 60));
        lcd->print(strTime);
      }
      break;
      case TEMPSENS:
        if (use.heater) {
          block[i].blink = relays[block[i].index].on;  
        }
        if (use.sensors) {
          if (!block[i].blink || (block[i].blink && blinkOn)) {
            lcd->print((block[i].index >= DEVICE_MAX_COUNT || sens[block[i].index].tCurrent==DEVICE_DISCONNECTED_C)?" --- ":String(sens[0].tCurrent));
          } else {
            lcd->print(block[i].text);
            isAnyBlink = true;
          }
        }
      break;
      case ASIS:
        lcd->print(block[i].text);
      break;
    }
  }
#define WL_MIN -100
#define WL_MAX -24
//  lcd->print(" rssi: ");
//  lcd->print(WiFi.RSSI());
  int8_t level = -(WL_MIN - WiFi.RSSI())/10;
  if (wlLevel != level) {
    wlLevel = level;
    byte disp[8];
    for (uint8_t i = 0; i < 8; i++) {
      if (7 - i <= level) {
        disp[i] = B11111;
      } else {
        disp[i] = B00000;
      }
    }
    lcd->createChar(1, disp);
    lcd->setCursor(15,0);
    lcd->print("\1");
  }
//  lcd->setCursor(0,1);
//  lcd->print((sens[0].tCurrent==DEVICE_DISCONNECTED_C)?" --- ":String(sens[0].tCurrent));
//  lcd->setCursor(6,1);
//  lcd->print((sens[1].tCurrent==DEVICE_DISCONNECTED_C)?" --- ":String(sens[1].tCurrent));
//  lcd->setCursor(6,0);
//  lcd->print((sens[2].tCurrent==DEVICE_DISCONNECTED_C)?" --- ":String(sens[2].tCurrent));
//  lcd.print(" Mem: ");
//  lcd.print(ESP.getFreeHeap());
//  lcd.print(" RSSI: ");
//  lcd.print(WiFi.RSSI());
//  lcd->setCursor(12,1);
//  for (uint8_t i=0; i < RELAY_COUNT; i++) {
//    lcd->print(relays[i].on?"\2":" ");
//  }
  //lcd.print(" ");
  //lcd.print((ecoMode?"\2":" "));
  //lcd.setCursor(15,1);
  //lcd.print(digitalRead(D3)?"\2":" ");  
  return isAnyBlink?lcdBlink:lcdSleep;
}

bool readLcd() {
  uint8_t address = LCD_ID;
  uint8_t width   = LCD_WIDTH;
  uint8_t height  = LCD_HEIGHT;

  /*
  block[0].col = 0;
  block[0].row = 0;
  block[0].type = CLOCK;
  block[0].blink = false;
  block[1].col = 0;
  block[1].row = 1;
  block[1].type = TEMPSENS;
  block[1].index = 0;
  block[1].blink = false;
  block[2].type = EMPTY;
  */
  File configFile = SPIFFS.open(F(CFG_LCD), "r");
  if (configFile) {
   char c;
   xml.reset();
   xmlTag = "";
   xmlOpen = "";
   uint8_t i = 0;
   bool blockOpen = false;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
       if
      (xmlTag != "" || xmlOpen != "") {
       if 
      (xmlOpen.endsWith("/block")) {
       if (i < LCD_BLOCKS - 1) {
          if (blockOpen) {
            i++;
          } else {
            blockOpen = true;
          }
          block[i].type = EMPTY;
          block[i].col = 0;
          block[i].row = 0;
          block[i].text = LCD_TEXT;
          block[i].index = 0;
          block[i].blink = false;
        } else {
          //Config error
        } 
       } else if 
      (xmlTag.endsWith(F("/address"))) {
        address = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/width"))) {
        width = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/height"))) {
        height = xmlData.toFloat();
       } else if
      (xmlTag.endsWith(F("/block/row"))) {
        block[i].row = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/block/col"))) {
        block[i].col = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/block/type"))) {
         if
        (xmlData == "clock") {
          block[i].type = CLOCK;
        } else if
         (xmlData == "temp") {
          block[i].type = TEMPSENS;
        } else {
          block[i].type = ASIS;
        } 
       } else if
      (xmlTag.endsWith(F("/block/value")) || xmlTag.endsWith(F("/block/index"))) {
        block[i].index  = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/block/text"))) {
        block[i].text  = xmlData;
       } else if
      (xmlTag.endsWith(F("/block/blink"))) {
        block[i].blink  = xmlData.toInt() == 1;
       } else if
      (xmlTag.endsWith(F("/sleeptime"))) {
        lcdSleep  = xmlData.toInt();
       } else if
      (xmlTag.endsWith(F("/blinktime"))) {
        lcdBlink  = xmlData.toInt();
       }
     }
    xmlTag = "";
    xmlOpen = "";
   }
   configFile.close();
  }
  lcd = new LiquidCrystal_I2C(address, width, height);
  return true;  
}

bool initLcd() {
  readLcd();
  lcd->begin();
  byte disp[8];
  disp[0] = B00000;
  disp[1] = B00000;
  disp[2] = B00000;
  disp[3] = B11111;
  disp[4] = B00000;
  disp[5] = B11111;
  disp[6] = B01110;
  disp[7] = B00100;
  lcd->createChar(2, disp);
  taskAdd(updateLcd);
  return true;
}
