/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Ponlawat/source/uji/edge-devices/service-provider/src/service-provider.ino"
/*
 * Project service-provider
 */

#include "HttpClient.h"

void cloneConstChar(const char* src, char* dest);
void setState(int newState);
String makeUrl(String path);
bool JSONGetBool(JSONValue* value, String propertyName, bool* out);
bool JSONGetDouble(JSONValue* value, String propertyName, double* out);
bool JSONGetInt(JSONValue* value, String propertyName, int* out);
bool JSONGetString(JSONValue* value, String propertyName, String* out);
bool JSONGetValue(JSONValue* value, String propertyName, JSONValue* out);
int fogApiGet(String path, JSONValue* jsonValue);
int setFogHostname(String apiBase);
int setFogPort(String port);
void testFogAPI();
void setup();
void loop();
void loopStartup();
void loopLost();
void loopIdle();
#line 7 "c:/Users/Ponlawat/source/uji/edge-devices/service-provider/src/service-provider.ino"
#define byte            uint8_t

#define STATE_STARTUP   1
#define STATE_LOST      2
#define STATE_IDLE      3

#define LOST_TRY_CONNECT_AGAIN_COUNT  10

int CURRENT_STATE = STATE_STARTUP;

String FogAPIHostName = "192.168.1.236";
int FogAPIPort = 80;

void cloneConstChar(const char* src, char* dest) {
  int length = strlen(src);
  dest = new char[length];
  for (int i = 0; i < length; i++) {
    dest[i] = src[i];
  }
}

void setState(int newState) {
  if (newState != CURRENT_STATE) {
    CURRENT_STATE = newState;
    Particle.publish("STATE_CHANGED", String(CURRENT_STATE));
  }
}

String makeUrl(String path) {
  return FogAPIHostName + path;
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

int setFogHostname(String apiBase) {
  FogAPIHostName = apiBase;
  testFogAPI();
  return 0;
}

int setFogPort(String port) {
  FogAPIPort = atoi(port.c_str());
  return 0;
}

void testFogAPI() {
  Particle.publish("API_TEST");
  JSONValue responseJson;
  int status = fogApiGet("/alive", &responseJson);
  if (responseJson.isValid()) {
    bool ok;
    JSONGetBool(&responseJson, "ok", &ok);
    if (ok) {
      setState(STATE_IDLE);
      return;
    } else {
      String message;
      JSONGetString(&responseJson, "msg", &message);
      Particle.publish("API_FAIL", message);
      setState(STATE_LOST);
    }
  } else {
    Particle.publish("API_FAIL", String(status));
    setState(STATE_LOST);
  }
}

void setup() {
  Particle.publish("DEVICE_START");
  Particle.function("setFogHostName", setFogHostname);
  Particle.function("setFogPort", setFogPort);
  testFogAPI();
}

void loop() {
  switch (CURRENT_STATE) {
    case STATE_STARTUP:
      loopStartup(); return;
    case STATE_LOST:
      loopLost(); return;
    case STATE_IDLE:
      loopIdle(); return;
  }
}

void loopStartup() {
  Particle.publish("STATE_STARTUP");
  delay(5);
}

int loopLostCount = 0;
void loopLost() {
  Particle.publish("STATE_LOST", "Communication lost with fog, updated ip needed to be assigned");
  delay(10);
  if (loopLostCount++ > LOST_TRY_CONNECT_AGAIN_COUNT) {
    loopLostCount = 0;
    testFogAPI();
  }
}

void loopIdle() {
  Particle.publish("STATE_IDLE");
  delay(10);
}
