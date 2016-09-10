////////////////////////////////////////////
// EHCtl (c)2016, a.m.emelianov@gmail.com
// Local clock 2016.3

#pragma once
extern int8_t timeZone;
extern IPAddress timeServer;
#define NTP_CHECK_DELAY 300000;
//extern uint32_t currentTime;
//extern uint32_t clockLastUpdate;
#ifdef ESP8266

/*uint32_t initNtp() {
  configTime(timeZone * 3600, 0, timeServer.toString().c_str());
  addTaskWithDelay(checkNtp, NTP_CHECK_DELAY);
  return 0;
}
*/
uint32_t initNtp() {
  if (time(NULL) == 0) {
    configTime(timeZone * 3600, 0, timeServer.toString().c_str());
    return NTP_CHECK_DELAY;
  }
  return 0;
}
#else
uint32_t initNtp() {
  return 0;
}
uint32_t clockUpdate() {
  uint32_t mi = millis();
  currentTime += (mi - clockLastUpdate)/1000;
  clockLastUpdate = mi;
  return 1000;
}
#endif
