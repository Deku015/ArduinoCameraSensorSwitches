# ArduinoCameraSensorSwitches
Final Project from my stay at Cal State Fullerton, mix of hardware and software to make local area home automation system.
CameraWebServer - ESP32Cam + powersupply - Allows interaction via http, check serial output for ip address
ESP32Switches_490 - ESP32 + motor + relay + powersupply - Allows via http to access camera web server via button, turn on and off a relay switch with 5 V motor attached
ESP32DistanceEmail - ESP32 + ultrasonic sensor + powersupply - Allows emails to be sent when ultrasonic sensor is tripped (<10 inches about), email sent via SMTP to designated email
ESP32Camera_Ultrasonic_Sensor - ESP32 + ultrasonic sensor + ESP32CAM + power supply - Ultrasonic sensor triggered camera shot that sends to the user via an urgent email using SMTP

These devices provide a security network of home IOT devices that are simply managed on home private networks and communicated with via HTTP and SMTP
