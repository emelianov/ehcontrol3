#ehcontrol3 2016.3.1
ESP8266-based Home automation solution

*&copy;2016, Alexander Emelianov (a.m.emelianov@gmail.com)*

###1. Introduction

###2. Librares used
* Arduino (https://github.com/arduino/Arduino)
* ESP8266 core for Arduino (https://github.com/esp8266/Arduino)
* Run 2016.1 (https://github.com/emelianov/Run)
* TinyXML (https://github.com/adafruit/TinyXML)
* Library for the LiquidCrystal LCD display connected to an Arduino board (https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library)

###3. Supported hardware
At the moment code is only tested with NodeMCU 1.0 board.

###4. Initial configuration
* Load code to ESP module with Arduino IDE or any way You able to.
* Power on module.
* Look open Wireless network named alike 'ESP_12345' and connect.
* Open address http://192.168.4.1/config with your browser.
* Press Format FileSystem.
* Modify settings and press Save button.
* Reboot.

###5. Building modules
* Core and inputs
* Temperature sensors
* Heater control
