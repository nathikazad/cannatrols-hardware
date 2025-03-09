#include "config.h"
#include <ArduinoJson.h>

void resetTimeLeft();
// This gets called when a command is received from the mobile app
void commandCallback(StaticJsonDocument<200> doc) {
  String command = doc["command"];
  if (command == "advanceCycle") { // This gets called when the cycle is advanced
    String cycle = doc["cycle"];
    state.cycle = stringToCycle(cycle);
    if (state.cycle == dry) {
      state.timeLeft = dryTarget.time * 1000;
    } else if (state.cycle == cure) {
      state.timeLeft = cureTarget.time * 1000;
    } else if (state.cycle == store) {
      state.timeLeft = 0;
    }
    state.isPlaying = true;
    publishState(true);
  } else if (command == "setTargets") { // This gets called when the targets are set
    String cycle = doc["cycle"];
    Serial.println("setTargets for "+cycle);
    Target target;
    target.temperature = round(doc["targetTemperature"].as<float>() * 10) / 10.0;
    target.dewPoint = round(doc["targetDewPoint"].as<float>() * 10) / 10.0;
    target.time = doc["targetTime"];
    target.stepMode = stringToStepMode(doc["stepMode"]);
    if (cycle == "store") {
      storeTarget = target;
    } else if (cycle == "cure") {
      cureTarget = target;
    } else if (cycle == "dry") {
      dryTarget = target;
    }
    saveTargets();
    publishTargets();
  } else if (command == "getTargets") { // This gets called when the home target is set
    Serial.println("getTargets");
    publishTargets();
  } else if (command == "pause") { // This gets called when the cycle is paused
    state.isPlaying = false;
    publishState(true);
  } else if (command == "play") { // This gets called when the cycle is played
    if (state.timeLeft == 0) {
      resetTimeLeft();
    }
    state.isPlaying = true;
    publishState(true);
  } else if (command == "restart") { // This gets called when the cycle is restarted
    state.isPlaying = true;
    resetTimeLeft();
    publishState(true);
  } 
  
}

void resetTimeLeft() {  
  if (state.cycle == store) {
    state.timeLeft = storeTarget.time * 1000;
  } else if (state.cycle == cure) {
    state.timeLeft = cureTarget.time * 1000;
  } else if (state.cycle == dry) {
    state.timeLeft = dryTarget.time * 1000;
  }
}
// This gets called when the timeLeft reaches 0
void timeLeftReachedZeroCallback() {
  // Should advance to next cycle
 if (state.cycle == dry) {
    state.cycle = cure;
    state.timeLeft = cureTarget.time * 1000;
  } else if (state.cycle == cure) {
    state.cycle = store;
    state.timeLeft = 0;
  }
  publishState(true);
}

// This gets called every 10ms
// Simulating data for now
void measureMetrics() {
  double targetTemperature = 0;
  double targetDewPoint = 0;
  if (state.cycle == store) {
    targetTemperature = storeTarget.temperature;
    targetDewPoint = storeTarget.dewPoint;
  } else if (state.cycle == cure) {
    targetTemperature = cureTarget.temperature;
    targetDewPoint = cureTarget.dewPoint;
  } else if (state.cycle == dry) {
    targetTemperature = dryTarget.temperature;
    targetDewPoint = dryTarget.dewPoint;
  }

  state.temperature = targetTemperature + (rand() % 20)/10.0;
  state.temperature = round(state.temperature * 10) / 10.0;
  state.humidity = 57.0 + (rand() % 4);
  state.dewPoint = targetDewPoint + (rand() % 20)/10.0;
  state.dewPoint = round(state.dewPoint * 10) / 10.0;
}
