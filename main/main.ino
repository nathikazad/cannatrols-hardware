// main.ino
#include "config.h"
#include <WiFi.h>

State state;
Target storeTarget;
Target cureTarget;
Target dryTarget;

bool wifiConnectionInProgress = false;

void setup() {
  Serial.begin(115200);
  loadTargets();    
  loadState();
  setupBLE();
  setupMQTT();
}

bool connectToWifi() {
  // Prevent concurrent WiFi connection attempts
  if (wifiConnectionInProgress) {
    Serial.println("WiFi connection already in progress, skipping");
    return false;
  }
  
  wifiConnectionInProgress = true;
  
  String ssid, password;
  bool wifiCredentialsSet = getWiFiCredentials(ssid, password);
  if (wifiCredentialsSet) {
    Serial.println("Found stored WiFi credentials. Connecting to: " + ssid);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Disconnecting from current WiFi network");
      WiFi.disconnect(true);
      delay(1000); // Brief delay to ensure disconnect completes
    }
    WiFi.begin(ssid, password);
    // Wait for connection with timeout
    int timeout = 20; // 10 seconds timeout (20 * 500ms)
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(500);
      Serial.print(".");
      timeout--;
    }
    if (WiFi.status() == WL_CONNECTED) {
      sendBleData("WIFI_CONNECTED:SUCCESS");
      Serial.println("\nConnected to WiFi!");
      Serial.println("IP address: " + WiFi.localIP().toString());
      fetchAndSaveOwnerId();
    } else {
      sendBleData("WIFI_CONNECTED:FAILED");
      Serial.println("\nFailed to connect to WiFi.");
    }
  } else {
    Serial.println("No WiFi credentials stored.");
  }
  
  wifiConnectionInProgress = false;
  return (WiFi.status() == WL_CONNECTED);
}

void fetchAndSaveOwnerId() {
  String deviceId = getDeviceId();
  Serial.println("Device Id: "+ deviceId);
  String ownerId;
  String deviceName;
  bool success = askServerForDeviceInfo(deviceId, ownerId, deviceName);
  if (success) {
    Serial.println("OWNER_ID:" + ownerId);
    Serial.println("DEVICE_NAME:" + deviceName);
    saveOwnerId(ownerId);
    saveDeviceName(deviceName);
  } else {
    Serial.println("MACHINE_OWNER_ID:ERROR");
  }
}

void bleDataReceiveCallback(String receivedData) {
  // Process commands from BLE client
  receivedData.trim();
  
  if (receivedData == "GET_DEVICE_ID") {
    sendBleData("DEVICE_ID:" + getDeviceId());
  }
  else if (receivedData == "IS_WIFI_CONNECTED") {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    sendBleData("WIFI_CONNECTED:" + String(isConnected ? "SUCCESS" : "FAILED"));
  }
  else if (receivedData == "GET_WIFI_CREDENTIALS") {
    String ssid, password;
    getWiFiCredentials(ssid, password);
    sendBleData("WIFI_SSID:" + ssid);    
  }
  else if (receivedData.startsWith("SET_WIFI_CREDENTIALS:")) {
    // Format: SET_WIFI_CREDENTIALS:SSID,PASSWORD
    String credentials = receivedData.substring(21); // Skip command prefix
    int commaIndex = credentials.indexOf(',');
    if (commaIndex != -1) {
      String ssid = credentials.substring(0, commaIndex);
      String password = credentials.substring(commaIndex + 1);
      saveWiFiCredentials(ssid.c_str(), password.c_str());
      connectToWifi();
    } else {
      sendBleData("WIFI_CONNECTED:INVALID_FORMAT");
    }
  }
  else if (receivedData.startsWith("GET_MACHINE_OWNER:")) {
    fetchAndSaveOwnerId();
    String ownerId = getOwnerId();
    sendBleData("MACHINE_OWNER:"+ownerId);
  }
  else if (receivedData.startsWith("SET_DEVICE_NAME:")) {
    int colonIndex = receivedData.indexOf(':');
    String deviceName = receivedData.substring(colonIndex + 1);
    saveDeviceName(deviceName);
    sendBleData("DEVICE_NAME:" + deviceName);
  }
  else if (receivedData.startsWith("GET_DEVICE_NAME")) {
    String deviceName = getDeviceName();
    sendBleData("DEVICE_NAME:" + deviceName);
  }
  else {
    // Echo unknown commands
    sendBleData("UNKNOWN_COMMAND:" + receivedData);
  }
}

void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, trying to connect to it");
    connectToWifi();
  }
  delay(10000);
}