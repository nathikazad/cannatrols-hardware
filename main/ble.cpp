// ble.cpp
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"

// UART service UUID
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Device name and manufacturer data
#define MANUFACTURER_ID 0x02A5  // Manufacturer ID
#define MANUFACTURER_DATA "CANN" // Company identifier

// Handshake settings
const char* SECRET_KEY = "MySecretKey123";
const char* CHALLENGE_PREFIX = "CHALLENGE:";
const char* RESPONSE_PREFIX = "RESPONSE:";
const char* AUTH_SUCCESS = "AUTH_OK";
const char* AUTH_FAILED = "AUTH_FAILED";

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isAuthenticated = false;
char lastChallenge[17];

void generateChallenge(char* challenge, size_t length) {
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  
  // Better randomization by mixing analog input with system time
  randomSeed(analogRead(0) + millis());
  
  for (size_t i = 0; i < length - 1; i++) {
    int index = random(0, sizeof(charset) - 2); // -2 because of null terminator
    challenge[i] = charset[index];
  }
  challenge[length - 1] = '\0'; // Ensure null termination
}

String hash(String plainText) {
  String response = "";
    // Simple hash algorithm to make it non-trivial to reverse
  for (size_t i = 0; i < plainText.length(); i++) {
    byte value = plainText.charAt(i);
    
    // Mix in the secret key
    value ^= SECRET_KEY[i % strlen(SECRET_KEY)];
    
    // Add some bit rotation for extra complexity
    value = (value << 2) | (value >> 6); // Rotate 2 bits left
    
    // Convert to hexadecimal (2 chars per byte)
    if (value < 16) response += "0";
    response += String(value, HEX);
  }
  return response;
}

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      isAuthenticated = false;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      isAuthenticated = false;
      Serial.println("Device disconnected");
      delay(500); // Give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // Restart advertising
      Serial.println("Start advertising");
    }
};

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* rxValue = pCharacteristic->getData();
      int len = pCharacteristic->getLength();
      
      if (len > 0) {
        String receivedData = String((char*)rxValue, len);
        receivedData.trim();
        Serial.println("Received from BLE: " + receivedData);
        
        if (!isAuthenticated) {
          if (receivedData == "REQUEST_CHALLENGE") {
            generateChallenge(lastChallenge, sizeof(lastChallenge));
            String challenge = String(CHALLENGE_PREFIX) + String(lastChallenge);
            pTxCharacteristic->setValue((uint8_t*)challenge.c_str(), challenge.length());
            pTxCharacteristic->notify();
            Serial.println("Challenge sent on request: " + challenge);
            return;
          } else if (receivedData.startsWith(RESPONSE_PREFIX)) {
            // Verify response
            String receivedResponse = receivedData.substring(strlen(RESPONSE_PREFIX));
            String combined = String(lastChallenge) + String(SECRET_KEY);
            String expectedResponse = hash(combined);
            Serial.println(receivedResponse + " "+ expectedResponse);
            receivedResponse.trim();
            expectedResponse.trim();
            if (receivedResponse.equals(expectedResponse)) {
              isAuthenticated = true;
              pTxCharacteristic->setValue((uint8_t*)AUTH_SUCCESS, strlen(AUTH_SUCCESS));
              pTxCharacteristic->notify();
              Serial.println("Device authenticated successfully");
            } else {
              pTxCharacteristic->setValue((uint8_t*)AUTH_FAILED, strlen(AUTH_FAILED));
              pTxCharacteristic->notify();
              Serial.println("Authentication failed");
              
              // Generate new challenge
              generateChallenge(lastChallenge, sizeof(lastChallenge));
              String newChallenge = String(CHALLENGE_PREFIX) + String(lastChallenge);
              pTxCharacteristic->setValue((uint8_t*)newChallenge.c_str(), newChallenge.length());
              pTxCharacteristic->notify();
            }
          }
        } else {
          // Normal communication when authenticated
          // Echo received data
          bleDataReceiveCallback(receivedData);
          // String reply = "Echo: " + receivedData;
          // pTxCharacteristic->setValue((uint8_t*)reply.c_str(), reply.length());
          // pTxCharacteristic->notify();
        }
      }
    }
};

void setupBLE() {
    // Setup BLE after WiFi to ensure proper resource allocation
  String deviceName = getDeviceName();
  randomSeed(analogRead(0));

  // Create the BLE Device
  BLEDevice::init(deviceName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                       BLECharacteristic::PROPERTY_WRITE
                     );
  pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());

  // Start the service
  pService->start();

  // Start advertising with manufacturer data
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  // Create manufacturer data as a String
  char manData[20];
  uint8_t* pData = (uint8_t*)manData;
  pData[0] = MANUFACTURER_ID & 0xFF;
  pData[1] = (MANUFACTURER_ID >> 8) & 0xFF;
  strcpy(manData + 2, MANUFACTURER_DATA);
  String manufacturerData = String(manData);
  
  BLEAdvertisementData advertisementData;
  advertisementData.setManufacturerData(manufacturerData);

  BLEUUID deviceIdServiceUuid("FFF1");
  advertisementData.setServiceData(deviceIdServiceUuid, getDeviceId());

  pAdvertising->setAdvertisementData(advertisementData);
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE System Ready: "+ deviceName +" "+ getDeviceId());
  Serial.println("Waiting for client connection...");
}


void sendBleData(String data) {
  pTxCharacteristic->setValue((uint8_t*)data.c_str(), data.length());
  pTxCharacteristic->notify();
}