/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Ponlawat/source/uji/edge-devices/service-consumer/src/service-consumer.ino"
/*
 * Project service-consumer
 */

#include "HttpClient.h"
#include "Grove_ChainableLED.h"

bool JSONGetBool(JSONValue* value, String propertyName, bool* out);
bool JSONGetDouble(JSONValue* value, String propertyName, double* out);
bool JSONGetInt(JSONValue* value, String propertyName, int* out);
bool JSONGetString(JSONValue* value, String propertyName, String* out);
bool JSONGetValue(JSONValue* value, String propertyName, JSONValue* out);
int httpSend(String hostname, int port, String path, byte method, String* body,
  JSONValue* responseJSON, String* rawResponse);
bool sendRequestAndReturnData(String hostname, int port, String path, byte method, String* body, JSONValue* data);
void setup();
void loop();
#line 8 "c:/Users/Ponlawat/source/uji/edge-devices/service-consumer/src/service-consumer.ino"
#define METHOD_GET  1
#define METHOD_POST 2

#define LED_PIN1  D2
#define LED_PIN2  D3

String APIBaseHostname = "http://192.168.1.236";
int APIBasePort = 80;

ChainableLED Led(LED_PIN1, LED_PIN2, 1);

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

int httpSend(String hostname, int port, String path, byte method, String* body,
  JSONValue* responseJSON, String* rawResponse) {
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

  switch (method) {
    case METHOD_GET:
      http.get(request, response, headers); break;
    case METHOD_POST:
      request.body = *body;
      http.post(request, response, headers); break;
    default:
      return -1;
  }

  *responseJSON = JSONValue::parseCopy(response.body);
  *rawResponse = response.body;
  return response.status;
}

bool sendRequestAndReturnData(String hostname, int port, String path, byte method, String* body, JSONValue* data) {
  JSONValue response;
  String rawResponse;
  int status = httpSend(hostname, port, path, method, body, &response, &rawResponse);

  if (!response.isValid()) {
    return false;
  }

  bool ok;
  if (!JSONGetBool(&response, "ok", &ok) || !ok || !JSONGetValue(&response, "data", data)) {
    String msg;
    if (!JSONGetString(&response, "msg", &msg)) {
      msg = String(status);
    }
    return false;
  }

  return true;
}

void setup() {
  Led.init();
}

void loop() {
  Led.setColorRGB(0, 255, 0, 0);
}
