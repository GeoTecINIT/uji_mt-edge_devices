/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Ponlawat/source/uji/edge-devices/service-provider/src/service-provider.ino"
/*
 * Project service-provider
 */

#include "HttpClient.h"
#include "RdWebServer.h"
#include "RestAPIEndpoints.h"


void cloneConstChar(const char* src, char* dest);
void setState(int newState);
String makeUrl(String path);
int getHexStrByteLength(String hexStr);
byte* hexToBytes(String hexStr);
String bytesToHex(byte* bytes, int length);
bool JSONGetBool(JSONValue* value, String propertyName, bool* out);
bool JSONGetDouble(JSONValue* value, String propertyName, double* out);
bool JSONGetInt(JSONValue* value, String propertyName, int* out);
bool JSONGetString(JSONValue* value, String propertyName, String* out);
bool JSONGetValue(JSONValue* value, String propertyName, JSONValue* out);
int fogApiGet(String path, JSONValue* jsonValue);
void testFogAPI();
void setLocation(byte* location);
void apiReturnSuccess(String& response, char* data);
void apiReturnFail(String& response, String msg, char* data);
void apiReturnInvalidInput(String& response);
bool apiRequireMethod(RestAPIEndpointMsg& request, String& response, int method);
void apiAlive(RestAPIEndpointMsg& request, String& response);
void apiSetFogIp(RestAPIEndpointMsg& request, String& response);
void apiGetLocation(RestAPIEndpointMsg& request, String& response);
void apiSetLocation(RestAPIEndpointMsg& request, String& response);
void setup();
void loop();
void setupAPI();
void loopStartup();
void loopLost();
void loopIdle();
#line 10 "c:/Users/Ponlawat/source/uji/edge-devices/service-provider/src/service-provider.ino"
#define byte            uint8_t

#define STATE_STARTUP   1
#define STATE_LOST      2
#define STATE_IDLE      3

#define API_METHOD_GET  1
#define API_METHOD_POST 2

#define VERBOSITY_NO      0
#define VERBOSITY_GENERAL 1
#define VERBOSITY_DEBUG   2

#define VERBOSITY         VERBOSITY_GENERAL | VERBOSITY_DEBUG

int CurrentState = STATE_STARTUP;

String FogAPIHostName = "192.168.1.236";
int FogAPIPort = 80;

RestAPIEndpoints apiEndPoints;
RdWebServer* webServer;

byte CurrentLocation[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void cloneConstChar(const char* src, char* dest) {
  int length = strlen(src);
  dest = new char[length];
  for (int i = 0; i < length; i++) {
    dest[i] = src[i];
  }
}

void setState(int newState) {
  if (newState != CurrentState) {
    CurrentState = newState;
    (VERBOSITY && VERBOSITY_GENERAL) && Particle.publish("STATE_CHANGED", String(CurrentState));
  }
}

String makeUrl(String path) {
  return FogAPIHostName + path;
}

int getHexStrByteLength(String hexStr) {
  return (hexStr.length() % 2 == 0) ? ((hexStr.length() - 2) / 2) : ((hexStr.length() - 2) / 2) + 1;
}

byte* hexToBytes(String hexStr) {
  hexStr = hexStr.substring(2);
  if (hexStr.length() % 2 == 1) {
    hexStr = "0" + hexStr;
  }

  int length = hexStr.length();
  byte* bytes = new byte[length / 2];
  for (int i = 0; i < length; i += 2) {
    bytes[i / 2] = strtol(hexStr.substring(i, i + 2).c_str(), NULL, 16);
  }
  return bytes;
}

String bytesToHex(byte* bytes, int length) {
  static const char* hex = "0123456789abcdef";
  String result = "0x";
  for (int i = 0; i < length; i++) {
    result += hex[bytes[i] >> 4];
    result += hex[bytes[i] & 0x0f];
  }
  return result;
}

bool JSONGetBool(JSONValue* value, String propertyName, bool* out) {
  JSONObjectIterator iterator(*value);
  while (iterator.next()) {
    if (iterator.name() == propertyName) {
      *out = iterator.value().toBool();
      return true;
    }
  }
  return false;
}

bool JSONGetDouble(JSONValue* value, String propertyName, double* out) {
  JSONObjectIterator iterator(*value);
  while (iterator.next()) {
    if (iterator.name() == propertyName) {
      *out = iterator.value().toDouble();
      return true;
    }
  }
  return false;
}

bool JSONGetInt(JSONValue* value, String propertyName, int* out) {
  JSONObjectIterator iterator(*value);
  while (iterator.next()) {
    if (iterator.name() == propertyName) {
      *out = iterator.value().toInt();
      return true;
    }
  }
  return false;
}

bool JSONGetString(JSONValue* value, String propertyName, String* out) {
  JSONObjectIterator iterator(*value);
  while (iterator.next()) {
    if (iterator.name() == propertyName) {
      *out = String(iterator.value().toString());
      return true;
    }
  }
  return false;
}

bool JSONGetValue(JSONValue* value, String propertyName, JSONValue* out) {
  JSONObjectIterator iterator(*value);
  while (iterator.next()) {
    if (iterator.name() == propertyName) {
      *out = iterator.value();
      return true;
    }
  }
  return false;
}

int fogApiGet(String path, JSONValue* jsonValue) {
  HttpClient http;
  http_header_t headers[] = {
    {"Content-Type", "application/json"},
    {"Accept", "*/*"},
    {"User-agent", "Particle HttpClient"},
    {NULL, NULL}
  };
  http_request_t request;
  http_response_t response;

  request.hostname = FogAPIHostName;
  request.port = FogAPIPort;
  request.path = path;

  http.get(request, response, headers);

  *jsonValue = JSONValue::parseCopy(response.body);
  return response.status;
}

void testFogAPI() {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_TEST");
  JSONValue responseJson;
  int status = fogApiGet("/alive", &responseJson);
  if (responseJson.isValid()) {
    bool ok;
    JSONGetBool(&responseJson, "ok", &ok);
    if (ok) {
      ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_OK");
      setState(STATE_IDLE);
      return;
    } else {
      String message;
      JSONGetString(&responseJson, "msg", &message);
      ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_FAIL", message);
      setState(STATE_LOST);
    }
  } else {
    ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_FAIL", String(status));
    setState(STATE_LOST);
  }
}

void setLocation(byte* location) {
  if (sizeof(location) != 8) {
    ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("LOCATION_SET_REJECTED", String(sizeof(location)));
    return;
  }
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("LOCATION_SET");
  for (int i = 0; i < 8; i++) {
    CurrentLocation[i] = location[i];
  }
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("LOCATION_UPDATED", bytesToHex(CurrentLocation, 8));
}

void apiReturnSuccess(String& response, char* data) {
  response = data == NULL ? "{\"ok\":true,\"msg\":null,\"data\":null}" : ("{\"ok\":true,\"msg\":null,\"data\":" + String(data) + "}");
}

void apiReturnFail(String& response, String msg, char* data) {
  response = "{\"ok\":false,\"msg\":";
  if (msg == NULL) {
    response += "null";
  } else {
    response += "\"" + msg + "\"";
  }
  response += ",\"data\":";
  response += (data == NULL ? "null" : data);
  response += "}";
}

void apiReturnInvalidInput(String& response) {
  apiReturnFail(response, "Invalid Input", NULL);
}

bool apiRequireMethod(RestAPIEndpointMsg& request, String& response, int method) {
  if (request._method != method) {
    apiReturnFail(response, "Invalid Method", NULL);
    return false;
  }
  return true;
}

void apiAlive(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_ALIVE");
  apiReturnSuccess(response, (char*)String(CurrentState).c_str());
}

void apiSetFogIp(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SET_IP", (char*)request._pMsgContent);
  if (!apiRequireMethod(request, response, API_METHOD_POST)) {
    return;
  }

  JSONValue value = JSONValue::parseCopy((char*)request._pMsgContent);
  if (!JSONGetString(&value, "hostname", &FogAPIHostName)) {
    apiReturnInvalidInput(response);
    return;
  }
  if (!JSONGetInt(&value, "port", &FogAPIPort)) {
    apiReturnInvalidInput(response);
    return;
  }
  apiReturnSuccess(response, NULL);
  testFogAPI();
}

void apiGetLocation(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_GET_LOCATION", (char*)request._pMsgContent);
  if (!apiRequireMethod(request, response, API_METHOD_GET)) {
    return;
  }

  String data = "\"" + bytesToHex(CurrentLocation, 8) + "\"";
  apiReturnSuccess(response, (char*)data.c_str());
}

void apiSetLocation(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SET_LOCATION", (char*)request._pMsgContent);
  if (!apiRequireMethod(request, response, API_METHOD_POST)) {
    return;
  }

  JSONValue value = JSONValue::parseCopy((char*)request._pMsgContent);
  String cellHex;
  if (!JSONGetString(&value, "location", &cellHex)) {
    apiReturnInvalidInput(response);
    return;
  }
  if (cellHex.length() != 18) {
    apiReturnInvalidInput(response);
    return;
  }
  apiReturnSuccess(response, NULL);

  byte* cells;
  cells = hexToBytes(cellHex);
  setLocation(cells);
}

void setup() {
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("DEVICE_START");
  setupAPI();
  testFogAPI();
}

void loop() {
  switch (CurrentState) {
    case STATE_STARTUP:
      loopStartup(); return;
    case STATE_LOST:
      loopLost(); return;
    case STATE_IDLE:
      loopIdle(); return;
  }
}

void setupAPI() {
  IPAddress ip = WiFi.localIP();
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("LOCAL_IP", ip.toString());
  Particle.variable("ipAddress", ip.toString());

  apiEndPoints.addEndpoint("alive", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiAlive, "", "");
  apiEndPoints.addEndpoint("set-fog", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiSetFogIp, "", "");
  apiEndPoints.addEndpoint("get-location", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiGetLocation, "", "");
  apiEndPoints.addEndpoint("set-location", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiSetLocation, "", "");

  webServer = new RdWebServer();
  webServer->addRestAPIEndpoints(&apiEndPoints);
  webServer->start(80);
}

void loopStartup() {
}

void loopLost() {
  webServer->service();

  if (Time.second() == 0) {
    testFogAPI();
  }
}

void loopIdle() {
  webServer->service();
}
