// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>

extern bool deviceConnected;
extern bool isAuthenticated;
extern Preferences preferences;

String getDeviceId();
void setupBLE();
void sendBleData(String data);
void bleDataReceiveCallback(String receivedData); 
bool askServerForMachineOwnerId(String machineId, String &ownerId);
#endif // CONFIG_H