# EiEiEi - Tamaguino clone

Dieses Projekt ist für das EiEiEi Oster event 2021 entstanden. Es ist ein Tamaguino Klon auf Basis des ESP32 Controllers.

## Kompilieren

Zum Kompilieren des Projekts muss zuerst die tamaguino/settings.h angepasst werden. Die WiFi Zugangsdaten sind notwendig für ein funktionierendes EiEiEi. 

Anschließend kann mit der Arduino IDE die notwendigen Bibliotheken installiert werden:
    - Adafruit GFX Library
    - Adafruit BusIO
    - Adafruit SSD1306
    - Adafruit MQTT Library
    - ArduinoUniqueID

Der Code kann dann auf das EiEiEi hochgeladen werden.

### PlatformIO / OTA

Beim ersten mal muss der Code per USB-Kabel hochgeladen werden. Danach besteht die Möglichkeit eines O(ver)T(he)A(ir)-Updates. Damit das OTA-Update funktioniert, müssen in der platformio.ini die entsprechenden Zeilen (upload_protocol, upload_port, upload_flags) einkommentiert werden.