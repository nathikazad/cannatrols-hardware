#include <Preferences.h>
#include "config.h"
Preferences preferences;

String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char deviceId[13];
  sprintf(deviceId, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  return String(deviceId);
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  Serial.println("New WiFi credentials saved");
}

bool getWiFiCredentials(String &ssid, String &password) {
  preferences.begin("wifi", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
  return ssid.length() > 0;
}

void saveDeviceName(const String &deviceName) {
  preferences.begin("device", false);
  preferences.putString("name", deviceName);
  preferences.end();
  Serial.println("Device name saved: " + deviceName);
}

String getDeviceName() {
  preferences.begin("device", false);
  String deviceName = preferences.getString("name", "NONE");
  preferences.end();
  return deviceName;
}

void saveOwnerId(const String &ownerId) {
  preferences.begin("owner", false);
  preferences.putString("id", ownerId);
  preferences.end();
  Serial.println("Owner id saved: " + ownerId);
}

void saveState() {
  preferences.begin("state", false);
  preferences.putFloat("targetTemperature", state.targetTemperature);
  preferences.putFloat("targetDewPoint", state.targetDewPoint);
  preferences.putFloat("targetTime", state.targetTime);
  preferences.putString("cycle", cycleToString(state.cycle));
  preferences.putString("stepMode", stepModeToString(state.stepMode));
  preferences.end();
}

void loadState() {
  preferences.begin("state", false);
  state.targetTemperature = preferences.getFloat("targetTemperature", 68.0);
  state.targetDewPoint = preferences.getFloat("targetDewPoint", 54.0);
  state.targetTime = preferences.getFloat("targetTime", 0);
  state.cycle = stringToCycle(preferences.getString("cycle", "store"));
  state.stepMode = stringToStepMode(preferences.getString("stepMode", "step"));
  preferences.end();
}

String getOwnerId() {
  preferences.begin("owner", false);
  String ownerId = preferences.getString("id", "NONE");
  preferences.end();
  return ownerId;
}

bool ownerIdIsNone() {
  return getOwnerId().equals("None");
}

