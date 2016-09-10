//////////////////////////////////////////////////////
// EHControl2 2016.2 (c)2016, a.m.emelianov@gmail.com
// I2C LCD 2016.2.1

#pragma once
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "sensors.h"
#include "relays.h"
#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <time.h>
#endif

#ifndef LCD_ID
 #define LCD_ID    0x27
#endif
#ifndef LCD_HEIGHT
 #define LCD_HEIGHT 2
#endif
#ifndef LCD_WIDTH
 #define LCD_WIDTH  16
#endif

#define LCD_INTERVAL 5000

//extern uint32_t currentTime;

LiquidCrystal_I2C lcd(LCD_ID, LCD_WIDTH, LCD_HEIGHT);
void initLcd() {
 lcd.begin();
// lcd.setCursor(9,1);
// lcd.print("Boot");
 byte disp[8];
 disp[0] = B00000;
 disp[1] = B00000;
 disp[2] = B00000;
 disp[3] = B11111;
 disp[4] = B00000;
 disp[5] = B11111;
 disp[6] = B01110;
 disp[7] = B00100;
 lcd.createChar(2, disp);
 //taskAdd(updateLcd);
}
int8_t wlLevel = 0;
uint32_t updateLcd() {
  char  strTime[8];
#ifdef ESP8266
  uint16_t  minutesFromMidnight = time(NULL) % 86400UL / 60;
#else
  uint16_t  minutesFromMidnight = currentTime % 86400UL / 60;
#endif
  sprintf_P(strTime, PSTR("%02d:%02d"), (uint8_t)(minutesFromMidnight / 60), (uint8_t)(minutesFromMidnight % 60));
  lcd.setCursor(0,0);
  lcd.print(strTime);
#ifdef ESP8266
#define WL_MIN -90
#define WL_MAX -24
  lcd.print(" rssi: ");
  lcd.print(WiFi.RSSI());
  int8_t level = (WL_MAX - WiFi.RSSI())/10;
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
    lcd.createChar(1, disp);
    lcd.setCursor(15,0);
    lcd.print("\1");
  }
  lcd.setCursor(0,1);
  lcd.print((sens[0].tCurrent==DEVICE_DISCONNECTED_C)?" --- ":String(sens[0].tCurrent));
//  lcd.print(" Mem: ");
//  lcd.print(ESP.getFreeHeap());
//  lcd.print(" RSSI: ");
//  lcd.print(WiFi.RSSI());
  lcd.print(" ");
  for (uint8_t i=0; i < RELAY_COUNT; i++) {
    if (relays[i].on) {
      lcd.print("\2");
    } else {
      lcd.print(" ");
    }
  }
  lcd.setCursor(15,1);
  lcd.print(digitalRead(D3)?"\2":" ");  
#endif
  return LCD_INTERVAL;
}

