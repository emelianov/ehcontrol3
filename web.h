////////////////////////////////////////////////////
// EHControl 2016.3
//

#pragma once
#include <ESP8266WebServer.h>

#define DBG_OUTPUT_PORT Serial

extern IPAddress SysLogServer;

ESP8266WebServer server(80);
File fsUploadFile;

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
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

void handleFileDelete(){
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
  if(server.args() == 0)
    return server.send_P(500, "text/plain", PSTR("BAD ARGS"));
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
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

void handleFileList() {
  BUSY
  if(!server.hasArg("dir")) {
	 server.send_P(500, "text/plain", PSTR("BAD ARGS"));
	 return;
  }
  String path = server.arg("dir");
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
  IDLE
}

void handleSet() {
  BUSY
  if(server.hasArg("eco")) {
   ecoMode = server.arg("eco").toInt() == 1;
   server.send(200, "text/json", "OK");
   IDLE
   return;
  }
  server.send_P(500, "text/plain", PSTR("BAD ARGS"));
  IDLE
}

void handleRoot() {
      BUSY
      if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
        IDLE
        return server.requestAuthentication();
      }
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send_P(200, "text/html", PSTR("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"));
      IDLE
}
void handleUpdate() {
      BUSY
      if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
        IDLE
        return server.requestAuthentication();
      }
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
      IDLE
}
void handleProtectedFile() {
  BUSY
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
	IDLE
	return server.requestAuthentication();
  }
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
    char data[400];
    sprintf_P(data, PSTR("<?xml version = \"1.0\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><a0>%d</a0><revision>%s</revision></private></ctrl>"), ESP.getFreeHeap(), WiFi.RSSI(), analogRead(A0), REVISION);
    server.send(200, "text/xml", data);
}
void handleShortState() {
  BUSY
  char buf[400];
  uint8_t i;
  String result("<?xml version = \"1.0\" ?>\n<ctrl><state>\n");
  for (i = 0; i < DEVICE_MAX_COUNT; i++) {
    if (sens[i].gid != 0) {
     sprintf_P(buf, PSTR("<sensor><t>%s</t><gid>%d</gid></sensor>\n"), String(sens[i].tCurrent).c_str(), sens[i].gid);    
     result += buf;
    }
  }
  for (i = 0; i < RELAY_COUNT; i++) {
    if (relays[i].gid != 0) {
      sprintf_P(buf, PSTR("<relay><r>%d</r><gid>%d</gid></relay>\n"), relays[i].on, relays[i].gid);
      result += buf;
    }
  }
  for (i = 0; i < INPUTS_COUNT; i++) {
    if (inputs[i].gid != 0) {
      sprintf_P(buf, PSTR("<input><on>%d</on><gid>%d</gid></input>\n"), inputs[i].on, inputs[i].gid);
      result += buf;
    }
  }
  for (i = 0; i < ANALOG_COUNT; i++) {
    if (analogs[i].gid != 0) {
      sprintf_P(buf, PSTR("<analog><v>%d</v><gid>%d</gid></analog>\n"), analogs[i].value, analogs[i].gid);
      result += buf;
    }
  }
  sprintf_P(buf, PSTR("</state>\n<env>\n<time>%ld</time>\n<eco>%d</eco>\n</env>\n</ctrl>"), time(NULL) % 86400UL, ecoMode);
  result += buf;
  server.send(200, "text/xml", result);
  IDLE
}

void handleState() {
  BUSY
  char buf[400];
  uint8_t i;
  String result("<?xml version = \"1.0\" ?>\n<ctrl><state>\n");
    for (i = 0; i < DEVICE_MAX_COUNT; i++) {
     sprintf_P(buf, PSTR("<sensor><t>%s</t><name>%s</name><id>%02X%02X%02X%02X%02X%02X%02X%02X</id><gid>%d</gid></sensor>\n"),
        String(sens[i].tCurrent).c_str(), sens[i].name.c_str(),
        sens[i].device[0], sens[i].device[1], sens[i].device[2], sens[i].device[3], sens[i].device[4], sens[i].device[5], sens[i].device[6], sens[i].device[7],
        sens[i].gid);    
     result += buf;
    }
    
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

    for (i = 0; i < INPUTS_COUNT; i++) {
      sprintf_P(buf, PSTR("<input><on>%d</on><gid>%d</gid></input>\n"), inputs[i].on, inputs[i].gid);
      result += buf;
    }
  
    for (i = 0; i < ANALOG_COUNT; i++) {
      sprintf_P(buf, PSTR("<analog><v>%d</v><gid>%d</gid></analog>\n"), analogs[i].value, analogs[i].gid);
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
    
    server.send(200, "text/xml", result);
    IDLE
}
void handleReboot() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  BUSY
  WiFiUDP::stopAll();
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      if (relays[i].pin != -1) { _off(i);}
  }
	ESP.restart();
}
#define WEB_CONFIRM_TIME  15000
bool allowFormat = false;
uint32_t disallowFormat() {
  allowFormat = false;
  return 0;
}
void handleFormat() {
  if(!server.authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server.requestAuthentication();
  }
  if (!allowFormat) {
    allowFormat = true;
    taskAddWithDelay(disallowFormat, WEB_CONFIRM_TIME);
    server.send_P(200, "text/html", PSTR("<html><head><script>if (confirm('Procced with FORMAT and DESTROY ALL DATA on internal file system ?')) document.location('/format');</script></head></html>"));
  }
  server.send_P(200, "text/html", PSTR("<html><body>Format</body></html>"));
}

uint32_t initWeb() {
//First callback is called after the request has ended with all parsed arguments
//Second callback handles file uploads at that location
  server.on("/update", HTTP_GET, handleRoot);                       //Update
  server.on("/update", HTTP_POST, handleUpdate, handleUpdateUpload);//Update firmware
  server.on("/list", HTTP_GET, handleFileList);                     //List directory
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.html")) server.send_P(404, "text/plain", PSTR("FileNotFound"));
  });
  server.on("/edit", HTTP_PUT, handleFileCreate);                   //Create file
  server.on("/edit", HTTP_DELETE, handleFileDelete);                //Delete file
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload); //Upload file
  server.onNotFound(handleGenericFile);                             //Load file from FS
  server.on("/state", HTTP_GET, handleState);                     //Get sensors, relays, etc  state
  server.on("/short", HTTP_GET, handleShortState);                 //Get sensors, relays, etc  state
  server.on("/pull", HTTP_GET, handleShortState);                 //Get sensors, relays, etc  state
  server.on("/all", HTTP_GET, handlePrivate);                       //Get internal information
  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/format", HTTP_GET, handleFormat);                   //Format FileSystem
  server.on("/secure.xml", HTTP_GET, handleProtectedFile);
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
