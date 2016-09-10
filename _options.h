////////////////////////////////////////////////////
// EHControl 2016.3
// Options 

#pragma once


//D0 LED
//D1 I2C
//D2 I2C
//#define PIN_R1	D3
#define PIN_ACT D4		//Net LED
#define PIN_ALERT 16
//#define PIN_R2	D5
#define ONEWIRE_PIN 12	//D6
#define PIN_R1	D7
#define	PIN_R2	D8
#define PIN_R3	D9
#define PIN_R4	D10

#define WIFI_RETRY_DELAY 1000

#define BUSY digitalWrite(PIN_ACT, 0);
#define IDLE digitalWrite(PIN_ACT, 1);

#define ALERT   digitalWrite(PIN_ALERT, LOW);
#define NOALERT digitalWrite(PIN_ALERT, HIGH);


#define LCD_I2C
#define UDP_PORT 33666

#define AGER_INTERVAL 15000
#define AGER_EXPIRE 20


