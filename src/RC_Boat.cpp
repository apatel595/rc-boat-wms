/*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This program represents the RC boat, it contains the setup of ESP-NOW between the boat and remote.
             It reads data from the three sensors; DS18B20 temperature sensor, TDS (Total Dissolved Solids), and 
             turbidity sensor (water clarity) based on research and proven formulas from forums and datasheets.
             The sensor data is sent to the remote via ESP-NOW. There is also a callback to receive joystick and button data.
             from the remote. The boat has functions for DC motor and servo camera mount control. It also triggers a siren
             when the remote button is pressed.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>

// MAC Address of Master (Remote)

uint8_t masterAddress[] = {0xD8, 0x13, 0x2A, 0xC2, 0x00, 0xB0}; // Replace with MAC Address of other ESP
esp_now_peer_info_t peerInfo;

// Sensor Pins 
const int PIN_TDS = 36;
const int oneWireBus = 0;
const int PIN_TURBIDITY = 35;

float valTempC = 0.0;
float valTDS = 0.0;
float valNTU = 0.0;


// Motor Pins 
const int motor1Pin1 = 27; 
const int motor1Pin2 = 26; 
const int enable1Pin = 14;
const int motor2Pin1 = 33; 
const int motor2Pin2 = 32; 
const int enable2Pin = 25;


// Motor PWM
const int freq = 5000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 150;


// Servo Pins
const int servoXPin = 2;     
const int servoYPin = 4; 

Servo servoX;
Servo servoY;

int angleX = 90;
int angleY = 90;

// Mosfet Pin
const int mosGatePin = 15;

// Sensor Instances
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Structure for Outgoing Sensor Data
typedef struct sensorReadings
{
    float tempC;
    float tds;
    float turbidity;
} sensorReadings;

sensorReadings sensorData;

/*
Test Idea
// Array for  Average Servo Readings and Default Position
const int numServoReadings = 10; 
int readingsX[numServoReadings];
int readingsY[numServoReadings];
int readIndex = 0;

int camXPos = 90;  
int camYPos = 90; 
*/

// Incoming Controller Data
int incomingX = 0;
int incomingY = 0;
int incomingCamX = 0;
int incomingCamY = 0;
int incomingHonk = 0;

// Structure for Controller Data
typedef struct joystickPos {
    int motorX;
    int motorY;
    int camX;
    int camY;
    int sigHonk;
} joystickPos;

joystickPos joystickData;

// Reading Sensor Data
void getSensors()
{
    sensors.requestTemperatures();

    valTempC = sensors.getTempCByIndex(0);  
    
    float turbidityAnalog = analogRead(PIN_TURBIDITY);
    float turbidityVolt = turbidityAnalog * (5 / 4095.0); 
    
    if (turbidityVolt > 4.2)
    {
        valNTU = 0.00;
    }
    else if(turbidityVolt < 2.56)
    {
        valNTU = 3004.73;
    }
    else
    {
        valNTU = (-1120.4 * turbidityVolt * turbidityVolt) + (5742.3 * turbidityVolt) - 4352.9;
    }
    
    float tdsAnalog = analogRead(PIN_TDS);
    float tdsVolt = tdsAnalog * (3.3 / 4095.0); 
    float tempCoef = 1.0 + 0.02 * (valTempC - 25.0);
    float elecCond = (tdsVolt / tempCoef);

    float valTDS = (133.42 * pow(elecCond,3) - 255.86 * pow(elecCond,2) + 857.39 * elecCond) * 0.5;
    
    Serial.print(valNTU);
    //Serial.print(valNTU);
    Serial.print(" NTU \n");
    
    Serial.print("Temp: ");
    Serial.print(valTempC);
    Serial.println("ºC");
    Serial.print("TDS: ");
    Serial.print(valTDS);
    Serial.print(" PPM \n");

    // Set Data in Structure and Send 
    sensorData.tempC = valTempC;
    sensorData.tds = valTDS;
    sensorData.turbidity = valNTU;

    esp_err_t result = esp_now_send(masterAddress, (uint8_t *) &sensorData, sizeof(sensorData));
}

// L293D motor control for forward, backward, left, and right movements
void turnRight() {
    Serial.println("Turning Right");
    analogWrite(enable1Pin, dutyCycle); 
    analogWrite(enable2Pin, dutyCycle); 
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    digitalWrite(enable1Pin, HIGH); 

    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, LOW);
    digitalWrite(enable2Pin, LOW); 
 
}

void turnLeft() {
    Serial.println("Turning Left");
    analogWrite(enable1Pin, dutyCycle); 
    analogWrite(enable2Pin, dutyCycle); 
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(enable1Pin, LOW); 

    digitalWrite(motor2Pin1, HIGH);
    digitalWrite(motor2Pin2, LOW);
    digitalWrite(enable2Pin, HIGH); 
    
}

void moveForward() {
    Serial.println("Moving Forward");
    analogWrite(enable1Pin, dutyCycle); 
    analogWrite(enable2Pin, dutyCycle); 
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    digitalWrite(enable1Pin, HIGH); 

    digitalWrite(motor2Pin1, HIGH);
    digitalWrite(motor2Pin2, LOW);
    digitalWrite(enable2Pin, HIGH); 
}


void moveBackward() {
    Serial.println("Moving Backward");
    analogWrite(enable1Pin, dutyCycle); 
    analogWrite(enable2Pin, dutyCycle); 
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(enable1Pin, HIGH);

    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, HIGH);
    digitalWrite(enable2Pin, HIGH);
}

void stopMotors() {
    Serial.println("Motor stopped");
    analogWrite(enable1Pin, 0); 
    analogWrite(enable2Pin, 0); 
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(enable1Pin, LOW); 

    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, LOW);
    digitalWrite(enable2Pin, LOW); 
}

// Motor Control
void controlMotors()
{
    if (incomingX < 1000) 
    {
        moveForward();
    } 
    else if (incomingX > 3000) 
    {
        moveBackward();
    } 
    else if (incomingY < 1000) 
    {
        turnRight();
    } 
    else if (incomingY > 3000)
    {
        turnLeft();
    } else 
    {
        stopMotors();
    }
}

// Servo Control
void controlServos() 
{
    if (incomingCamX > 3000 && angleX < 175) 
    {
        angleX  += 15;
        servoX.write(angleX);    
    } 

    if (incomingCamX < 1000 && angleX > 0) 
    {
        angleX  -= 15;
        servoX.write(angleX);    
    } 

    if (incomingCamY > 3000 && angleY < 175) 
    {
        angleY  += 15;
        servoY.write(angleY);    
    } 

    if (incomingCamY < 1000 && angleY > 0) 
    {
        angleY  -= 15;
        servoY.write(angleY);    
    } 


}

// Send Data to Remote
void SendtoRemote(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&joystickData, incomingData, sizeof(joystickData));
    incomingX = joystickData.motorX;
    incomingY = joystickData.motorY;
    incomingCamX = joystickData.camX;
    incomingCamY = joystickData.camY;
    incomingHonk = joystickData.sigHonk;

    // Test Cases
    /*
    Serial.print("Received Motor X: ");
    Serial.print(incomingX);
    Serial.print(" | Received Motor Y: ");
    Serial.println(incomingY);

    Serial.print("Received Cam X: ");
    Serial.print(incomingCamX);
    Serial.print(" | Received Cam Y: ");
    Serial.println(incomingCamY);
    */

    Serial.print("Honk Signal: ");
    Serial.print(incomingHonk);

    controlMotors();
    controlServos();

    if(incomingHonk == 0)
    {
        digitalWrite(mosGatePin,HIGH);
    }
    else
    {
        digitalWrite(mosGatePin,LOW);
    }
}

void setup() {
    Serial.begin(115200);

    // Init Sensors
    sensors.begin();

    // Pin Setup
    pinMode(motor1Pin1, OUTPUT);
    pinMode(motor1Pin2, OUTPUT);
    pinMode(enable1Pin, OUTPUT);

    pinMode(motor2Pin1, OUTPUT);
    pinMode(motor2Pin2, OUTPUT);
    pinMode(enable2Pin, OUTPUT); 
    pinMode(mosGatePin,OUTPUT);

    servoX.attach(servoXPin);
    servoY.attach(servoYPin);

    servoX.write(90);
    servoY.write(90);

    digitalWrite(mosGatePin,LOW);

    // Default Stop Motors
    Serial.println("Testing DC Motor Control...");
    stopMotors();

    //  Wi-Fi Station Mode
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register Callbacks
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(SendtoRemote);

    // Set up Channel 
    memcpy(peerInfo.peer_addr, masterAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

}

void loop() {
    getSensors();
    delay(1000); 
    //Serial.println(WiFi.macAddress());
}
