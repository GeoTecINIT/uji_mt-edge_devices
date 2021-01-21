/*
 * Project service-provider
 */

#include "HttpClient.h"
#include "RdWebServer.h"
#include "RestAPIEndpoints.h"


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
byte CurrentRegionID = 0;

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

int fogApiGet(String path, JSONValue* jsonValue, String hostname, int port) {
  HttpClient http;
  http_header_t headers[] = {
    {"Content-Type", "application/json"},
    {"Accept", "*/*"},
    {"User-agent", "Particle HttpClient"},
    {NULL, NULL}
  };
  http_request_t request;
  http_response_t response;

  request.hostname = hostname;
  request.port = port;
  request.path = path;

  http.get(request, response, headers);

  *jsonValue = JSONValue::parseCopy(response.body);
  return response.status;
}

int fogApiGet(String path, JSONValue* jsonValue) {
  return fogApiGet(path, jsonValue, FogAPIHostName, FogAPIPort);
}

bool fogApiGetData(String path, JSONValue* data, String hostname, int port) {
  JSONValue response;
  fogApiGet(path, &response, hostname, port);
  if (!response.isValid()) {
    ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_FAIL", path);
    return false;
  }

  bool ok;
  if (!JSONGetBool(&response, "ok", &ok) || !ok || !JSONGetValue(&response, "data", data)) {
    ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_FAIL", path);
    return false;
  }

  return true;
}

bool fogApiGetData(String path, JSONValue* data) {
  return fogApiGetData(path, data, FogAPIHostName, FogAPIPort);
}

bool testHostname(String hostname) {
  JSONValue data;
  return fogApiGetData("/alive", &data, hostname, FogAPIPort);
}

void testFogUpdateState() {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_TEST");
  if (testHostname(FogAPIHostName)) {
    ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_OK");
    setState(STATE_IDLE);
  } else {
    setState(STATE_LOST);
  }
}

void updateHostname(String newHostname) {
  if (newHostname.compareTo(FogAPIHostName) != 0) {
    JSONValue data;
    if (testHostname(newHostname)) {
      FogAPIHostName = newHostname;
      ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("FOG_UPDATED", FogAPIHostName);
    }
  }
}

void updateRegionID(byte newID) {
  if (CurrentRegionID != newID) {
    CurrentRegionID = newID;
    ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("REGION_UPDATED", String(CurrentRegionID));
  }
}

void checkRegion() {
  JSONValue data;
  if (fogApiGetData("/region/query/" + bytesToHex(CurrentLocation, 8), &data)) {
    int regionID;
    String ipv4;
    if (JSONGetInt(&data, "id", &regionID) && JSONGetString(&data, "ipv4", &ipv4)) {
      updateRegionID(regionID);
      updateHostname(ipv4);
    }
  }
}

void setLocation(byte* location) {
  for (int i = 0; i < 8; i++) {
    CurrentLocation[i] = location[i];
  }
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("LOCATION_UPDATED", bytesToHex(CurrentLocation, 8));
  checkRegion();
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
  testFogUpdateState();
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

  setLocation(hexToBytes(cellHex));
}

void setup() {
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("DEVICE_START");
  setupAPI();
  testFogUpdateState();
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
    testFogUpdateState();
  }
}

void loopIdle() {
  webServer->service();
}
