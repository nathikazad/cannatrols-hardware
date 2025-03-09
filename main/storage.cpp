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

String getOwnerId() {
  preferences.begin("owner", false);
  String ownerId = preferences.getString("id", "NONE");
  preferences.end();
  return ownerId;
}

bool ownerIdIsNone() {
  return getOwnerId().equals("None");
}

void saveTargets() {
  preferences.begin("targets", false);
  preferences.putFloat("storeTargetTemperature", storeTarget.temperature);
  preferences.putFloat("storeTargetDewPoint", storeTarget.dewPoint);
  preferences.putFloat("storeTargetTime", storeTarget.time);
  preferences.putString("storeTargetStepMode", stepModeToString(storeTarget.stepMode));
  preferences.putFloat("cureTargetTemperature", cureTarget.temperature);
  preferences.putFloat("cureTargetDewPoint", cureTarget.dewPoint);
  preferences.putFloat("cureTargetTime", cureTarget.time);
  preferences.putString("cureTargetStepMode", stepModeToString(cureTarget.stepMode));
  preferences.putFloat("dryTargetTemperature", dryTarget.temperature);
  preferences.putFloat("dryTargetDewPoint", dryTarget.dewPoint);
  preferences.putFloat("dryTargetTime", dryTarget.time);
  preferences.putString("dryTargetStepMode", stepModeToString(dryTarget.stepMode));
  preferences.end();
}

void loadTargets() {
  preferences.begin("targets", false);
  storeTarget.temperature = preferences.getFloat("storeTargetTemperature", 68.0);
  storeTarget.dewPoint = preferences.getFloat("storeTargetDewPoint", 54.0);
  storeTarget.time = preferences.getFloat("storeTargetTime", 0);
  storeTarget.stepMode = stringToStepMode(preferences.getString("storeTargetStepMode", "step"));
  cureTarget.temperature = preferences.getFloat("cureTargetTemperature", 68.0);
  cureTarget.dewPoint = preferences.getFloat("cureTargetDewPoint", 54.0);
  cureTarget.time = preferences.getFloat("cureTargetTime", 0);
  cureTarget.stepMode = stringToStepMode(preferences.getString("cureTargetStepMode", "step"));
  dryTarget.temperature = preferences.getFloat("dryTargetTemperature", 68.0);
  dryTarget.dewPoint = preferences.getFloat("dryTargetDewPoint", 54.0);
  dryTarget.time = preferences.getFloat("dryTargetTime", 0);
  dryTarget.stepMode = stringToStepMode(preferences.getString("dryTargetStepMode", "step"));
  preferences.end();
}

void saveState() {
  preferences.begin("state", false);
  preferences.putString("cycle", cycleToString(state.cycle));
  preferences.putFloat("temperature", state.temperature);
  preferences.putFloat("humidity", state.humidity);
  preferences.putFloat("dewPoint", state.dewPoint);
  preferences.putLong("timeLeft", state.timeLeft);
  preferences.putBool("isPlaying", state.isPlaying);
  preferences.end();
}

void loadState() {
  preferences.begin("state", false);
  state.cycle = stringToCycle(preferences.getString("cycle", "store"));
  state.temperature = preferences.getFloat("temperature", 0);
  state.humidity = preferences.getFloat("humidity", 0);
  state.dewPoint = preferences.getFloat("dewPoint", 0);
  state.timeLeft = preferences.getLong("timeLeft", 0);
  state.isPlaying = preferences.getBool("isPlaying", false);
  preferences.end();
}




