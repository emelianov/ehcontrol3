//////////////////////////////////////////////////////
// EHControl3 2016.4 (c)2016, a.m.emelianov@gmail.com
// HTTP-server

#pragma once
#include <ESP8266WebServer.h>

extern IPAddress SysLogServer;

ESP8266WebServer server(80);
File fsUploadFile;
String getContentType(String filename){
  if(server.hasArg(F("download"))) return "application/octet-stream";
  else if(filename.endsWith(F(".htm"))) return "text/html";
  else if(filename.endsWith(F(".html"))) return "text/html";
  else if(filename.endsWith(F(".css"))) return "text/css";
  else if(filename.endsWith(F(".js"))) return "application/javascript";
  else if(filename.endsWith(F(".png"))) return "image/png";
  else if(filename.endsWith(F(".gif"))) return "image/gif";
  else if(filename.endsWith(F(".jpg"))) return "image/jpeg";
  else if(filename.endsWith(F(".ico"))) return "image/x-icon";
  else if(filename.endsWith(F(".xml"))) return "text/xml";
  else if(filename.endsWith(F(".pdf"))) return "application/x-pdf";
  else if(filename.endsWith(F(".zip"))) return "application/x-zip";
  else if(filename.endsWith(F(".gz"))) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith(F("/"))) path += F("index.html");
  String contentType = getContentType(path);
  String pathWithGz = path + F(".gz");
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += F(".gz");
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-store, must-revalidate");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFile() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Refresh", "5; url=/config");
  server.send_P(200, "text/plain", PSTR("OK"));  
}
void handleFileUpload(){
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  if(server.uri() != "/edit") return;
  BUSY
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
  }
  IDLE
}
/*
void handleFileDelete(){
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  if(server.args() == 0) return server.send_P(500, "text/plain", PSTR("BAD ARGS"));
  String path = server.arg(0);
  if(path == "/")
    return server.send_P(500, "text/plain", PSTR("BAD PATH"));
  if(!SPIFFS.exists(path))
    return server.send_P(404, "text/plain", PSTR("FileNotFound"));
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  if(server.args() == 0)
    return server.send_P(500, "text/plain", PSTR("BAD ARGS"));
  String path = server.arg(0);
  if(path == "/")
    return server.send_P(500, "text/plain", PSTR("BAD PATH"));
  if(SPIFFS.exists(path))
    return server.send_P(500, "text/plain", PSTR("FILE EXISTS"));
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send_P(500, "text/plain", PSTR("CREATE FAILED"));
  server.send(200, "text/plain", "");
  path = String();
}
*/

void handleSet() {
  BUSY
/*  if(server.hasArg("gid")) {
   int16_t gid = server.arg("gid").toInt() == 1;
   for (uint8_t i = 0 i < INPUT_COUNT; i++) {
    if (inputs[i].gid == server.arg("gid").toInt() == 1;
   }
   server.sendHeader("Connection", "close");
   //server.sendHeader("Cache-Control", "no-store, must-revalidate");
   server.sendHeader("Refresh", "5; url=/");
   server.send_P(200, "text/plain", PSTR("OK"));
   IDLE
   return;
  }
*/
  server.send_P(500, "text/plain", PSTR("BAD ARGS"));
  IDLE
}
//Haeter
void handleHeater() {
  BUSY
  bool isOk = false;
  bool isT = false;
  float t = 20;
  uint8_t idx = 0;
   if
  (server.hasArg("eco")) {
    bool ecoNew = server.arg("eco").toInt() == 1;
    if (ecoNew != ecoMode) {
      ecoMode = ecoNew;
      isOk = true;
    }
   } else if
  (server.hasArg("z1")) {
    t = server.arg("z1").toFloat();
    idx = ZONE1;
    isT = true;
    isOk = true;
   } else if
  (server.hasArg("z2")) {
    t = server.arg("z2").toFloat();
    idx = ZONE2;
    isT = true;
    isOk = true;
   } else if
  (server.hasArg("reload")) {
    isT = false;
    isOk = readRelays();
   }
   if (isT) {
    if (t > T_LIMIT_MIN && t < T_LIMIT_MAX) {
      if (IS_ECO) {
        relays[idx].t[ECO]    = t;
      } else if (relays[idx].isT2) {
        relays[idx].t[NIGHT]  = t;
      } else {
        relays[idx].t[DAY]    = t;
      }
    } else {
      isOk = false;
    }
   }
   if (isOk) {
    switchSchedule();
    taskDel(saveRelaysSettings);
    taskAddWithDelay(saveRelaysSettings, AUTOSAVE_DELAY);    
    server.sendHeader("Connection", "close");
    //server.sendHeader("Cache-Control", "no-store, must-revalidate");
    server.sendHeader("Refresh", "5; url=/");
    server.send_P(200, "text/plain", PSTR("OK"));
   } else {
    server.send_P(500, "text/plain", PSTR("BAD ARGS"));
   }
   IDLE
   return;
}

void handleUpdate() {
      if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
        return server.requestAuthentication();
      }
      BUSY
      server.sendHeader("Connection", "close");
      server.sendHeader("Refresh", "10; url=/");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
}
void handleProtectedFile() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
	  return server.requestAuthentication();
  }
  BUSY
  if(!handleFileRead(server.uri()))
    server.send_P(404, "text/plain", PSTR("FileNotFound"));
  IDLE
}
void handleUpdateUpload() {
  BUSY
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        WiFiUDP::stopAll();
        for (uint8_t i = 0; i < RELAY_COUNT; i++) {
          if (relays[i].pin != -1) { _off(i);}
        }
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
  IDLE
}
void handleGenericFile() {
  BUSY
  if(!handleFileRead(server.uri()))
    server.send_P(404, "text/plain", PSTR("FileNotFound"));
  IDLE
}
void handlePrivate() {
  BUSY
  char data[400];
  sprintf_P(data, PSTR("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><revision>%s</revision>\
<use>\
<sensors>%d</sensors>\
<lcd>%d</lcd>\
<heater>%d</heater>\
<partners>%d</partners>\
<ap>%d</ap>\
<syslog>%d</syslog>\
</use>\
</private></ctrl>"), ESP.getFreeHeap(), WiFi.RSSI(), REVISION, use.sensors, use.lcd, use.heater, use.partners, use.ap, use.syslog);
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.send(200, "text/xml", data);
  IDLE
}
void handleShortState() {
  BUSY
  char buf[400];
  uint8_t i;
  DeviceAddress zerro;
  memset(zerro, 0, sizeof(DeviceAddress));
  String result = F("<?xml version = \"1.0\"  encoding=\"UTF-8\" ?>\n<ctrl><state>\n");
  for (i = 0; i < DEVICE_MAX_COUNT; i++) {
    if (sens[i].gid != 0 && memcmp(sens[i].device, zerro, sizeof(DeviceAddress)) != 0) {
     sprintf_P(buf, PSTR("<sensor><t>%s</t><gid>%d</gid></sensor>\n"), String(sens[i].tCurrent).c_str(), sens[i].gid);    
     result += buf;
    }
  }
  for (i = 0; i < RELAY_COUNT; i++) {
    if (relays[i].gid != 0 && relays[i].pin != -1) {
      sprintf_P(buf, PSTR("<relay><r>%d</r><gid>%d</gid></relay>\n"), relays[i].on, relays[i].gid);
      result += buf;
    }
  }
  for (i = 0; i < INPUTS_COUNT; i++) {
    if (inputs[i].gid != 0 && inputs[i].pin != -1) {
      sprintf_P(buf, PSTR("<input><on>%d</on><gid>%d</gid></input>\n"), inputs[i].on, inputs[i].gid);
      result += buf;
    }
  }
  for (i = 0; i < ANALOG_COUNT; i++) {
    if (analogs[i].gid != 0 && analogs[i].pin != -1) {
      sprintf_P(buf, PSTR("<analog><v>%d</v><gid>%d</gid></analog>\n"), analogs[i].value, analogs[i].gid);
      result += buf;
    }
  }
  sprintf_P(buf, PSTR("</state>\n<env>\n<time>%ld</time>\n<eco>%d</eco>\n</env>\n</ctrl>"), time(NULL) % 86400UL, ecoMode);
  result += buf;
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.send(200, "text/xml", result);
  IDLE
}

void handleState() {
  BUSY
  char buf[400];
  uint8_t i;
  String result = F("<?xml version = \"1.0\" encoding=\"UTF-8\" ?>\n<ctrl><state>\n");
  if (use.sensors || use.partners) {
    for (i = 0; i < DEVICE_MAX_COUNT; i++) {
     sprintf_P(buf, PSTR("<sensor><t>%s</t><name>%s</name><id>%02X%02X%02X%02X%02X%02X%02X%02X</id><gid>%d</gid><age>%d</age></sensor>\n"),
        String(sens[i].tCurrent).c_str(), sens[i].name.c_str(),
        sens[i].device[0], sens[i].device[1], sens[i].device[2], sens[i].device[3], sens[i].device[4], sens[i].device[5], sens[i].device[6], sens[i].device[7],
        sens[i].gid, sens[i].age);    
     result += buf;
    }
  }
  if (use.heater) {
    for (i = 0; i < RELAY_COUNT; i++) {
		sprintf_P(buf, PSTR("\
<relay><r>%d</r><rname>%s</rname>\
<rhi>%s</rhi><rlow>%s</rlow>\
<td>%s</td><tn>%s</tn><te>%s</te><ts>%s</ts>\
<ront>%d</ront><rofft>%d</rofft><rnowt>%d</rnowt></relay>\n"),
        relays[i].on, relays[i].name.c_str(),
        String(relays[i].tHi).c_str(), String(relays[i].tLow).c_str(),
        String(relays[i].t[DAY]).c_str(), String(relays[i].t[NIGHT]).c_str(), String(relays[i].t[ECO]).c_str(), String(relays[i].t[OTHER]).c_str(),
        relays[i].onT2, relays[i].offT2, relays[i].isT2);
		result += buf;
    }
  }
    for (i = 0; i < INPUTS_COUNT; i++) {
      sprintf_P(buf, PSTR("<input><on>%d</on><gid>%d</gid><age>%d</age></input>\n"), inputs[i].on, inputs[i].gid, inputs[i].age);
      result += buf;
    }
  
    for (i = 0; i < ANALOG_COUNT; i++) {
      sprintf_P(buf, PSTR("<analog><v>%d</v><gid>%d</gid><age>%d</age></analog>\n"), analogs[i].value, analogs[i].gid,analogs[i].age);
      result += buf;
    }

  IPAddress ip = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  IPAddress gw = WiFi.gatewayIP();    
  sprintf_P(buf, PSTR("\
</state>\n<config>\n\
<ip>%d.%d.%d.%d</ip>\n\
<netmask>%d.%d.%d.%d</netmask>\n\
<gw>%d.%d.%d.%d</gw>\n\
<ns>%d.%d.%d.%d</ns>\n\
<log>%d.%d.%d.%d</log>\n\
<ntp>%s</ntp>\n\
<ntp>%s</ntp>\n\
<ntp>%s</ntp>\n\
<tz>%d</tz>\n\
</config>\n\
<env>\n\
<time>%ld</time>\n\
<uptime>%ld</uptime>\n\
<sid>.</sid>\n\
<eco>%d</eco>\n\
<name>%s</name>\n\
</env>\n\
</ctrl>"),
  ip[0],ip[1],ip[2],ip[3],
  mask[0],mask[1],mask[2],mask[3],
  gw[0],gw[1],gw[2],gw[3],
  gw[0],gw[1],gw[2],gw[3],
  sysLogServer[0],sysLogServer[1],sysLogServer[2],sysLogServer[3],
  timeServer[0].c_str(),timeServer[1].c_str(),timeServer[2].c_str(),
  timeZone,
  time(NULL) % 86400UL,
  (uint32_t)millis()/1000, ecoMode, name.c_str());
    result += buf;
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-store, must-revalidate");    
    server.send(200, "text/xml", result);
    IDLE
}

#define WEB_CONFIRM_TIME  15000
bool allowFormat = false;
bool allowReboot = false;
bool allowDelete = false;
uint32_t disallowFormatRebootDelete() {
  allowFormat = false;
  allowReboot = false;
  allowDelete = false;
  return 0;
}

void handleReboot() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  BUSY
  if (!allowReboot) {
    allowReboot = true;
    taskAddWithDelay(disallowFormatRebootDelete, WEB_CONFIRM_TIME);
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-store, must-revalidate");
    server.sendHeader("Refresh", "15; url=/");
    server.send_P(200, PSTR("text/html"), PSTR("<html><head><script>window.location=(confirm('Procced with system reboot?'))?'/reboot':'/config';</script></head></html>"));
    IDLE
    return;
  }
  WiFiUDP::stopAll();
  if (use.heater) {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      if (relays[i].pin != -1) { _off(i);}
    }
  }
  server.sendHeader("Refresh", "15; url=/");
  server.send_P(200, PSTR("text/plain"), PSTR("OK"));
	ESP.restart();
}
void handleFormat() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  BUSY
  if (!allowFormat) {
    allowFormat = true;
    taskAddWithDelay(disallowFormatRebootDelete, WEB_CONFIRM_TIME);
    server.send_P(200, PSTR("text/html"), PSTR("<html><head><script>window.location=(confirm('Procced with FORMAT and DESTROY ALL DATA on internal file system ?'))?'/format':'/config';</script></head></html>"));
    IDLE
    return;
  }
  allowFormat = false;
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader("Refresh", "15; url=/config");
  server.send_P(200, PSTR("text/plain"), PSTR("OK"));
  ALERT
  SPIFFS.format();
  NOALERT
  IDLE
}
void handleDelete() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  BUSY
  if (!allowDelete) {
    allowDelete = true;
    taskAddWithDelay(disallowFormatRebootDelete, WEB_CONFIRM_TIME);
    server.send_P(200, PSTR("text/html"), PSTR("<html><head><script>window.location=(confirm('Procced with delete ?'))?'/delete'+window.location.search:'/config';</script></head></html>"));
    IDLE
    return;
  }
  allowDelete = false;
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader("Refresh", "15; url=/config");
  String path;
  if(server.args() != 0) {
    path = server.arg(0);
    if(path != "/" && SPIFFS.exists(path)) {
      SPIFFS.remove(path);
      server.send_P(200, PSTR("text/plain"), PSTR("OK"));
      IDLE
      return;
    }
  }
  server.send(200, PSTR("text/plain"), PSTR("ERROR"));
  IDLE
}

void handleConfig() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  BUSY
  disallowFormatRebootDelete();
  String path = server.hasArg("dir")?server.arg("dir"):"/";
  Dir dir = SPIFFS.openDir(path);
  String output = F("<html><head><meta charset='utf-8'><title>ehcontrol3</title>\
<script>\
var filename = '/global.xml';\
function getFileFromServer(url, doneCallback) {\
    var xhr;\
    xhr = new XMLHttpRequest();\
    xhr.onreadystatechange = handleStateChange;\
    xhr.open('GET', url, true);\
    xhr.send();\
    function handleStateChange() {\
        if (xhr.readyState === 4) {\
            doneCallback(xhr.status == 200 ? xhr.responseText : null);\
        }\
    }\
}\
function sendFileToServer(url, fname, dataCallback) {\
  var boundaryString = '---------------------------132611019532525';\
      var boundary = '--' + boundaryString;\
      var requestbody = \
              boundary + '\\r\\n'\
            + 'Content-Disposition: form-data; name=\"update\"; filename=\"' + fname + '\"\\r\\n'\
            + 'Content-Type: text/xml\\r\\n\\r\\n'\
            + dataCallback() + '\\r\\n'\
      + boundary + '--\\r\\n';\
  var xhr = new XMLHttpRequest();\
  xhr.open('POST', url, true);\
        xhr.setRequestHeader('Content-type', 'multipart/form-data; boundary=' + boundaryString);\
      xhr.setRequestHeader('Connection', 'close');\
      xhr.setRequestHeader('Content-length', requestbody.length);\
  xhr.onreadystatechange = handleStateChange;\
  xhr.send(requestbody);\
  function handleStateChange() {\
    if (this.readyState != 4) return;\
      alert( this.responseText );\
  }\
}\
</script>\
</head><body><h1>Configuration</h1><br>\
<table width=100% border=0px><tr><td valign='top'>\
<form method='POST' action='/edit' enctype='multipart/form-data'>\
 Upload file to local filesystem:<br>\
 <input type='file' name='update'>\
 <input type='submit' value='Upload file'>\
</form>\
FileSystem contents:<br><table border=0px>");
  while(dir.next()){
    File entry = dir.openFile("r");
    String filename = String(entry.name());
    output += F("<tr><td><a href='");
    output += filename;
    output += F("'>");
    output += filename.substring(1);
    output += F("</a></td><td><a href='' onClick='filename=\"");
    output += filename;
    output += F("\";getFileFromServer(filename, function(text) { if (text != null) document.getElementById(\"text\").value = text; });return false;'>Edit</a><br>");
    output += F("</td><td><a href='' onClick='window.location=\"/delete?file=");
    output += filename;
    output += F("\";return false;'>Delete</a></td></tr>");
    entry.close();
  }
  output += F("</table><br><form method='POST' action='/update' enctype='multipart/form-data'>\
 Update firmware:<br>\
 <input type='file' name='update'>\
 <input type='submit' value='Update firmware'>\
 <br><br>\
 <input type='button' value='Format FileSystem' onClick='window.location=\"/format\";'>\
 &nbsp;\
 <input type='button' value='Reboot device' onClick='window.location=\"/reboot\";'>\
</form>\
</td><td>\
<form method='POST' action='/edit' enctype='multipart/form-data' onSubmit='return false;'>\
 <textarea id='text' cols=80 rows=40></textarea>\
 <br>\
 <input type='button' value='Save file' onClick='sendFileToServer(\"/edit\", filename, function() {return document.getElementById(\"text\").value;});'>\
</form>\
</td></tr></body></html>");
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html; charset=utf-8", output);
  IDLE
}
void handleWeather() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader(F("Access-Control-Allow-Origin"), "*");
  String output = F("{ \"temperature\": ");
  output += String(sens[1].tCurrent);
  output += F(", \"humidity\": 38 }");
  server.send(200, "text/html; charset=utf-8", output);
}
uint32_t initWeb() {
//First callback is called after the request has ended with all parsed arguments
//Second callback handles file uploads at that location
  server.on("/update", HTTP_POST, handleUpdate, handleUpdateUpload);//Update firmware
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.html")) server.send_P(404, "text/plain", PSTR("FileNotFound"));
  });
//  server.on("/edit", HTTP_PUT, handleFileCreate);                 //Create file
//  server.on("/edit", HTTP_DELETE, handleFileDelete);              //Delete file
  server.on("/edit", HTTP_POST, handleFile, handleFileUpload);    //Upload file
  server.onNotFound(handleGenericFile);                           //Load file from FS
  server.on("/state", HTTP_GET, handleState);                     //Get sensors, relays, etc  state
  if (use.partners) {
    server.on("/short", HTTP_GET, handleShortState);              //Get sensors, relays, etc  state
    server.on("/pull", HTTP_GET, handleShortState);               //Get sensors, relays, etc  state
  }
  server.on("/all", HTTP_GET, handlePrivate);                     //Get internal information
  server.on("/reboot", HTTP_GET, handleReboot);                   //Reboot device
  server.on("/set", HTTP_GET, handleSet);                         //Set internal variable
  if (use.heater) {
    server.on("/heater", HTTP_GET, handleHeater);                         //Set heater settings
  }
  server.on("/format", HTTP_GET, handleFormat);                   //Format FileSystem
  server.on("/delete", HTTP_GET, handleDelete);                   //Delete File
  server.on("/config", HTTP_GET, handleConfig);                   //System configuration
  server.on("/secure.xml", HTTP_GET, handleProtectedFile);        //Load restricted secure.xml from FS
  server.on("/weather", HTTP_GET, handleWeather);                 //For testing
  return 0;
}
uint32_t handleWeb() {
  server.handleClient();
  return 100;
}
uint32_t startWeb() {
  server.begin();
  taskAdd(handleWeb);
  return 0;
}
