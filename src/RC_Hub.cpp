/*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This program represents the RC hub, it serves as a hub for all sensor data from the boat. and sends 
             to a web server for live monitoring and future processing. 
             
             Why was a third ESP added?
             Due to limitations with ESP-NOW when implementing Wi-FI for IoT purposes a race condition occurs between
             Wi-Fi and I2C used on the remote. Due to time constraints, a simple solution was to add a third ESP32
             and use only one way communication to send the sensor data to the third ESP and post that to a web server

             Topology Diagram:
             Hub  <----- Boat <-----> Remote

*/

#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Network credentials
const char* ssid = "Enter SSID HERE";
const char* password = "ENTER Password HERE";

const char* serverUrl = "http://IP:Port/sensors";

float incomingTemp = 0.0;
float incomingTDS = 0.0;
float incomingTurb = 0.0;

// Callback for boat sensor data
typedef struct sensorReadings {
    float tempC;
    float tds;
    float turbidity;
} sensorReadings;

sensorReadings sensorData;

// Read sensor data from Boat
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&sensorData, incomingData, sizeof(sensorData));
    incomingTemp = sensorData.tempC;
    incomingTDS = sensorData.tds;
    incomingTurb = sensorData.turbidity;

    /*
    Test Cases
    Serial.print("Received Temp: ");
    Serial.println(incomingTemp);
    Serial.print("Received TDS: ");
    Serial.println(incomingTDS);
    Serial.print("Received Turbidity: ");
    Serial.println(incomingTurb);
    */
}

// Posts to Webserver
void sendSensorData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        // This will store all sensor data in one json payload
        String jsonPayload = "{\"tempC\":" + String(incomingTemp) + 
                             ",\"tds\":" + String(incomingTDS) +
                             ",\"turbidity\":" + String(incomingTurb) + "}";

        int httpResponseCode = http.POST(jsonPayload); // send request

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response Code: " + String(httpResponseCode));
            Serial.println("Response: " + response);
        } else {
            Serial.println("Error on sending POST: " + String(httpResponseCode));
        }

        http.end(); // free resources
    }
}


void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi in STA mode
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Sends Every 5 Seconds
void loop() {
    static unsigned long lastSendTime = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastSendTime >= 5000) { 
        sendSensorData();
        lastSendTime = currentMillis;
    }
}
