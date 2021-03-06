//////////////////////////////////////////////////////
// EHControl3 2016.4 (c)2016, a.m.emelianov@gmail.com
// Heater control

#pragma once
 
#define BURNER 0      //Горелка
#define ZONE1 1       //Насос 1 этаж
#define ZONE2 2       //Насос 2 этаж
#define FLOOR 3       //Насос теплый пол

#define TIN 0         //Подача котла
#define TZONE1 1      //Температура на 1 этаже
#define TZONE2 2      //Температура на 2 этаже
#define TFLOOR_OUT 3  //Обратка теплый пол
#define TOUT 4        //Обратка котла
#define TZONE1_OUT 5  //Обратка 1 этаж
#define TZONE2_OUT 6  //Обратка 2 этаж
#define TOUTSIDE 11   //Температура на улице

#define TGET(S) sens[S].tCurrent
#define TAGE(S) sens[S].age
#define IS_ON(S) relays[S].on
//#define TCUR(S) sens[S].tCurrent
#define TCUR(S) temps[S]
#define THI(S) relays[S].tHi
#define TLOW(S) relays[S].tLow
#define IS_IN(S) ins[S].on
#define TIDLE tIdle
#define CLOCK_IS_SET time(NULL)
//#define IS_ECO digitalRead(ECO_PIN) == HIGH || ecoMode
#define IS_ECO ecoMode
#define TMAX tMax
#define IS_NIGHT isNight
#define SHOULD_ON(S,R) TCUR(S) < TLOW(R)
#define SHOULD_OFF(S,R) TCUR(S) > THI(R)

#define T_ECO_DELTA dTeco
#define T_DAY_DELTA dTday
#define T_NIGHT_DELTA dTnight
#define T_BURNER_DELTA dTburner

float tIdle = 40;
float tMax = 80;

bool ecoMode = false;
float temps[3];

uint32_t switchSchedule() {
  uint16_t minutesFromMidnight;
  if (outside > -1 && outside < DEVICE_MAX_COUNT) {
    if (sens[outside].age > AGER_EXPIRE || sens[outside].tCurrent == DEVICE_DISCONNECTED_C) {
      relays[BURNER].tHi = relays[BURNER].t[DAY];      
    } else {
      relays[BURNER].tHi = eqTemp(sens[outside].tCurrent);
    }
  } else if (IS_ECO) {
    relays[BURNER].tHi = relays[BURNER].t[NIGHT];
  } else {
    relays[BURNER].tHi = relays[BURNER].t[DAY];
  }
  relays[BURNER].tLow = relays[BURNER].tHi - T_BURNER_DELTA;
  tIdle = relays[BURNER].t[ECO];
  tMax = relays[BURNER].t[OTHER];
  for(uint8_t i = 1; i < RELAY_COUNT; i++) {
    minutesFromMidnight = time(NULL) % 86400UL / 60;
    if (IS_ECO) {							// Eco mode
        relays[i].tHi = relays[i].t[ECO];
        relays[i].tLow = relays[i].t[ECO] - T_ECO_DELTA;       
    } else {								// Normal mode
      if (relays[i].onT2 < relays[i].offT2)	// |   |T1|####|T2|   |
       relays[i].isT2 = (minutesFromMidnight > relays[i].onT2 && minutesFromMidnight < relays[i].offT2);
      else									// |###|T1|   |T2|###|
       relays[i].isT2 = (minutesFromMidnight > relays[i].onT2 || minutesFromMidnight < relays[i].offT2);
      if (relays[i].isT2 && CLOCK_IS_SET) {	// Night
        relays[i].tHi = relays[i].t[NIGHT];
        relays[i].tLow = relays[i].t[NIGHT] - T_NIGHT_DELTA;
        relays[i].isT2 = true;
      } else { 								// Day or clock not set
        relays[i].tHi = relays[i].t[DAY];
        relays[i].tLow = relays[i].t[DAY] - T_DAY_DELTA;
        relays[i].isT2 = false;
      }
    }
  }  
  return 30000;  //30sec
}

bool fillRelays() {
  if (TAGE(TIN) > AGER_EXPIRE || TGET(TIN) == DEVICE_DISCONNECTED_C) {
    ALERT
    OFF(BURNER);
    ON(ZONE1);
    ON(ZONE2);
    ON(FLOOR);
    return false;
  }
  NOALERT
  temps[TIN]    = TGET(TIN);
  temps[TZONE1]  = TGET(TZONE1);
  temps[TZONE2]  = TGET(TZONE2);
   if 
  (TAGE(TZONE1) <= AGER_EXPIRE && TAGE(TZONE2) <= AGER_EXPIRE) {
    return true;
   } else if
  (TAGE(TZONE1) > AGER_EXPIRE && TAGE(TZONE2) <= AGER_EXPIRE) {
    temps[TZONE1] = TGET(ZONE2);
    return true;
   } else if 
  (TAGE(TZONE2) > AGER_EXPIRE && TAGE(TZONE1) <= AGER_EXPIRE) {
    temps[TZONE2] = TGET(ZONE1);
    return true;
  }
  temps[TZONE1] = 20;
  temps[TZONE2] = 20;
  return true;
}

uint32_t lazyRelays() {
  if (fillRelays()) {
   if (TCUR(TIN) < TMAX)
   {// If Boiler is not overheat
  	if (relays[FLOOR].isT2 || TCUR(TIN) < TIDLE || IS_ECO) {
  		OFF(FLOOR);
  	} else {
      ON(FLOOR);
    }
   }
  }
  return 10000;
}

uint32_t switchRelays() {
  if (fillRelays()) {
  //OFF(BURNER);
  // Zone_1 ON/OFF
  if (SHOULD_ON(TZONE1, ZONE1))     ON (ZONE1);
  if (SHOULD_OFF(TZONE1, ZONE1))    OFF(ZONE1);
  // Zone_2 ON/OFF
  if (SHOULD_ON(TZONE2, ZONE2))     ON (ZONE2);
  if (SHOULD_OFF(TZONE2, ZONE2))    OFF(ZONE2);
  // Burner ON/OFF
  if (IS_ON(ZONE1) || IS_ON(ZONE2))
  {// If Zone1 or Zone2 is ON
    if (SHOULD_ON(TIN, BURNER))     ON(BURNER);      
    if (SHOULD_OFF(TIN, BURNER))    OFF(BURNER);
  } else
  { // If Zone1 and Zone2 is OFF
    OFF(BURNER)    
/*    if (IS_ON(FLOOR))
    {// If Floor is ON
    } else // If Floor is OFF
     if (TCUR(TIN) > TIDLE)
     { // If Burner temperature more than Idle
       //if (TCUR(TIN) > tBoilerFloorHi) ON(ZONE1);		//Commented as summer mode is not implemented
       //ON(FLOOR);
      }
*/
  }
/*
  if (TCUR(TIN) < TIDLE)
  {// If Boiler is cold
      OFF(ZONE1);
      OFF(ZONE2);
      OFF(FLOOR);
  }
  */
  if (TCUR(TIN) > TMAX)
  {// If Boiler is overheat
      OFF(BURNER);
      ON(ZONE1);
      ON(ZONE2);
      ON(FLOOR);
  }
}
  SWITCH	//Do actual relay switching
  return 5000;   //5sec
}
