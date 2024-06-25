
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: k_suwatchai@hotmail.com
 *
 * Github: https://github.com/mobizt/Firebase-ESP-Client
 *
 * Copyright (c) 2023 mobizt
 *
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
// Insert your network credentials
#define WIFI_SSID "vodafone9AB36A"
#define WIFI_PASSWORD "GsKLbZyxsZY3fmKY"

// Insert Firebase project API Key

// Insert RTDB URLefine the RTDB URL */

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyAevqWIEvp3LruApovAREKFK04oOjhG7CM" //"AIzaSyBJkeTgiUT2fZHOCa6IKQXHpxGxyQpXYJU"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://esp32-47542-default-rtdb.europe-west1.firebasedatabase.app/" // "https://jjegarcia-esp32-default-rtdb.europe-west1.firebasedatabase.app/"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "jjegarcia.test@gmail.com"
#define USER_PASSWORD "test@123"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

enum _type
{
  _boo,
  _int,
  _double,
  _float,
  _string
};

typedef union
{
  void (*saveBool)(String path, bool value);
  void (*saveInt)(String path, int value);
  void (*saveDouble)(String path, double value);
  void (*saveFloat)(String path, float value);
  void (*saveString)(String path, char *value);
} functions;

functions my_functions;

void saveBool(String path, bool value);
void saveInt(String path, int value);
void saveDouble(String path, double value);
void saveFloat(String path, float value);
void saveString(String path, String value);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

  Serial.begin(115200);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    if (millis() - ms > 10000)
      break;
#endif
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  // otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  config.wifi.clearAP();
  config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

  // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
  // https://github.com/mobizt/Firebase-ESP-Client#about-firebasedata-object
  // fbdo.keepAlive(5, 5, 1);

  /** Timeout options.

  //Network reconnect timeout (interval) in ms (10 sec - 5 min) when network or WiFi disconnected.
  config.timeout.networkReconnect = 10 * 1000;

  //Socket connection and SSL handshake timeout in ms (1 sec - 1 min).
  config.timeout.socketConnection = 10 * 1000;

  //Server response read timeout in ms (1 sec - 1 min).
  config.timeout.serverResponse = 10 * 1000;

  //RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.
  config.timeout.rtdbKeepAlive = 45 * 1000;

  //RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.
  config.timeout.rtdbStreamReconnect = 1 * 1000;

  //RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream
  //will return false (error) when it called repeatedly in loop.
  config.timeout.rtdbStreamError = 3 * 1000;

  Note:
  The function that starting the new TCP session i.e. first time server connection or previous session was closed, the function won't exit until the
  time of config.timeout.socketConnection.

  You can also set the TCP data sending retry with
  config.tcp_data_sending_retry = 1;

  */
}

template <typename T>
void saveValue(_type path, T value)
{
  switch (path)
  {
    break;
  case _boo:
    return my_functions.saveBool("/test/bool", value);
  case _int:
    return my_functions.saveInt("/test/int", value);
  case _double:
    return my_functions.saveDouble("/test/double", value);
  case _float:
    return my_functions.saveFloat("/test/float", value);
    // case _string:
    //   return my_functions.saveString("/test/string", value);
  }
}
void saveBool(String path, bool value)
{
  Serial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, path, value) ? "ok" : fbdo.errorReason().c_str());

  Serial.printf("Get bool... %s\n", Firebase.RTDB.getBool(&fbdo, path) ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());
}

void saveInt(String path, int value)
{
  Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdo, path, value) ? "ok" : fbdo.errorReason().c_str());

  Serial.printf("Get int... %s\n", Firebase.RTDB.getInt(&fbdo, path) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
}

void saveFloat(String path, float value)
{
  Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, path, value) ? "ok" : fbdo.errorReason().c_str());

  Serial.printf("Get float... %s\n", Firebase.RTDB.getFloat(&fbdo, path) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
}

void saveDouble(String path, double value)
{
  Serial.printf("Set double... %s\n", Firebase.RTDB.setDouble(&fbdo, path, value) ? "ok" : fbdo.errorReason().c_str());

  Serial.printf("Get double... %s\n", Firebase.RTDB.getDouble(&fbdo, path) ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());
}

void saveString(String path, String value)
{
  Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, path, value) ? "ok" : fbdo.errorReason().c_str());
  Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, path) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
}

void loop()
{
  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    saveValue(_boo, true);
    saveValue(_int,3);
    saveValue(_double,4.4);
    saveValue(_float,33.354);
    saveString("/test/string", "stringy");
    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Edit_Parse.ino
    FirebaseJson json;

    if (count == 0)
    {
      json.set("value/round/" + String(count), F("cool!"));
      json.set(F("value/ts/.sv"), F("timestamp"));
      Serial.printf("Set json... %s\n", Firebase.RTDB.set(&fbdo, F("/test/json"), &json) ? "ok" : fbdo.errorReason().c_str());
    }
    else
    {
      json.add(String(count), F("smart!"));
      Serial.printf("Update node... %s\n", Firebase.RTDB.updateNode(&fbdo, F("/test/json/value/round"), &json) ? "ok" : fbdo.errorReason().c_str());
    }

    Serial.println();

    // For generic set/get functions.

    // For generic set, use Firebase.RTDB.set(&fbdo, <path>, <any variable or value>)

    // For generic get, use Firebase.RTDB.get(&fbdo, <path>).
    // And check its type with fbdo.dataType() or fbdo.dataTypeEnum() and
    // cast the value from it e.g. fbdo.to<int>(), fbdo.to<std::string>().

    // The function, fbdo.dataType() returns types String e.g. string, boolean,
    // int, float, double, json, array, blob, file and null.

    // The function, fbdo.dataTypeEnum() returns type enum (number) e.g. firebase_rtdb_data_type_null (1),
    // firebase_rtdb_data_type_integer, firebase_rtdb_data_type_float, firebase_rtdb_data_type_double,
    // firebase_rtdb_data_type_boolean, firebase_rtdb_data_type_string, firebase_rtdb_data_type_json,
    // firebase_rtdb_data_type_array, firebase_rtdb_data_type_blob, and firebase_rtdb_data_type_file (10)

    count++;
  }
}
