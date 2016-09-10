//////////////////////////////////////////////////////
// EHControl2 2016 (c)2016, a.m.emelianov@gmail.com
// NTP Client 2016.2.1

#pragma once

#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <WiFiUdp.h>
#else
 #include "network.h"
#endif
//#include "syslog.h"
#include <Run.h>

#define NTP_USELESS   40
#define NTP_PORT      123
#define NTP_PACKET_SIZE 48
#define MAX_NETWORK_ERRORS 8
#define NTP_DELAY 150
#define NTP_RETRY 20
#define NTP_INTERVAL 300000L
#define NTP_FAIL_DELAY 30000L

extern uint32_t clockLastUpdate;
extern bool clockIsSet;
extern uint32_t currentTime;
extern WiFiUDP udp;
extern int8_t timeZone;
extern IPAddress timeServer;
uint8_t networkErrors = MAX_NETWORK_ERRORS;

uint32_t getNtpTime();

uint32_t reciveNtpData() {
  uint8_t i;
   if (udp.parsePacket() == NTP_PACKET_SIZE) {
    for (i = 0; i < NTP_USELESS; ++i)
     udp.read();
     uint32_t time = udp.read();  // NTP time
    for (i = 1; i < 4; i++)
     time = time << 8 | udp.read();
     time += (udp.read() > 115 - 150/8);
     udp.flush();
     currentTime      = time - 2208988800ul + timeZone * 3600;  
     clockLastUpdate  = millis();
     clockIsSet       = true;
//     LOG_NTP;
     taskAddWithDelay(getNtpTime, NTP_INTERVAL);
     IDLE
     return 0; 
    } else {
        networkErrors--;
        if (networkErrors < 1) {
          networkErrors = MAX_NETWORK_ERRORS;
#ifndef ESP8266
          resetEthernet();
#endif
          taskAddWithDelay(getNtpTime, NTP_INTERVAL);
	  IDLE
          return 0;
        }
    }
    return NTP_DELAY;
}

uint32_t getNtpTime()
{
  uint8_t i;
  uint32_t ntpFirstFourBytes[NTP_PACKET_SIZE/sizeof(uint32_t)];
  BUSY
  memset(ntpFirstFourBytes, 0, NTP_PACKET_SIZE);
  ntpFirstFourBytes[0] = 0xEC0600E3; // NTP request header
  udp.flush();
  if ( udp.beginPacket(timeServer, NTP_PORT) && \
  		udp.write((uint8_t *)&ntpFirstFourBytes, NTP_PACKET_SIZE) == NTP_PACKET_SIZE && \
  		udp.endPacket()) {
   taskAdd(reciveNtpData);
   return 0;
  } else {
   networkErrors--;
   if (networkErrors < 1) {
    networkErrors = MAX_NETWORK_ERRORS;
#ifndef ESP8266
    resetEthernet();
#endif
    IDLE
    return NTP_INTERVAL;
   }
  }
  IDLE
  return NTP_DELAY;
}
