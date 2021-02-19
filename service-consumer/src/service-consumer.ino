/*
 * Project service-consumer
 */

#include "HttpClient.h"
#include "Grove_ChainableLED.h"

#define METHOD_GET  1
#define METHOD_POST 2

#define LED_PIN1  D2
#define LED_PIN2  D3

#define REPUTATION_THRESHOLD          0.5
#define SERVICE_DURATION              60
#define SERVICE_CONSUMPTION_INTERVAL  10

String FogAPIBaseHostname = "192.168.1.236";
int FogAPIBasePort = 80;

String ServiceProviderHostname = "";
int LocationIdx = 0;
String Locations[] = {"0xd60001c000000000", "0xd5ffe07000000000"};

String ServiceToken;
int ServiceExpiration;

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

int httpSend(String hostname, int port, String path, byte method, String body,
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
      request.body = body;
      http.post(request, response, headers); break;
    default:
      return -1;
  }

  *responseJSON = JSONValue::parseCopy(response.body);
  *rawResponse = response.body;
  return response.status;
}

bool sendRequestAndReturnData(String hostname, int port, String path, byte method, String body, JSONValue* data) {
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

bool fogGet(String path, JSONValue* data) {
  return sendRequestAndReturnData(FogAPIBaseHostname, FogAPIBasePort, path, METHOD_GET, "", data);
}

bool fogPost(String path, String body, JSONValue* data) {
  return sendRequestAndReturnData(FogAPIBaseHostname, FogAPIBasePort, path, METHOD_POST, body, data);
}

bool providerGet(String path, JSONValue* data) {
  return sendRequestAndReturnData(ServiceProviderHostname, 80, path, METHOD_GET, "", data);
}

bool providerPost(String path, String body, JSONValue* data) {
  return sendRequestAndReturnData(ServiceProviderHostname, 80, path, METHOD_POST, body, data);
}

void setup() {
  Led.init();
}

void serviceProviderError(String msg) {
  Led.setColorRGB(0, 127, 0, 255);
  Particle.publish("SERVICE_ERR", msg);
}

bool consumeServiceCheckAlive() {
  JSONValue data;
  int status = 0;
  if (!providerGet("/alive", &data)) {
    serviceProviderError("Cannot get /alive from provider");
    return false;
  }
  if (data.toInt() != 3) {
    serviceProviderError("Provider is not in ready state");
    return false;
  }
  return true;
}

bool consumeServiceStart() {
  JSONValue data;
  ServiceExpiration = Time.now() + SERVICE_DURATION;
  Led.setColorRGB(0, 0, 255, 255);
  String body = "{\"service\":\"TEMP\",\"durationSec\":" + String(SERVICE_DURATION) + "}";
  if (!providerPost("/start-service/", body, &data)) {
    serviceProviderError("Cannot initialise service");
    return false;
  }
  if (!JSONGetString(&data, "token", &ServiceToken)) {
    serviceProviderError("Cannot get token");
    return false;
  }
  return true;
}

bool consumeServiceConsume() {
  JSONValue data;
  while (Time.now() + SERVICE_CONSUMPTION_INTERVAL < ServiceExpiration) {
    delay(SERVICE_CONSUMPTION_INTERVAL * 1000);
    Led.setColorRGB(0, 255, 255, 255);
    if (!providerGet("/get-service/" + ServiceToken, &data)) {
      serviceProviderError("Cannot get service");
      continue;
    }
    double value;
    if (!JSONGetDouble(&data, "value", &value)) {
      serviceProviderError("Cannot get service value");
      continue;
    }
    Led.setColorRGB(0, 0, 255, 0);
    Particle.publish("SERVICE_VALUE", String(value));
  }
  return true;
}

void consumeService() {
  if (!consumeServiceCheckAlive()) {
    return;
  }
  if (!consumeServiceStart()) {
    return;
  }
  if (!consumeServiceConsume()) {
    return;
  }
}

void loop() {
  Led.setColorRGB(0, 255, 255, 255);
  LocationIdx = (LocationIdx + 1) % 2;
  String location = Locations[LocationIdx];
  JSONValue data;
  if (fogGet("/reputation/list/TEMP/at/" + location, &data) && data.isArray()) {
    JSONArrayIterator iterator(data);
    Particle.publish("REPUTATION_LIST", "{\"location\":\"" + location + "\", \"count\": " + String(iterator.count()) + "}");
    if (iterator.count() == 0) {
      Led.setColorRGB(0, 255, 127, 0);
    } else {
      String deviceIP = "";
      double maxReputation = 0;
      for (size_t i = 0; iterator.next(); i++) {
        double reputation;
        JSONValue value = iterator.value();
        JSONGetDouble(&value, "reputation", &reputation);
        if (reputation < REPUTATION_THRESHOLD) {
          continue;
        }
        if (deviceIP.compareTo("") == 0 || reputation > maxReputation) {
          maxReputation = reputation;
          JSONGetString(&value, "ipv4", &deviceIP);
        }
      }

      if (deviceIP.compareTo("") != 0) {
        ServiceProviderHostname = deviceIP;
        Led.setColorRGB(0, 0, 0, 255);
        consumeService();
      } else {
        Led.setColorRGB(0, 255, 127, 0);
      }
    }
  } else {
    Led.setColorRGB(0, 255, 0, 0);
  }
  delay(5000);
}
