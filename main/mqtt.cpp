#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"
#include <ArduinoJson.h>

// MQTT topic for IR sensor
const char* topic_publish_state = "state";
const char* topic_publish_targets = "targets";
const char* topic_subscribe_command = "command";

// MQTT thread parameters
TaskHandle_t mqttTaskHandle = NULL;

// Publishing control
unsigned long lastPublishTime = 0;
unsigned long lastRunTime = 0;

void mqttTask(void *parameter);

// Create instances
WiFiClientSecure mqttSecureClient;
PubSubClient mqttClient(mqttSecureClient);

Cycle stringToCycle(String cycle) {
  if (cycle == "store") return store;
  if (cycle == "dry") return dry;
  if (cycle == "cure") return cure;
  return store;
}
String cycleToString(Cycle cycle) {
  if (cycle == store) return "store";
  if (cycle == dry) return "dry";
  if (cycle == cure) return "cure";
  return "store";
}

String stepModeToString(StepMode stepMode) {
  if (stepMode == step) return "step";
  if (stepMode == slope) return "slope";
  return "step";
}

StepMode stringToStepMode(String stepMode) {
  if (stepMode == "step") return step;
  if (stepMode == "slope") return slope;
  return step;
}

void publishState()
{
  StaticJsonDocument<200> doc;
  doc["temperature"] = state.temperature;
  doc["humidity"] = state.humidity;
  doc["dewPoint"] = state.dewPoint;
  doc["isPlaying"] = state.isPlaying;
  doc["timeLeft"] = state.timeLeft/1000;
  doc["cycle"] = cycleToString(state.cycle);
  // Serialize to string
  String messagePayload;
  serializeJson(doc, messagePayload);

  Serial.println(messagePayload);

  // Publish with QoS 1 to deviceId()/temperature
    String publishTopic = getOwnerId() + "/" + getDeviceId() + "/" + topic_publish_state;
  bool published = mqttClient.publish(publishTopic.c_str(), messagePayload.c_str(), true);

  if (!published) {
    Serial.println("Failed to publish message");
  } else {
    // Serial.println("Message published successfully");
  }
  lastPublishTime = millis(); 
}

void publishTargets()
{
  StaticJsonDocument<200> doc;
  doc["stemp"] = storeTarget.temperature;
  doc["sdp"] = storeTarget.dewPoint;
  doc["stime"] = storeTarget.time;
  doc["ssm"] = stepModeToString(storeTarget.stepMode);
  doc["ctemp"] = cureTarget.temperature;
  doc["cdp"] = cureTarget.dewPoint;
  doc["ctime"] = cureTarget.time;
  doc["csm"] = stepModeToString(cureTarget.stepMode);
  doc["dtemp"] = dryTarget.temperature;
  doc["ddp"] = dryTarget.dewPoint;
  doc["dtime"] = dryTarget.time;
  doc["dsm"] = stepModeToString(dryTarget.stepMode);
  // Serialize to string
  String messagePayload;
  serializeJson(doc, messagePayload);

  Serial.println(messagePayload);

  // Publish with QoS 1 to deviceId()/temperature
  String publishTopic = getOwnerId() + "/" + getDeviceId() + "/" + topic_publish_targets;
  bool published = mqttClient.publish(publishTopic.c_str(), messagePayload.c_str(), true);

  if (!published) {
    Serial.println("Failed to publish message");
  } else {
    Serial.println("Targets published successfully");
  }
  lastPublishTime = millis(); 
}

// Callback function for MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(message);
  
  // Check if it's the command topic
  String topicStr = String(topic);
  if (topicStr.endsWith(topic_subscribe_command)) {
    // parse {"command":"advanceCycle","cycle":"dry"}
    StaticJsonDocument<200> doc;
    deserializeJson(doc, message);
    commandCallback(doc);
  }
}

void setupMQTT() {
  mqttSecureClient.setCACert(mqtt_root_ca);
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(callback); // Set the callback function
  // Start MQTT task
  xTaskCreatePinnedToCore(
    mqttTask,          // Function to implement the task
    "MQTTTask",        // Name of the task
    8192,   // Stack size in words
    NULL,              // Task input parameter
    1,// Priority of the task
    &mqttTaskHandle,   // Task handle
    0     // // Use core 0 for MQTT (core 1 is typically used for the Arduino loop)
  );
  
  Serial.println("MQTT task started");
}

void reconnect() {
  // Only attempt to connect if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping MQTT connection attempt.");
    return;
  }

  if (ownerIdIsNone()) {
    Serial.println("Owner Id Not Set. Skipping MQTT connection attempt.");
    return;
  }
  
  Serial.println("WiFi connected and Owner Id Set. Connecting to MQTT Broker...");
  
  // Set a connection timeout - don't get stuck in an infinite loop
  unsigned long connectionStartTime = millis();
  const unsigned long connectionTimeout = 15000; // 15 seconds timeout
  
  while (!mqttClient.connected() && (millis() - connectionStartTime < connectionTimeout)) {
    Serial.println("Attempting MQTT connection...");
    char clientId[50];
    uint64_t chipId = ESP.getEfuseMac();
    snprintf(clientId, sizeof(clientId), "ESP32-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    
    if (mqttClient.connect(clientId, mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT Broker.");
      
      // Subscribe to command topic with device ID prefix
      String ownerId = getOwnerId();
      if (ownerId == "null") {
        ownerId = "Unregistered";
      }
      String subscriptionTopic = ownerId + "/" + getDeviceId() + "/" + topic_subscribe_command;
      mqttClient.subscribe(subscriptionTopic.c_str());
      Serial.print("Subscribed to: ");
      Serial.println(subscriptionTopic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" trying again in 2 seconds");
      delay(2000);
    }
  }
  
  if (!mqttClient.connected()) {
    Serial.println("MQTT connection timeout. Will try again later.");
  }
}

// MQTT task function that runs on its own core
void mqttTask(void *parameter) {
  Serial.println("MQTT Task started");
  
  // Run indefinitely
  for (;;) {
    if((state.cycle == cure || state.cycle == dry) && state.isPlaying && state.timeLeft > 0) {
      unsigned long timeElapsed = millis() - lastRunTime;
      if (timeElapsed > state.timeLeft) {
        timeElapsed = state.timeLeft;
      }
      state.timeLeft = state.timeLeft - timeElapsed;
      if (state.timeLeft == 0) {
        timeLeftReachedZeroCallback();
      }
    }
    lastRunTime = millis();
    measureMetrics();
    // Check WiFi and MQTT connection
    if (WiFi.status() == WL_CONNECTED && !ownerIdIsNone()) {
      if (!mqttClient.connected()) {
        reconnect();
      }
      
      if (mqttClient.connected()) {
        // This is where the callback happens - inside mqttClient.loop()
        mqttClient.loop();
        
        unsigned long currentTime = millis();
        
        if (currentTime - lastPublishTime >= MQTT_PUBLISH_INTERVAL_MS * 1000) {
          // Create a JSON document with timestamp for security
          publishState();
        }
      }
    } else {
      Serial.println("WiFi disconnected. Waiting for reconnection...");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    
    // Short delay for task yield - prevents watchdog timer issues
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Function to stop the MQTT task if needed
void stopMQTTTask() {
  if (mqttTaskHandle != NULL) {
    vTaskDelete(mqttTaskHandle);
    mqttTaskHandle = NULL;
    Serial.println("MQTT task stopped");
  }
}