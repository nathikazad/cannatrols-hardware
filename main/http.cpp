#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool askServerForMachineOwnerId(String machineId, String &ownerId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot fetch owner ID.");
    return false;
  }

  HTTPClient http;
  http.begin("https://edlquuxypulyedwgweai.supabase.co/functions/v1/get-machine-owner-id");
  
  // Add headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", "493ac3f1-18a3-4d63-829b-1ae37b3062dc");
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["machineId"] = machineId;
  
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
        return true;
      } else {
        Serial.println("API reported failure");
        preferences.begin("user", false);
        preferences.remove("id");
        preferences.end();
        return false;
      }
      
    } else {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
      return false;
    }
  } else {
    Serial.println("HTTP Error: " + String(httpResponseCode));
    return false;
  }
  
  http.end();
  return false;
}