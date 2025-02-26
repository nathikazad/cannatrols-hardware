// main.ino
#include "config.h"
#include <WiFi.h>

// For storing WiFi credentials
Preferences preferences;

void setup() {
  Serial.begin(115200);
  
  // Initialize preferences for storing WiFi credentials
  preferences.begin("wifi", false);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();
  
  if (ssid.length() > 0) {
    Serial.println("Found stored WiFi credentials. Connecting to: " + ssid);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    int timeout = 20; // 10 seconds timeout (20 * 500ms)
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(500);
      Serial.print(".");
      timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      String deviceId = getDeviceId();
      Serial.println(deviceId);
      String ownerId;
      bool success = askServerForMachineOwnerId(deviceId, ownerId);
      
      if (success) {
        Serial.println("MACHINE_OWNER_ID:" + ownerId);
      } else {
        Serial.println("MACHINE_OWNER_ID:ERROR");
      }
    } else {
      Serial.println("\nFailed to connect to WiFi.");
    }
  } else {
    Serial.println("No WiFi credentials stored.");
  }
  
  setupBLE();
}

String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char deviceId[13];
  sprintf(deviceId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  return String(deviceId);
}

void bleDataReceiveCallback(String receivedData) {
  // Process commands from BLE client
  receivedData.trim();
  
  if (receivedData == "GET_DEVICE_ID") {
    // Return a unique device identifier (using chip ID on ESP32)
    uint64_t chipId = ESP.getEfuseMac();
    char deviceId[13];
    sprintf(deviceId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    sendBleData("DEVICE_ID:" + getDeviceId());
  }
  else if (receivedData == "IS_INTERNET_CONNECTED") {
    // Check if the device has internet connectivity
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    sendBleData("INTERNET_STATUS:" + String(isConnected ? "CONNECTED" : "DISCONNECTED"));
  }
  else if (receivedData == "GET_WIFI_CREDENTIALS") {
    // Return the current WiFi credentials (SSID only for security)
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "NONE");
    preferences.end();
    sendBleData("WIFI_SSID:" + ssid);    
  }
  else if (receivedData.startsWith("SET_WIFI_CREDENTIALS:")) {
    // Format: SET_WIFI_CREDENTIALS:SSID,PASSWORD
    String credentials = receivedData.substring(21); // Skip command prefix
    int commaIndex = credentials.indexOf(',');
    
    if (commaIndex != -1) {
      String ssid = credentials.substring(0, commaIndex);
      String password = credentials.substring(commaIndex + 1);

      // save credentials to non-volatile storage
      preferences.begin("wifi", false);
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.end();
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Disconnecting from current WiFi network");
        WiFi.disconnect(true);
        delay(1000); // Brief delay to ensure disconnect completes
      }
      WiFi.begin(ssid.c_str(), password.c_str());
      
      // Wait for connection with timeout
      int timeout = 10; // 10 seconds timeout
      while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        timeout--;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        sendBleData("WIFI_CONNECTED:SUCCESS");
      } else {
        sendBleData("WIFI_CONNECTED:FAILED");
      }
    } else {
      sendBleData("WIFI_CONNECTED:INVALID_FORMAT");
    }
  }
  else if (receivedData.startsWith("GET_MACHINE_OWNER:")) {
    preferences.begin("user", false);
    String userId = preferences.getString("id", "NONE");
    preferences.end();
    sendBleData("USER_CREDENTIALS:"+userId);
  }
  else if (receivedData.startsWith("SET_DEVICE_NAME:")) {
    // Format: SET_WIFI_CREDENTIALS:SSID,PASSWORD
    int colonIndex = receivedData.indexOf(':');
    String deviceName = receivedData.substring(colonIndex + 1);
    // save credentials to non-volatile storage
    preferences.begin("device", false);
    preferences.putString("name", deviceName);
    preferences.end();
    sendBleData("DEVICE_NAME:"+deviceName);
  }
  else if (receivedData.startsWith("GET_DEVICE_NAME:")) {
    preferences.begin("device", false);
    String deviceName = preferences.getString("device", "NONE");
    preferences.end();
    sendBleData("DEVICE_NAME:"+deviceName);
  }
  else {
    // Echo unknown commands
    sendBleData("UNKNOWN_COMMAND:" + receivedData);
  }
}

void loop() {
  if (Serial.available() && deviceConnected && isAuthenticated) {
    String input = Serial.readStringUntil('\n');
    Serial.println("Received from Serial: " + input);
    sendBleData(input);
  }
  delay(10);
}