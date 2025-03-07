// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>
#include <ArduinoJson.h>

#define MQTT_PUBLISH_INTERVAL_MS 10 // in seconds

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

enum Cycle {
    store,
    dry,
    cure
};

enum StepMode {
    step,
    slope
};

class State {
public:
    bool isPlaying = false;
    double temperature = 0;
    double humidity = 0;
    double dewPoint = 0;
    unsigned long timeLeft = 0; // in milliseconds
    Cycle cycle = store;
    StepMode stepMode = step;
};

extern State state;

class Target {
public:
    double temperature = 0;
    double dewPoint = 0;
    unsigned long time = 0; // in seconds
    StepMode stepMode = step;
};

extern Target storeTarget;
extern Target cureTarget;
extern Target dryTarget;

void setupMQTT();
Cycle stringToCycle(String cycle);
String cycleToString(Cycle cycle);
StepMode stringToStepMode(String stepMode);
String stepModeToString(StepMode stepMode);
void publishState();
void commandCallback(StaticJsonDocument<200> doc);
void timeLeftReachedZeroCallback();
void measureMetrics();
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
void saveState();
void loadState();
#endif // CONFIG_H