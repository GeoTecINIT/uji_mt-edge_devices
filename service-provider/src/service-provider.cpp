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
#include "DHT.h"
#include "uECC.h" // https://github.com/kmackay/micro-ecc

void cloneConstChar(const char* src, char* dest);
void setState(int newState);
String makeUrl(String path);
int getHexStrByteLength(String hexStr);
byte* hexToBytes(String hexStr);
String bytesToHex(byte* bytes, int length);
String servicesToArrayString();
static int randomBytes(uint8_t *destination, unsigned size);
bool JSONGetBool(JSONValue* value, String propertyName, bool* out);
bool JSONGetDouble(JSONValue* value, String propertyName, double* out);
bool JSONGetInt(JSONValue* value, String propertyName, int* out);
bool JSONGetString(JSONValue* value, String propertyName, String* out);
bool JSONGetValue(JSONValue* value, String propertyName, JSONValue* out);
int fogApiSend(String path, byte method, String body, String hostname, int port,
  JSONValue* responseJSON, String* rawResponse);
int fogApiGet(String path, JSONValue* responseJSON, String hostname, int port, String* rawResponse);
int fogApiGet(String path, JSONValue* jsonValue);
int fogApiPost(String path, String body, JSONValue* responseJSON, String hostname, int port, String* rawResponse);
int fotApiPost(String path, String body, JSONValue* jsonValue);
void publishFogFail(String path, String msg);
bool fogApiSendRequestAndReturnData(String path, byte method, String body, JSONValue* data, String hostname, int port);
bool fogApiGetData(String path, JSONValue* data, String hostname, int port);
bool fogApiGetData(String path, JSONValue* data);
bool fogApiPostData(String path, String body, JSONValue* data, String hostname, int port);
bool fogApiPostData(String path, String body, JSONValue* data);
bool fogApiPostTransaction(String path, String body, String* txHash, String hostname, int port);
bool fogApiPostTransaction(String path, String body, String* txHash);
bool fogPostSignSubmitTransaction(String path, String body);
bool testHostname(String hostname);
void testFogUpdateState();
void updateHostname(String newHostname);
void updateRegionID(byte newID);
void checkRegion();
void setLocation(byte* location);
bool validLocation(byte* location);
void splitSignatures(byte* signatures, byte* r, byte* s, int* v);
void checkDataForUpdate(JSONValue* data);
bool isRegistered();
void registerDeviceToFog();
bool tokenIdentical(byte* token1, byte* token2);
int getServiceConnectionIndex(byte* token);
int getServiceIndex(String *search);
int startService(int serviceIndex, int durationSec);
void stopServiceConnection(int serviceConnectionIndex);
void serviceFunctionTemp(String &responseData);
void serviceFunctionHumd(String &responseData);
void checkExpiredConnections();
void apiReturnSuccess(String& response, char* data);
void apiReturnFail(String& response, String msg, char* data);
void apiReturnInvalidInput(String& response);
bool apiRequireMethod(RestAPIEndpointMsg& request, String& response, int method);
bool apiRequireState(RestAPIEndpointMsg& request, String& response, int state);
void apiAlive(RestAPIEndpointMsg& request, String& response);
void apiSetFogIp(RestAPIEndpointMsg& request, String& response);
void apiGetLocation(RestAPIEndpointMsg& request, String& response);
void apiSetLocation(RestAPIEndpointMsg& request, String& response);
void apiSetEthAccount(RestAPIEndpointMsg& request, String& response);
void apiListServices(RestAPIEndpointMsg& request, String& response);
void apiStartService(RestAPIEndpointMsg& request, String& response);
void apiGetService(RestAPIEndpointMsg& request, String& response);
void apiStopService(RestAPIEndpointMsg& request, String& response);
void setup();
void loop();
void setupAPI();
void loopStartup();
void loopLost();
void loopIdle();
#line 11 "c:/Users/Ponlawat/source/uji/edge-devices/service-provider/src/service-provider.ino"
#define byte            uint8_t

#define STATE_STARTUP   1
#define STATE_LOST      2
#define STATE_IDLE      3

#define API_METHOD_GET  1
#define API_METHOD_POST 2

#define FOG_METHOD_GET  1
#define FOG_METHOD_POST 2

#define VERBOSITY_NO      0
#define VERBOSITY_GENERAL 1
#define VERBOSITY_DEBUG   2

#define VERBOSITY         VERBOSITY_GENERAL | VERBOSITY_DEBUG

#define TX_SUBMIT_TRY_COUNT 10

#define UNCONECCTED_RANDOM_PIN  A0

#define MAX_CONNECTION_LENGTH   5

#define SENSOR_DHT_PIN  D2
#define SENSOR_DHT_TYPE DHT11

typedef void (*ServiceFunction)(String&);

struct ServiceConnection {
  byte* token;
  int serviceIndex;
  int expiration;
};

int CurrentState = STATE_STARTUP;

IPAddress LocalIP;

String FogAPIHostName = "";
int FogAPIPort = 80;

bool RegisteredChecked = false;

String EthAddress = "0x0000000000000000000000000000000000000000";
byte* EthPrivate = new byte[32];
int EthChainID = 0;

String Services[] = {"TEMP", "HUMD"};
ServiceFunction ServiceFunctions[2];
int ServiceLength = 2;

ServiceConnection* ServiceConnections[MAX_CONNECTION_LENGTH];

RestAPIEndpoints apiEndPoints;
RdWebServer* webServer;

byte CurrentLocation[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte CurrentRegionID = 0;

DHT dht(SENSOR_DHT_PIN, SENSOR_DHT_TYPE);
float RecordedTemperature = 0;
float RecordedHumidity = 0;

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
    (VERBOSITY & VERBOSITY_GENERAL) && Particle.publish("STATE_CHANGED", String(CurrentState));
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
  String result = "0x";
  for (int i = 0; i < length; i++) {
    char* byteHex = new char[1];
    sprintf(byteHex, "%x", (bytes[i] >> 4));
    result += byteHex;
    sprintf(byteHex, "%x", (bytes[i] & 0x0f));
    result += byteHex;
  }
  return result;
}

String servicesToArrayString() {
  String arr = "[";
  for (int i = 0; i < ServiceLength; i++) {
    if (i > 0) {
      arr += ",";
    }
    arr += "\"" + Services[i] + "\"";
  }
  arr += "]";
  return arr;
}

static int randomBytes(uint8_t *destination, unsigned size) {
  for (int i = 0; i < size; i++) {
    byte value = 0;
    for (int j = 0; j < 8; j++) {
      int init = analogRead(UNCONECCTED_RANDOM_PIN);
      int count = 0;
      while (analogRead(UNCONECCTED_RANDOM_PIN) == init) {
        count++;
      }

      value = count == 0 ? (value << 1) | (init & 0x01) : (value << 1) | (count & 0x01);
    }
    destination[i] = value;
  }
  return 1;
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

int fogApiSend(String path, byte method, String body, String hostname, int port,
  JSONValue* responseJSON, String* rawResponse) {
  HttpClient http;
  http_header_t headers[] = {
    {"Content-Type", "application/json"},
    {"Accept", "*/*"},
    {"User-agent", "Particle HttpClient"},
    {"Address", EthAddress},
    {NULL, NULL}
  };
  http_request_t request;
  http_response_t response;

  request.hostname = hostname;
  request.port = port;
  request.path = path;

  switch (method) {
    case FOG_METHOD_GET:
      http.get(request, response, headers); break;
    case FOG_METHOD_POST:
      request.body = body;
      http.post(request, response, headers); break;
    default:
      return -1;
  }

  *responseJSON = JSONValue::parseCopy(response.body);
  *rawResponse = response.body;
  return response.status;
}

int fogApiGet(String path, JSONValue* responseJSON, String hostname, int port, String* rawResponse) {
  return fogApiSend(path, FOG_METHOD_GET, "", hostname, port, responseJSON, rawResponse);
}

int fogApiGet(String path, JSONValue* jsonValue) {
  String rawResponse;
  return fogApiGet(path, jsonValue, FogAPIHostName, FogAPIPort, &rawResponse);
}

int fogApiPost(String path, String body, JSONValue* responseJSON, String hostname, int port, String* rawResponse) {
  return fogApiSend(path, FOG_METHOD_POST, body, hostname, port, responseJSON, rawResponse);
}

int fotApiPost(String path, String body, JSONValue* jsonValue) {
  String pure;
  return fogApiPost(path, body, jsonValue, FogAPIHostName, FogAPIPort, &pure);
}

void publishFogFail(String path, String msg) {
  if (((VERBOSITY) & (VERBOSITY_DEBUG)) > 0) {
    msg = msg.replace("\"", "\\\"");
    Particle.publish("FOG_API_FAIL", "{\"path\":\"" + path + "\", \"err\":\"" + msg + "\"}");
  }
}

bool fogApiSendRequestAndReturnData(String path, byte method, String body, JSONValue* data, String hostname, int port) {
  JSONValue response;
  String rawResponse;
  int status;
  switch (method) {
    case FOG_METHOD_GET:
      status = fogApiGet(path, &response, hostname, port, &rawResponse); break;
    case FOG_METHOD_POST:
      status = fogApiPost(path, body, &response, hostname, port, &rawResponse); break;
    default:
      return false;
  }

  if (!response.isValid()) {
    publishFogFail(path, rawResponse);
    return false;
  }

  bool ok;
  if (!JSONGetBool(&response, "ok", &ok) || !ok || !JSONGetValue(&response, "data", data)) {
    String msg;
    if (!JSONGetString(&response, "msg", &msg)) {
      msg = String(status);
    }
    publishFogFail(path, msg);
    return false;
  }

  return true;
}

bool fogApiGetData(String path, JSONValue* data, String hostname, int port) {
  return fogApiSendRequestAndReturnData(path, FOG_METHOD_GET, "", data, hostname, port);
}

bool fogApiGetData(String path, JSONValue* data) {
  return fogApiGetData(path, data, FogAPIHostName, FogAPIPort);
}

bool fogApiPostData(String path, String body, JSONValue* data, String hostname, int port) {
  return fogApiSendRequestAndReturnData(path, FOG_METHOD_POST, body, data, hostname, port);
}

bool fogApiPostData(String path, String body, JSONValue* data) {
  return fogApiPostData(path, body, data, FogAPIHostName, FogAPIPort);
}

bool fogApiPostTransaction(String path, String body, String* txHash, String hostname, int port) {
  JSONValue data;
  if (!fogApiSendRequestAndReturnData(path, FOG_METHOD_POST, body, &data, hostname, port)) {
    return false;
  }

  if (!JSONGetString(&data, "hash", txHash) || !JSONGetInt(&data, "chainID", &EthChainID)) {
    publishFogFail(path, "INVALID_RESPONSE");
    return false;
  }

  return true;
}

bool fogApiPostTransaction(String path, String body, String* txHash) {
  return fogApiPostTransaction(path, body, txHash, FogAPIHostName, FogAPIPort);
}

bool fogPostSignSubmitTransaction(String path, String body) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("FOG_REQTX_REGISTER", body);
  String txHashStr;
  if (!fogApiPostTransaction(path, body, &txHashStr)) {
    return false;
  }

  byte* txHash = hexToBytes(txHashStr);

  byte* publicKey = new byte[64];
  uECC_compute_public_key(EthPrivate, publicKey, uECC_secp256k1());

  int tryCount = 0;
  while (tryCount++ < TX_SUBMIT_TRY_COUNT) {
    byte* signatures = new byte[64];
    if (uECC_sign(EthPrivate, txHash, 32, signatures, uECC_secp256k1()) != 1) {
      continue;
    }

    if (uECC_verify(publicKey, txHash, 32, signatures, uECC_secp256k1()) != 1) {
      continue;
    }

    byte* r = new byte[32];
    byte* s = new byte[32];
    int v;
    splitSignatures(signatures, r, s, &v);
    char* vHex = new char[20];
    sprintf(vHex, "0x%x", v);

    String submitSignatureBody = "{\"tx\":\"" + bytesToHex(txHash, 32) + "\","
      + "\"r\":\"" + bytesToHex(r, 32) + "\",\"s\":\"" + bytesToHex(s, 32) + "\","
      + "\"v\":\"" + vHex + "\"}";

    JSONValue* dataResponse;
    if (fogApiPostData("/transaction/submit-signature", submitSignatureBody, dataResponse)) {
      ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("TX_SUBMIT_SUCCESS", txHashStr);
      return true;
    }

    delete signatures, r, s, vHex;
    delay(1000);
  }

  delete txHash, publicKey;

  Particle.publish("TX_SUBMIT_FAIL", txHashStr);
  return false;
}

bool testHostname(String hostname) {
  JSONValue data;
  return fogApiGetData("/alive", &data, hostname, FogAPIPort);
}

void testFogUpdateState() {
  if (FogAPIHostName != "" && testHostname(FogAPIHostName)) {
    setState(STATE_IDLE);
    checkRegion();
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
  if (!validLocation(CurrentLocation)) {
    return;
  }

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
  if (!validLocation(location)) {
    return;
  }

  for (int i = 0; i < 8; i++) {
    CurrentLocation[i] = location[i];
  }
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("LOCATION_UPDATED", bytesToHex(CurrentLocation, 8));

  if (CurrentState != STATE_IDLE) {
    return;
  }

  checkRegion();

  fogPostSignSubmitTransaction("/device/update/location/tx", "{\"location\":\"" + bytesToHex(CurrentLocation, 8) + "\"}");
}

bool validLocation(byte* location) {
  byte checking = 0x0;
  for (int i = 0; i < 8; i++) {
    checking |= location[i];
  }
  return checking != 0x0;
}

void splitSignatures(byte* signatures, byte* r, byte* s, int* v) {
  for (int i = 0; i < 32; i++) {
    r[i] = signatures[i];
  }
  for (int i = 32; i < 64; i++) {
    s[i - 32] = signatures[i];
  }
  *v = (EthChainID * 2) + 35;
}

void checkDataForUpdate(JSONValue* data) {
  String ipv4;
  if (JSONGetString(data, "ipv4", &ipv4)) {
    String localIP = LocalIP.toString();
    if (ipv4.compareTo(localIP) != 0) {
      fogPostSignSubmitTransaction("/device/update/ip/tx",
        "{\"ipv4\":\"" + localIP + "\",\"ipv6\":null}");
    }
  }
}

bool isRegistered() {
  JSONValue data;
  bool registered = fogApiGetData("/device/get/" + EthAddress, &data);
  return registered;
}

void registerDeviceToFog() {
  String body = "{\"location\":\"" + bytesToHex(CurrentLocation, 8) + "\","
    + "\"ipv4\":\"" + LocalIP.toString() + "\",\"ipv6\":null,"
    + "\"services\":" + servicesToArrayString() + "}";
  
  fogPostSignSubmitTransaction("/device/register/tx", body);
}

bool tokenIdentical(byte* token1, byte* token2) {
  for (int i = 0; i < 8; i++) {
    if (token1[i] != token2[i]) {
      return false;
    }
  }
  return true;
}

int getServiceConnectionIndex(byte* token) {
  for (int i = 0; i < MAX_CONNECTION_LENGTH; i++) {
    if (ServiceConnections[i] == NULL) {
      continue;
    }
    if (tokenIdentical(token, ServiceConnections[i]->token)) {
      return i;
    }
  }
  return -1;
}

int getServiceIndex(String *search) {
  for (int i = 0; i < ServiceLength; i++) {
    if (Services[i].compareTo(*search) == 0) {
      return i;
    }
  }

  return -1;
}

int startService(int serviceIndex, int durationSec) {
  byte* token = new byte[8];
  do {
    randomBytes(token, 8);
    if (getServiceConnectionIndex(token) == -1) {
      break;
    }
  } while (true);
  
  for (int i = 0; i < MAX_CONNECTION_LENGTH; i++) {
    if (ServiceConnections[i] == NULL) {
      ServiceConnections[i] = new ServiceConnection();
      ServiceConnections[i]->token = token;
      ServiceConnections[i]->serviceIndex = serviceIndex;
      ServiceConnections[i]->expiration = Time.now() + durationSec;
      return i;
    }
  }

  return -1;
}

void stopServiceConnection(int serviceConnectionIndex) {
  JSONValue* data;
  String serviceName = Services[ServiceConnections[serviceConnectionIndex]->serviceIndex];
  String token = bytesToHex(ServiceConnections[serviceConnectionIndex]->token, 8);
  fogApiPostData("/service/close", "{\"service\":\"" + serviceName + "\",\"token\":\"" + token + "\"}", data);
  delete ServiceConnections[serviceConnectionIndex]->token;
  delete ServiceConnections[serviceConnectionIndex];
  ServiceConnections[serviceConnectionIndex] = NULL;
}

void serviceFunctionTemp(String &responseData) {
  float value = dht.readTemperature(false, true);
  if (isnan(value)) {
    value = RecordedTemperature;
  } else {
    RecordedTemperature = value;
  }
  responseData = "{\"value\":" + String(value) + ",\"unit\":\"celcius\"}";
}

void serviceFunctionHumd(String &responseData) {
  float value = dht.readHumidity(true);
  if (isnan(value)) {
    value = RecordedHumidity;
  } else {
    value /= 100.0;
    RecordedHumidity = value;
  }
  responseData = "{\"value\":" + String(value) + "}";
}

void checkExpiredConnections() {
  for (int i = 0; i < MAX_CONNECTION_LENGTH; i++) {
    if (ServiceConnections[i] != NULL && Time.now() > ServiceConnections[i]->expiration) {
      stopServiceConnection(i);
    }
  }
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

bool apiRequireState(RestAPIEndpointMsg& request, String& response, int state) {
  if (CurrentState != state) {
    apiReturnFail(response, "Not now", NULL);
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
  RegisteredChecked = false;
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

  byte* location = hexToBytes(cellHex);
  setLocation(location);
  delete location;
}

void apiSetEthAccount(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SET_ETHACC");
  if (!apiRequireMethod(request, response, API_METHOD_POST)) {
    return;
  }

  JSONValue value = JSONValue::parseCopy((char*)request._pMsgContent);
  String address, privateKey;
  if (!JSONGetString(&value, "address", &address)
    || !JSONGetString(&value, "privateKey", &privateKey)) {
    apiReturnInvalidInput(response);
    return;
  }

  if (address.length() != 42 || privateKey.length() != 66) {
    apiReturnInvalidInput(response);
    return;
  }

  byte* privateKeyBytes = hexToBytes(privateKey);
  for (int i = 0; i < 32; i++) {
    EthPrivate[i] = privateKeyBytes[i];
  }
  delete privateKeyBytes;
  EthAddress = address;

  RegisteredChecked = false;
  apiReturnSuccess(response, NULL);
}

void apiListServices(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SERVICES_LIST");
  if (!apiRequireMethod(request, response, API_METHOD_GET) || !apiRequireState(request, response, STATE_IDLE)) {
    return;
  }

  String data = "[";
  for (int i = 0; i < ServiceLength; i++) {
    if (i > 0) {
      data += ",";
    }
    data += "\"" + Services[i] + "\"";
  }
  return apiReturnSuccess(response, (char*)data.c_str());
}

void apiStartService(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SERVICE_START");
  if (!apiRequireMethod(request, response, API_METHOD_POST) || !apiRequireState(request, response, STATE_IDLE)) {
    return;
  }

  JSONValue value = JSONValue::parseCopy((char*)request._pMsgContent);
  String service;
  int durationSec;
  if (!JSONGetString(&value, "service", &service)
    || !JSONGetInt(&value, "durationSec", &durationSec)) {
    return apiReturnInvalidInput(response);
  }

  int serviceIndex = getServiceIndex(&service);
  if (serviceIndex < 0) {
    return apiReturnFail(response, "No service", NULL);
  }
  int connectionIndex = startService(serviceIndex, durationSec);
  if (connectionIndex < 0) {
    return apiReturnFail(response, "Error", NULL);
  }

  String token = bytesToHex(ServiceConnections[connectionIndex]->token, 8);
  response = "{\"ok\":true,\"msg\":null,\"token\":\"" + token + "\"}";
}

void apiGetService(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SERVICE_GET");
  if (!apiRequireMethod(request, response, API_METHOD_GET) || !apiRequireState(request, response, STATE_IDLE)) {
    return;
  }

  String tokenHex = request._pArgStr;
  if (tokenHex.length() != 18) {
    return apiReturnFail(response, "Invalid Token", NULL);
  }
  byte* token = hexToBytes(tokenHex);
  int connectionIndex = getServiceConnectionIndex(token);
  delete token;
  if (connectionIndex < 0) {
    return apiReturnFail(response, "Invalid token", NULL);
  }

  String data = "";
  JSONValue jsonData;
  ServiceFunctions[ServiceConnections[connectionIndex]->serviceIndex](data);
  String serviceName = Services[ServiceConnections[connectionIndex]->serviceIndex];
  fogApiPostData("/service/data", "{\"service\":\"" + serviceName + "\",\"data\":" + data + "}", &jsonData);
  return apiReturnSuccess(response, (char*)data.c_str());
}

void apiStopService(RestAPIEndpointMsg& request, String& response) {
  ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("API_SERVICE_STOP");
  if (!apiRequireMethod(request, response, API_METHOD_POST) || !apiRequireState(request, response, STATE_IDLE)) {
    return;
  }

  JSONValue body = JSONValue::parseCopy((char*)request._pMsgContent);
  String tokenHex;
  if (!JSONGetString(&body, "token", &tokenHex)) {
    return apiReturnInvalidInput(response);
  }
  byte* token = hexToBytes(tokenHex);
  delete token;
  int connectionIndex = getServiceConnectionIndex(token);
  if (connectionIndex < 0) {
    return apiReturnFail(response, "Invalid token", NULL);
  }

  stopServiceConnection(connectionIndex);
  return apiReturnSuccess(response, NULL);
}

void setup() {
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("DEVICE_START");
  uECC_set_rng(&randomBytes);
  ServiceFunctions[0] = &serviceFunctionTemp;
  ServiceFunctions[1] = &serviceFunctionHumd;
  dht.begin();
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
  LocalIP = WiFi.localIP();
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("LOCAL_IP", LocalIP.toString());
  Particle.variable("ipAddress", LocalIP.toString());

  apiEndPoints.addEndpoint("alive", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiAlive, "", "");
  apiEndPoints.addEndpoint("set-fog", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiSetFogIp, "", "");
  apiEndPoints.addEndpoint("get-location", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiGetLocation, "", "");
  apiEndPoints.addEndpoint("set-location", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiSetLocation, "", "");
  apiEndPoints.addEndpoint("set-eth", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiSetEthAccount, "", "");
  apiEndPoints.addEndpoint("list-services", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiListServices, "", "");
  apiEndPoints.addEndpoint("start-service", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiStartService, "", "");
  apiEndPoints.addEndpoint("get-service", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiGetService, "", "");
  apiEndPoints.addEndpoint("stop-service", RestAPIEndpointDef::ENDPOINT_CALLBACK, apiStopService, "", "");

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
  if (!RegisteredChecked) {
    RegisteredChecked = true;
    if (!isRegistered()) {
      registerDeviceToFog();
    }
  }

  if (Time.second() % 10 == 0) {
    checkExpiredConnections();
  }

  webServer->service();
}
