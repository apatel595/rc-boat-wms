# rc-boat-wms

## Description:
Captsone Project (May 6 - Aug 16, 2024)

This project uses 3 ESPs (Boat, Remote, and Hub) to create a RC boat with a water monitoring system. The boat houses three water sensors; DS18B20 temperature sensor, TDS (Total Dissolved Solids), and Turbidity. The boat and remote use a two-way bluetooth communication protocol called ESP-NOW for data transfer. The onboard sensor data is sent to the remote and displayed on an RGB LCD through I2C commmunication. The remote is used to control the two propellers via an L293D motor driver. The remote features two joysticks to control the two DC motors and also a camera mount which utilze of two servos (pan and tilt). The remote can also trigger a honking mechanism. This setup involves a relay and 12V siren on the boat. The display data can be toggled between the three different sensors and the backlight will change depending on certain water conditions. The hub ESP will post data to a webserver. A frontend application is used to update the webserver every 5 seconds and the backend logs data into the MongoDB compass platform.

## Hardware Schematics

Remote Schematic:

![image](https://github.com/user-attachments/assets/23ca9b3f-62f8-4fd0-9528-42b5a6983b8c)

Boat Schematic:

![image](https://github.com/user-attachments/assets/cbafa9e3-0821-48f6-a6b4-ede573a88a19)


