#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
// http.cpp
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"


bool askServerForMachineOwnerId(String machineId, String &ownerId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot fetch owner ID.");
    return false;
  }

  WiFiClientSecure client;
  client.setCACert(supabase_root_ca);

  HTTPClient http;
  http.begin(client, "https://edlquuxypulyedwgweai.supabase.co/functions/v1/get-machine-owner-id");
  
  // Add headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", server_api_key);
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["machineId"] = machineId;
  doc["timestamp"] = millis();
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  // Send POST request
  int httpResponseCode = http.POST(requestBody);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
    
    // Parse JSON response
    StaticJsonDocument<512> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error) {
      bool success = responseDoc["success"];
      if (success) {
        ownerId = responseDoc["user_id"].as<String>();
        bool updated = responseDoc["updated"];
        Serial.println("Owner ID: " + ownerId + ", Updated: " + String(updated ? "true" : "false"));
        preferences.begin("user", false);
        preferences.putString("id", ownerId);
        preferences.end();
        http.end();
        return true;
      } else {
        Serial.println("API reported failure");
        preferences.begin("user", false);
        preferences.remove("id");
        preferences.end();
        http.end();
        return false;
      }
      
    } else {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
      http.end();
      return false;
    }
  } else {
    Serial.println("HTTP Error: " + String(httpResponseCode));
    http.end();
    return false;
  }
  
  http.end();
  return false;
}