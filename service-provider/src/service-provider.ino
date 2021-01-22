/*
 * Project service-provider
 */

#include "HttpClient.h"
#include "RdWebServer.h"
#include "RestAPIEndpoints.h"
#include "uECC.h" // https://github.com/kmackay/micro-ecc

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

int CurrentState = STATE_STARTUP;

IPAddress LocalIP;

String FogAPIHostName = "";
int FogAPIPort = 80;

bool RegisteredChecked = false;

String EthAddress = "0x0000000000000000000000000000000000000000";
byte* EthPrivate = new byte[32];
int EthChainID = 0;

String Services[] = {"TEMP", "HUMD"};
int ServiceLength = 2;

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

static int bigRandom(uint8_t *destination, unsigned size) {
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
  if (!fogApiPostTransaction("/device/register/tx", body, &txHashStr)) {
    return false;
  }

  byte* txHash = hexToBytes(txHashStr);

  byte* publicKey = new byte[64];
  uECC_compute_public_key(EthPrivate, publicKey, uECC_secp256k1());

  int tryCount = 0;
  while (tryCount++ < TX_SUBMIT_TRY_COUNT) {
    byte* signatures = new byte[64];
    int signStatus = uECC_sign(EthPrivate, txHash, 32, signatures, uECC_secp256k1());
    if (signStatus != 1) {
      Particle.publish("DEBUG_SIG_FAIL", String(signStatus));
      delay(500);
      continue;
    }

    if (uECC_verify(publicKey, txHash, 32, signatures, uECC_secp256k1()) != 1) {
      Particle.publish("DEBUG_UNVERIFIED_SIG", bytesToHex(signatures, 64));
      delay(500);
      continue;
    }

    byte* r = new byte[32];
    byte* s = new byte[32];
    int v;
    splitSignatures(signatures, r, s, &v);
    char* vHex = new char[20];
    sprintf(vHex, "0x%x", v);

    String submitSignatureBody = "{\"tx\":\"" + bytesToHex(txHash, 32) + "\","
      + "\"priv\":\"" + bytesToHex(EthPrivate, 32) + "\","
      + "\"signature\":\"" + bytesToHex(signatures, 64) + "\","
      + "\"r\":\"" + bytesToHex(r, 32) + "\",\"s\":\"" + bytesToHex(s, 32) + "\","
      + "\"v\":\"" + vHex + "\"}";
    Particle.publish("DEBUG", submitSignatureBody);
    delay(500);

    JSONValue* dataResponse;
    if (fogApiPostData("/transaction/submit-signature", submitSignatureBody, dataResponse)) {
      ((VERBOSITY) & (VERBOSITY_DEBUG)) && Particle.publish("TX_SUBMIT_SUCCESS", txHashStr);
      return true;
    }

    delay(1000);
  }

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

void splitSignatures(byte* signatures, byte* r, byte* s, int* v) {
  for (int i = 0; i < 32; i++) {
    r[i] = signatures[i];
  }
  for (int i = 32; i < 64; i++) {
    s[i - 32] = signatures[i];
  }
  *v = (EthChainID * 2) + 35;
}

bool isRegistered() {
  JSONValue data;
  return fogApiGetData("/device/get/" + EthAddress, &data);
}

void registerDeviceToFog() {
  String body = "{\"location\":\"" + bytesToHex(CurrentLocation, 8) + "\","
    + "\"ipv4\":\"" + LocalIP.toString() + "\",\"ipv6\":null,"
    + "\"services\":" + servicesToArrayString() + "}";
  
  fogPostSignSubmitTransaction("/device/register/tx", body);
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

  setLocation(hexToBytes(cellHex));
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
  EthAddress = address;

  RegisteredChecked = false;
  apiReturnSuccess(response, NULL);
}

void setup() {
  ((VERBOSITY) & (VERBOSITY_GENERAL)) && Particle.publish("DEVICE_START");
  uECC_set_rng(&bigRandom);
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
  if (!RegisteredChecked && !isRegistered()) {
    RegisteredChecked = true;
    registerDeviceToFog();
  }
  webServer->service();
}
