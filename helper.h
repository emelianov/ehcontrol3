//////////////////////////////////////////////////////
// EHControl3 2017.1 (c)2016, a.m.emelianov@gmail.com
// Config processor helper

#pragma once
#include <FS.h>
#include <TinyXML.h>

//XML processor settings
String xmlOpen;
String xmlTag;
String xmlData;
String xmlAttrib;
TinyXML xml;
uint8_t buffer[150];

void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
  if
  (statusflags & STATUS_TAG_TEXT) {
    xmlTag = String(tagName);
    xmlData = String(data);
  } else if
  (statusflags & STATUS_START_TAG) {
    xmlOpen = String(tagName);
  }
}

typedef void (*cbTag)(String value, void* var);
//typedef void (*cbTag)(String value, void* var);

class CfgEntry {
  private:
  String entry;
  cbTag cb;
  uint8_t* i8;
  int8_t* si8;
  uint16_t* i16;
  uint32_t* i32;
  bool* bo;
  float* fl;
  String* st;
  void* var;
  public:
  CfgEntry(String s) {
    entry = s;
    i8 = NULL;
    si8 = NULL;
    i16 = NULL;
    i32 = NULL;
    fl = NULL;
    bo = NULL;
    cb = NULL;
    st = NULL;
  }
  CfgEntry(String s, cbTag c) : CfgEntry(s) {
    cb = c;
  }
  CfgEntry(String s, cbTag c, void* v) : CfgEntry(s) {
    cb = c;
    var = v;
  }
  CfgEntry(String s, uint8_t* c) : CfgEntry(s) {
    i8 = c;
  }
  CfgEntry(String s, int8_t* c) : CfgEntry(s) {
    si8 = c;
  }
  CfgEntry(String s, uint16_t* c) : CfgEntry(s) {
    i16 = c;
  }
  CfgEntry(String s, uint32_t* c) : CfgEntry(s) {
    i32 = c;
  }
  CfgEntry(String s, float* c) : CfgEntry(s) {
    fl = c;
  }
  CfgEntry(String s, bool* c) : CfgEntry(s) {
    bo = c;
  }
  CfgEntry(String s, String* c) : CfgEntry(s) {
    st = c;
  }
  bool match(String s) {
    return s.endsWith(entry);
  }
  bool got(String s) {
    if (i8 != NULL) {
      *i8 = s.toInt();
    } else if (i16 != NULL) {
      *i16 = s.toInt();
    } else if (si8 != NULL) {
      *si8 = s.toInt();
    } else if (i32 != NULL) {
      *i32 = s.toInt();
    } else if (fl != NULL) {
      *fl = s.toFloat();
    } else if (bo != NULL) {
      *bo = s.toInt() == 1;
    } else if (st != NULL) {
      *st = s;
    } else {
      cb(s, var);
    }
    return true;
  }
  bool group(String s) {
    return s.endsWith(entry);
  }
};

bool cfgParse(String cfgName, CfgEntry* cfgEntry, uint16_t count) {
  xml.reset();
  xmlTag = "";
  xmlOpen = "";
  File configFile = SPIFFS.open(cfgName, "r");
  if (configFile) {
   char c;
   while (configFile.read((uint8_t*)&c, 1) == 1) {
    xml.processChar(c);
    if (xmlOpen != "") {
      for (uint16_t i = 0; i < count; i++) {
        cfgEntry[i].group(xmlOpen);
      }
    }
    if (xmlTag != "") {
      for (uint16_t i = 0; i < count; i++) {
        if (cfgEntry[i].match(xmlTag)) {
          cfgEntry[i].got(xmlData);
        }
      }
      xmlOpen = "";
      xmlTag = "";
      xmlData = "";
    }
   }
   configFile.close();
   return true;
  }
  return false;
}
