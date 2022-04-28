# ESP8266-Heartbeat-temp-humd

A portable IoT project with ESP8266 and Firebase

This project aims to track the heart rate of patients in one day and record the data, as well as the temperature and humidity of the room. The code provided was written in the Arduino IDE. The ESP8266 chip connected with the heartbeat sensor connected with an analog pin reads the data from the finger and sends it to a web service within a POST request through the WiFi network and the DHT11 module is connected to the D2 pin. The web service address and WiFi credentials are hard coded, so without compiling the code, these credentials and the web service address cannot be changed.
