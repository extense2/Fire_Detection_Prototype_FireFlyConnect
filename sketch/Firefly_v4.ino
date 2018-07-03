/* Description:
 *    This is a fire monitoring system using the following hardware components:
 *    - one Wemos D1 microcontroller 
 *    - one i2c Sparkfun "SparkX" GridEYE sensor (based on the Panasonic AMG88) 
 *    - one i2c atmospheric sensor (Adafruit BME280)
 *    - one i2c RGB sensor (Adafruit TCS34725)
 *    - a generic MQ-2 based analog gas/smoke sensor
 *    - a generic digital PIR motion sensor
 *    
 * Operation:   
 *    The hardware is configured to upload the sensor data (formatted as a JSON string) by sending an HTTP GET request to a Google Script at a fixed interval. 
 *    The Google Script handles the incoming data and formats/prints it in the Sheet specified by "GScriptId".
 *    The GridEYE sensor produces an 8*8 pixel grid of temperature values. 
 *    The user can choose to upload all 64 values, or a summary containing the hottest pixels in each row.
 *    
 * 
 * Important notes:
 *    1. The Wemos board must be in "Boot Sketch mode" by connecting Pins D3 and D4 to 3V3, and pin D8 to 0V. 
 *       Otherwise the sketch may not upload properly. See: https://github.com/esp8266/Arduino/issues/1017#issuecomment-156689684
 *    2. This uses an old version of the HTTPSRedirect library. 
 *       See: http://embedded-lab.com/blog/post-data-google-sheets-using-esp8266/#comment-2331802
 *    
 * Connections: 
 *    The i2c sensors' SDA and SCL pins are wired to the Wemos SDA/D2 and SCL/D1 pins respectively.
 *    The SparkX GridEYE must be powered from the 3.3V bus. All the other sensors are 5V compatible.
 *    The gas sensor's VCC and GND pins are connected to 5V and GND respectively. Its output is wired to A0.
 *    The PIR sensor's VCC and GND pins are connected to 5V and GND respectively. Its output is wired to D5.
 *    
 * Compiled: 
 *    19 June 2018 on WeMos D1 R2. 80MHz, 4M (3M SPIFFS), v2 Lower Memory, Disabled, None, Only Sketch, 921600.
 *    
 * Author:   
 *    Kevin Too
*/

// ================================================================================================ Libraries & Definitions
  #include <ESP8266WiFi.h>
  #include <Wire.h>
  #include <SparkFun_GridEYE_Arduino_Library.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BME280.h>
  #include "Adafruit_TCS34725.h"
  #include "HTTPSRedirect1.h"
  
  #define SERIAL_DEBUG 1
  #define GRIDEYE_RAW  1
  #define HEAT_MAP     1
  
  #if (GRIDEYE_RAW == 1)
    #define AMG_COUNT  64
  #else 
    #define AMG_COUNT  16
  #endif
  
  #define BME_COUNT    3        // BME280 produces Temperature, Pressure and Humidity reading (3 values)
  #define RGB_COUNT    6        // TCS34725 produces RGBC readings and also calculates lux and colour temp (6 values)
  #define TEMP_DELAY   10000    // How often to update Google Spreadsheet with temperature data, in milliseconds
  #define GAS_PIN_IN   A0       // Gas sensor analog output
  #define PIR_PIN      D5       // PIR sensor digital output
  #define timeout_WiFi 10000    // Retry to connect after 10s

// Default I2C addresses (already defined in libraries)
//  #define GRIDEYE_ADDR  0x69   
//  #define BME280_ADDR   0x77   
//  #define TCS34725_ADDR 0x29   
  
// ================================================================================================ Declarations

/*    WiFi setup variables    */
  const char* ssid            = "----SSID----"; 
  const char* password        = "----PASSWORD----";
  const char* GScriptId       = "AKfycbwxqesb-dBfgPk8s9MQ_Hj7-Dc0fkCajv2XWiDOgY8bN8EY_AM";
  const char* host            = "script.google.com"; 
  const char* googleRedirHost = "script.googleusercontent.com";
  const int   httpsPort       = 443; 
  String      url             = String("/macros/s/") + GScriptId + "/exec";
  HTTPSRedirect1 client(httpsPort);

  #if (GRIDEYE_RAW == 1)
    #if (HEAT_MAP == 1)
      String modeString = "?mode=2&data=";
    #else 
      String modeString = "?mode=1&data=";
    #endif
  #else
    String modeString   = "?mode=0&data=";
  #endif  

/*    WiFi GET string     */
  String s_GridEYE[AMG_COUNT];
  String s_BME[BME_COUNT];
  String s_RGB[RGB_COUNT];
  String s_Gas = "";
  String s_PIR = "";

/*   GridEYE data   */
  GridEYE grideye;
  int pixelTable[64];
  int maxTemp[8] = {0};                                         // Hottest pixel in each row of GridEYE array
  int minTemp[8]  = {255, 255, 255, 255, 255, 255, 255, 255};   // Coldest pixel in each row of GridEYE array
  int maxTempIndex[8], minTempIndex[8] = {0};                   // Index of hottest and coldest pixel in each row
  int hottestPixel, hottestPixelIndex = 0;                      // Hottest pixel in entire array (and its index)

/*   Declare remaining variables    */
  Adafruit_BME280 bme;                                                                          // Atmospheric sensor object
  float BME_data[BME_COUNT];                                                                    // Atmospheric sensor data
  Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);  // RGB sensor object
  uint16_t r, g, b, c, colorTemp, lux;                                                          // RGB sensor variables
  uint16_t RGB_data[RGB_COUNT];                                                                 // RGB sensor data
  int gasValue = 0;                                                                             // MQ-2 Gas Sensor
  int PIRValue = LOW;                                                                           // PIR motion sensor
  unsigned long timestamp = 0;                                                                   

// ================================================================================================ Setup

void setup() {
  Wire.begin();           // Start I2C communications 
  Serial.begin(115200);   // 

  sensors_initialize();
  //gas_begin();
    
  delay(1000);
  connectWiFi();

  // String arrays must be initialized for proper concatenation
  for (uint8_t i=0; i<AMG_COUNT; i++) {s_GridEYE[i] = "";}
  for (uint8_t i=0; i<BME_COUNT; i++) {s_BME[i]     = "";}
  for (uint8_t i=0; i<RGB_COUNT; i++) {s_RGB[i]     = "";}  
}

// ================================================================================================ Loop

void loop() {
  if (millis()-timestamp > TEMP_DELAY){
    readGridEYE();        // Read GridEYE sensor
    readBME();            // Read BME sensor
    readRGB();            // Read RGB sensor
    readGas();            // Read Gas sensor
    readPIR();            // Read PIR sensor
    pixelMinMax();        // Calculate maxima and indices for each row

    String finalURL = build_GETstring();    // Assemble string for GET request
    postData(finalURL);                     // Send data to Google Sheet

/* Print sensor readings on Serial monitor  */
  #if (SERIAL_DEBUG == 2)
    Serial.println();   
    Serial.println();
    Serial.println();
    printGridEYE();    
    printBME();
    printRGB();
    printGas();
    printPIR();
  #endif

    clear_GETstring();      // Clear string for next loop iteration
    timestamp = millis();    // Store timestamp for controlling upload frequency
  }
}
// ================================================================================================ 


//-------------------------------------------------------------------------------
//-----                        WiFi functions                             -----
//-------------------------------------------------------------------------------

void connectWiFi()
{
  uint32_t timestamp_WiFi = millis();
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to: "));
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis()-timestamp_WiFi > timeout_WiFi){
        WiFi.begin(ssid, password);
        timestamp_WiFi = millis();
    }
  }
  Serial.println("");
  Serial.println(F("\nWiFi connected"));  
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

  Serial.print(String("Connecting to ")); 
  Serial.println(host);

  bool flag = false; 
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort); 
    if (retval == 1) { 
      flag = true; break; 
    } else {
      Serial.println("Connection failed. Retrying…"); 
    }
  }
  // Connection Status, 1 = Connected, 0 is not. 
  Serial.println("Connection Status: " + String(client.connected())); 
  Serial.flush(); 
   
  if (!flag){ 
    Serial.print("Could not connect to server: "); 
    Serial.println(host); 
    Serial.println("Exiting…"); 
    Serial.flush(); 
    return; 
  }
    
}

//-------------------------------------------------------------------------------
//-----                        Sensor functions                             -----
//-------------------------------------------------------------------------------

void sensors_initialize(){
  grideye.begin();
  Serial.println(F("GridEYE sensor initialized.")); 
  
  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1);
  } else {
    Serial.println(F("BME280 sensor initialized."));    
  }

  if (tcs.begin()) {
    Serial.println("RGB sensor initialized.");
  } else {
    Serial.println(F("Could not find a valid RGB sensor, check wiring!"));
    while (1);
  }

  pinMode(PIR_PIN, INPUT); // for reading PIR sensor
}

void readGridEYE(){     // Populate temperature grid
  for(uint8_t i = 0; i < 64; i++){
    pixelTable[i] = grideye.getPixelTemperature(i);
  }
}

void printGridEYE(){
  for(unsigned char i = 0; i < 64; i++){  // Print raw GridEYE data
    Serial.print(pixelTable[i]);
    Serial.print('\t');
    if((i+1)%8==0){
      Serial.println();
    }
  }
  Serial.println();
}

void pixelMinMax(){
    // Find min and max values and their index
  for(uint8_t j = 0; j < 8; j++){ // row
    for(uint8_t i = 0; i < 8; i++){ // column
      if (pixelTable[i+(8*j)]>maxTemp[j]){
        maxTemp[j] = pixelTable[i+(8*j)];   // Find hottest pixel in each row
//        maxTempIndex[j] = i+(8*j);        // Record the pixel index (absolute: 0-63)
        maxTempIndex[j] = i;                // Record the pixel index (relative: 0-7)
      }
      if (pixelTable[i+(8*j)]<minTemp[j]){
        minTemp[j] = pixelTable[i+(8*j)]; // Find coolest pixel in each row
        minTempIndex[j] = i+(8*j);        // Record the pixel index (absolute: 0-63)
      }
    }
  }
}

void readBME(){
  BME_data[0] = bme.readTemperature();
  BME_data[1] = bme.readPressure();
  BME_data[2] = bme.readHumidity();
}

void printBME(){
  Serial.print(F("BME Temperature: "));
    Serial.print(BME_data[0]);
    Serial.println(F("deg C"));
  Serial.print(F("BME Pressure: "));
    Serial.print(BME_data[1]/100.0F);
    Serial.println(F("hPa"));  
  Serial.print(F("BME Humidity: "));
    Serial.print(BME_data[2]);
    Serial.println(F("%"));
  Serial.println();
}

void readRGB(){  
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  RGB_data[0]=r;
  RGB_data[1]=g;
  RGB_data[2]=b;
  RGB_data[3]=c;
  RGB_data[4]=colorTemp;
  RGB_data[5]=lux;
}

void printRGB(){
  Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
  Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
  Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
  Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
  Serial.println(" ");
}

void readGas(){
  gasValue = map(analogRead(GAS_PIN_IN),0,1023,0,100);
}

void printGas(){
  Serial.print(F("Gas Value (%): "));
  Serial.println(gasValue);
//  Serial.println();
}

void readPIR(){
  PIRValue = digitalRead(PIR_PIN);
}

void printPIR(){
  Serial.print(F("Motion detected: "));
  if (PIRValue==HIGH) {Serial.println("Yes");}
  else                {Serial.println("No");}
}

//-------------------------------------------------------------------------------
//-----                        build_GETstring                              -----
//-------------------------------------------------------------------------------

String build_GETstring(){
  String s_concat = "{";
  
  for (uint8_t i=0; i<AMG_COUNT; i++){  // Save values to string array
    #if (GRIDEYE_RAW == 1)
      s_GridEYE[i] = formatDataToJson("A"+String(i),pixelTable[i]);
      s_concat += s_GridEYE[i];                  // Concatenate into one single string    
    #else 
      if(i<8){s_GridEYE[i] = formatDataToJson("A"+String(i),maxTemp[i]);}
      else   {s_GridEYE[i] = formatDataToJson("A"+String(i-8),maxTempIndex[i-8]);}
      s_concat += s_GridEYE[i];                  // Concatenate into one single string
    #endif
    s_concat += ",";
  }

  for (uint8_t i=0; i<BME_COUNT; i++){  // Save values to string array
    s_BME[i] = formatDataToJson("B"+String(i),BME_data[i]);
    s_concat += s_BME[i];
    s_concat += ",";
  }

  for (uint8_t i=0; i<RGB_COUNT; i++){  // Save values to string array
    s_RGB[i] = formatDataToJson("C"+String(i),RGB_data[i]);
    s_concat += s_RGB[i];
    s_concat += ",";
  }

  s_Gas = formatDataToJson("D0",gasValue);
  s_concat += s_Gas;
  s_concat += ",";

  if (PIRValue==HIGH){s_PIR = formatDataToJson("E0",1);}
  else               {s_PIR = formatDataToJson("E0",0);}

  s_concat += s_PIR;
  s_concat += "}";
  
  return s_concat;
}

void clear_GETstring(){
  for(uint8_t i=0; i<8; i++){ // Clear min and max values
    maxTemp[i] = 0;
    minTemp[i] = 255;
    maxTempIndex[i] = 0;
    minTempIndex[i] = 0;
  }
//  s_concat = "";  // Clear GET string
}

//-------------------------------------------------------------------------------
//-----                        HTTPSRedirect                                -----
//-------------------------------------------------------------------------------

String formatDataToJson(String tag, int value){
  return String("\"" + tag + "\":\"" + String(value) + "\"");
}

void postData(String dataToPost){
  Serial.println("Data to post: " + dataToPost);

  if (!client.connected()){
    Serial.println("Connecting to client again…");
    client.connect(host, httpsPort);
  }
  String urlFinal = url + modeString + dataToPost;
  client.printRedir(urlFinal, host, googleRedirHost);

  Serial.println("Request complete.");
}
