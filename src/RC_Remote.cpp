 /*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This program represents the RC remote, it contains the setup of ESP-NOW between the remote and boat.
             It reads data from  two joysticks and two pushbuttons. This data is sent to the boat for motor, camera mount,
             and siren control. There is also a callback to receive sensor data and display it on a I2C display. Depending on
             the water conditions the RGB backlight will change colour. The second button is used to toggle between the data of
             the three sensors.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "rgb_lcd.h"

// MAC Address of Slave (Boat)
uint8_t slaveAddress[] = {0x48, 0xE7, 0x29, 0xA0, 0x6D, 0x1C}; // Replace with MAC Address of other ESP

// Joystick and button pins
const int camJoystickX = 34; 
const int camJoystickY = 35;
const int motorJoystickX = 32; 
const int motorJoystickY = 33;
const int btnHonk = 4;

int honkVal = 0;
int camXValue = 0;
int camYValue = 0; 
int motorXValue = 0;
int motorYValue = 0;

const int PIN_TDS = 36;
const int oneWireBus = 4;

// incoming sensor data
float incomingTemp = 0;
float incomingTDS = 0;
float incomingTurb = 0;

// toggle screen pin
const int btnChngScren = 13;
int chngScreenVal = 0;

// Structure for Outgoing Joystick Data
typedef struct joystickPos {
    int motorX;
    int motorY;
    int camX;
    int camY;
    int sigHonk;
} joystickPos;

// Structure for Incoming Joystick Data
typedef struct sensorReadings {
    float tempC;
    float tds;
    float turbidity;
} sensorReadings;

joystickPos joystickData;
sensorReadings sensorData;

esp_now_peer_info_t peerInfo;

// LCD counter
rgb_lcd lcd;
int lcdCounter = 1;

// Send Data to Boat
void SendToBoat(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void masterRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
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

void setup() {
    pinMode(PIN_TDS, INPUT);
    pinMode(btnChngScren, INPUT);
    pinMode(btnHonk, INPUT);

    Wire.begin(21, 22);
    lcd.begin(16, 2);

    // Init Serial Monitor
    Serial.begin(115200);
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Once ESPNow is successfully Init, we will register for Send CB to get the status of transmitted packet
    esp_now_register_send_cb(SendToBoat);
    esp_now_register_recv_cb(masterRecv);

    // Register peer
    memcpy(peerInfo.peer_addr, slaveAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
  
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }
}

// Toggle Display
void stateLCD()
{
    chngScreenVal = digitalRead(btnChngScren);
    Serial.print("LCD Value: ");
    Serial.print(chngScreenVal);

    // change lCD counter and reset
    if (chngScreenVal == 1) {
        lcdCounter++;
        if (lcdCounter > 3) {
            lcdCounter = 1;
        }
    }

    lcd.clear();

    // backlight condition for temperature   
    if(lcdCounter == 1)
    {
        
        if(incomingTemp >= 24.5)
        {
            lcd.setRGB(255, 0, 0);
        }
        else
        {
            lcd.setRGB(0, 0, 255);
        }
        lcd.setCursor(0,0);
        lcd.print("Temperature: ");
        lcd.setCursor(0,1);
        lcd.print(incomingTemp);
        lcd.print((char)0b11011111); // degree symbol
        lcd.print("C");
    }
    // backlight condition for TDS
    else if(lcdCounter == 2)
    {
        if(incomingTDS <= 100)
        {
            lcd.setRGB(0, 255, 255);
        }
        else
        {
            lcd.setRGB(255, 0, 255);
        }
        lcd.setCursor(0,0);
        lcd.print("TDS Level: ");
        lcd.setCursor(0,1);
        lcd.print(incomingTDS);
        lcd.print(" PPM");
    }
    // backlight condition for turbidity 
    else if(lcdCounter == 3)
    {
        if(incomingTurb <= 2000)
        {
            lcd.setRGB(40, 255, 40);
        }
        else
        {
            lcd.setRGB(150, 75, 0);   
        }
        lcd.setCursor(0,0);
        lcd.print("Turbidity: ");
        lcd.setCursor(0,1);
        lcd.print(incomingTurb);
        lcd.print(" NTU");
        
    }
    else if(lcdCounter == 4)
    {
        lcdCounter = 0;
    }
    delay(330);
}

void loop() {
    stateLCD();

    // Read data for joystick and honk button for sending
    camXValue = analogRead(camJoystickX);
    camYValue = analogRead(camJoystickY);
    motorXValue = analogRead(motorJoystickX);
    motorYValue = analogRead(motorJoystickY);
    honkVal = digitalRead(btnHonk);

    joystickData.camX = camXValue;
    joystickData.camY = camYValue;
    joystickData.motorX = motorXValue;
    joystickData.motorY = motorYValue;
    joystickData.sigHonk = honkVal;

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(slaveAddress, (uint8_t *) &joystickData, sizeof(joystickData));
  
    lcd.setCursor(0, 1);

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    } else {
        Serial.println("Error sending the data");
    }

    /*
    Test Cases
    Serial.print("Cam Joy X: ");
    Serial.print(camXValue);
    Serial.print(" | Cam Joy Y: ");
    Serial.println(camYValue);

    Serial.print("Motor Joy X: ");
    Serial.print(motorXValue);
    Serial.print("Motor Joy | Y: ");
    Serial.println(motorYValue);

    Serial.print("Honk State: ");
    Serial.println(honkVal);

    Serial.println(WiFi.macAddress());
    */

}
