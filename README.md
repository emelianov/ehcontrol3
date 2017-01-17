#ehcontrol3 2016.5
ESP8266-based Home automation solution

*&copy;2017, Alexander Emelianov (a.m.emelianov@gmail.com)*

###1. Introduction
This is system that can allow group of ESP8266-based devices share data of gpio, 1-Wire temperature sensors and analog inputs. Exchange and confiuration of system is based on XML. HTTP is used to control, monitor and interaction between devices.
At the moment system can be configured as heater controller or simple thermometer.

###2. Resources used
* Arduino (https://github.com/arduino/Arduino)
* ESP8266 core for Arduino (https://github.com/esp8266/Arduino)
* Run 2016.2 (https://github.com/emelianov/Run)
* TinyXML (https://github.com/adafruit/TinyXML)
* Library for the LiquidCrystal LCD display connected to an Arduino board (https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library)
* PCF8574_ESP (https://github.com/WereCatf/PCF8574_ESP)
* ADXL345 (https://github.com/emelianov/ADXL345)
* NeoPixelBus (https://github.com/Makuna/NeoPixelBus)

###3. Supported hardware
At the moment code is only tested on NodeMCU 1.0 board.

###4. Initial configuration mode for NodeMCU
* Load code to module with Arduino IDE or any way You able to.
* Power on module.
* Press and hold for 2 seconds Flash button. On-board led starts lazy blinking.
* Look open Wireless network named alike 'ESP_123456' and connect.
* Open address http://192.168.4.1/config with your browser. Use default admin name and password (admin/password3).
* Press Format FileSystem button to initialize internal file system. Be patient to wait operation completes.
*WARNING! All file system data will be lost.*
* Upload and edit configuration files.
* Reboot.

The same procedure can be used in case of lost network access to device or lost password.

Device automaticly enters to Initial configuration mode if no *global.xml* file found.