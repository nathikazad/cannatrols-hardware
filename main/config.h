// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>

extern bool deviceConnected;
extern bool isAuthenticated;
extern Preferences preferences;

extern const char* mqtt_broker;
extern const int mqtt_port;
extern const char* mqtt_username;
extern const char* mqtt_password;
extern const char* server_api_key;
extern const char* mqtt_root_ca;
extern const char* supabase_root_ca;

void setupMQTT();
  
void setupBLE();
void sendBleData(String data);
void bleDataReceiveCallback(String receivedData); 

bool askServerForDeviceInfo(String machineId, String &ownerId, String &deviceName);

String getDeviceId();
bool getWiFiCredentials(String &ssid, String &password);
void saveWiFiCredentials(const char* ssid, const char* password);
void saveDeviceName(const String &deviceName);
String getDeviceName();
String getOwnerId();
void saveOwnerId(const String &ownerId);
bool ownerIdIsNone();
#endif // CONFIG_H