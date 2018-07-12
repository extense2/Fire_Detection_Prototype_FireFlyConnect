Description:
   This is a fire monitoring system using the following hardware components:
   - one Wemos D1 microcontroller 
   - one i2c Sparkfun "SparkX" GridEYE sensor (based on the Panasonic AMG88) 
   - one i2c atmospheric sensor (Adafruit BME280)
   - one i2c RGB sensor (Adafruit TCS34725)
   - a generic MQ-2 based analog gas/smoke sensor
   - a generic digital PIR motion sensor
   
Operation:   
   The hardware is configured to upload the sensor data (formatted as a JSON string) by sending an HTTP GET request to a Google Script at a fixed interval. 
   The Google Script handles the incoming data and formats/prints it in the Sheet specified by "GScriptId".
   The GridEYE sensor produces an 8*8 pixel grid of temperature values. 
   The user can choose to upload all 64 values, or a summary containing the hottest pixels in each row.
   

Important notes:
   - The Wemos board must be in "Boot Sketch mode" by connecting Pins D3 and D4 to 3V3, and pin D8 to 0V. 
      Otherwise the sketch may not upload properly. See: https://github.com/esp8266/Arduino/issues/1017#issuecomment-156689684
   - This uses an old version of the HTTPSRedirect library. 
      See: http://embedded-lab.com/blog/post-data-google-sheets-using-esp8266/#comment-2331802
   
Connections: 
   The i2c sensors' SDA and SCL pins are wired to the Wemos SDA/D2 and SCL/D1 pins respectively.
   The SparkX GridEYE must be powered from the 3.3V bus. All the other sensors are 5V compatible.
   The gas sensor's VCC and GND pins are connected to 5V and GND respectively. Its output is wired to A0.
   The PIR sensor's VCC and GND pins are connected to 5V and GND respectively. Its output is wired to D5.
   
Compiled: 
   19 June 2018 on WeMos D1 R2. 80MHz, 4M (3M SPIFFS), v2 Lower Memory, Disabled, None, Only Sketch, 921600.
   
Author:   
   Kevin Too

