#include <TinyXML.h>
#include "FS.h"
extern IPAddress timeServer;
extern int8_t timeZone;

char* xmlTag = NULL;
char* xmlValue = NULL;
char* xmlAttrib = NULL;

uint8_t buffer[150];

void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
  if(statusflags & STATUS_TAG_TEXT) {
    xmlTag = tagName;
    xmlValue = data;
  }
}

//File configFile = SPIFFS.open(fname, "r");
class xmlO {
  uint8_t buffer[150];
  TinyXML xml;
  WiFiClient net = NULL;
  File configFile = NULL;
  void init() {
   xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  }
  xmlO(File& fromFile) {
    configFile = fromFile;
    net = NULL;
    init();
  }
  xmlO(WiFiClient& fromNet) {
    net = fromNet;
    configFile = NULL;
    init();
  }
  void push() {
    if (configFile) {
     xml.processChar(configFile.read());
    } else if (net) {
      xml.processChar(net.read());
    }
  }
//  static void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
//  }
};

