# ESP32-CW
ESP32 software to generate CW (Morse) ; Text to be converted in Morse, is typed in an android application  using Bluetooth connection 
This text is forwarded to an ESP32 board connected to a buzzer. Any ESP32 board can be used, GPIO pins for LED and Buzzer must be modified in Arduino software

esp32_ble_X_CW.ino   : software for ESP32 DEVKIT MH-ET-LIVE board ( approx 6€ ) a buzzer is attached to GPIO pin 22 and LED pin 2 is also used (to be compiled with Arduino IDE with ESP32 support)

BLE_ESP32_CW_V1.aia : source to AppInventor2 . Can be used to modifie apk file
BLE_ESP32_CW_V1.apk : For android smartphone compatible with android 5.0 and upper

Nota: To install ESP32 support in Arduino IDE follow these instructions https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md

Many thanks to arduinoboy for his very knowledgeable tutorial about how to create a simple Android app for two-way communication between your mobile device and the ESP32 using Bluetooth Low Energy   https://www.instructables.com/id/ESP32-BLE-Android-App-Arduino-IDE-AWESOME/

Look to this tutorial to get more detailed information about Bluetooth LE code in ESP32 software and Android App code

