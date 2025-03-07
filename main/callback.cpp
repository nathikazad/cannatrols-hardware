#include "config.h"
#include <ArduinoJson.h>

// This gets called when a command is received from the mobile app
void commandCallback(StaticJsonDocument<200> doc) {
  String command = doc["command"];
  String cycle = doc["cycle"];
  if (command == "advanceCycle") { // This gets called when the cycle is advanced
    state.cycle = stringToCycle(cycle);
    state.timeLeft = 0;
    state.targetTime = 0;
    saveState();
  } else if (command == "setTargets") { // This gets called when the targets are set
    state.targetTemperature = round(doc["targetTemperature"].as<float>() * 10) / 10.0;
    state.targetDewPoint = round(doc["targetDewPoint"].as<float>() * 10) / 10.0;
    state.targetTime = doc["targetTime"];
    state.stepMode = stringToStepMode(doc["stepMode"]);
    saveState();
  } else if (command == "pause") { // This gets called when the cycle is paused
    state.isPlaying = false;
  } else if (command == "play") { // This gets called when the cycle is played
    if (state.timeLeft == 0) {
      state.timeLeft = state.targetTime * 1000;
    }
    state.isPlaying = true;
  } else if (command == "restart") { // This gets called when the cycle is restarted
    state.isPlaying = true;
    state.timeLeft = state.targetTime * 1000;
  } 
  publishState();
}

// This gets called when the timeLeft reaches 0
void timeLeftReachedZeroCallback() {
  state.isPlaying = false;
  // Should advance to next cycle?
}

// This gets called every 10ms
void measureMetrics() {
  // Simulating data for now
  state.temperature = state.targetTemperature + (rand() % 20)/10.0;
  state.humidity = 57.0 + (rand() % 4);
  state.dewPoint = state.targetDewPoint + (rand() % 20)/10.0;
}
