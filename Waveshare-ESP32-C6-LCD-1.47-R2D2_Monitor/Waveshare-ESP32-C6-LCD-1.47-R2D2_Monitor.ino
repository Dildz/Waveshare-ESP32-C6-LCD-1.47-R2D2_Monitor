/*********************************************************************************************************
* Waveshare ESP32-C6-LCD-1.47 Internet Clock Project
*
* Description:
* This code displays temperature & humidity with Wi-Fi connectivity on the Waveshare ESP32-C6 1.47" LCD.
* The project combines real-time weather data, a countdown timer, and Wi-Fi monitoring. The UI was created
* with Squareline Studio and features OpenWeatherMap integration, hex countdown display, and visual
* indicators for Wi-Fi strength.
*
* Key Features:
*  - Temperature & humidity data from OpenWeatherMap (updated every 10 minutes)
*  - 10-minute countdown timer displayed in hex
*  - Seconds counter using millis
*  - LVGL-based graphical interface with FPS counter
*  - WiFi status monitoring with colour-coded signal strength (RSSI)
*  - IP address display
*  - Synchronized NeoPixel LED and on-screen R2D2-LED indicator
*
* How It Works:
*  1. Initialization: Sets up display, LVGL, and WiFi connection
*  2. Weather Updates: Fetches current conditions from OpenWeatherMap API
*  3. Countdown Timer: 10-minute countdown displayed in hexadecimal format
*  4. System Monitoring: Continuously updates WiFi status, FPS, and IP
*  5. LED Synchronization: Matches NeoPixel LED color with on-screen display
*
* Notes:
*  - Requires OpenWeatherMap API key (free tier available)
*  - Edit the Wi-Fi SSID, password, city and country code in configuration section
*  - City names must be URL encoded (spaces as %20)
*  - Uses metric units for temperature
*  - Backlight brightness is set to 75% by default
*  - Had to edit line 4 to [#include "lv_conf.h"] and comment out line 5 in LVGL_Driver.h
*  - Exposed NeoPixel RGB values in NeoPixel.cpp & inverted RGB channels for R2D2-LED to match colours
*  - SLS export path will need to get updated in the project settings
*  - When exporting in SLS & replacing project files, you NEED to edit the ui.h file on line 30:
*      #include "screens/ui_MainScreen.h"
*      >TO<
*      #include "ui_MainScreen.h"
*      (Remove 'screens/' - not sure why SLS is exporting this way as flat export option is selected.)
* 
**********************************************************************************************************/

/*************************************************************
******************* INCLUDES & DEFINITIONS *******************
**************************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "NeoPixel.h"
#include "ui.h"

/******************************
*** >> EDIT THIS SECTION << ***
*******************************/
const char* ssid = "YOUR_SSID";                // replace with your Wi-Fi SSID name
const char* password = "YOUR_PASSWORD";        // replace with your Wi-Fi password
const char* owmAPI = "YOUR_OWM_API_KEY";       // replace with your OWM API key
const char* city = "YOUR_CITY_NAME";           // URL encoded space - replace with your city/town
const char* countryCode = "YOUR_COUNTRY_CODE"; // replace with your country code

// External variables from NeoPixel.cpp
extern uint8_t currentRed;
extern uint8_t currentGreen;
extern uint8_t currentBlue;

// Global Variables
unsigned long lastWiFiUpdate = 0;
const int wifiUpdateInterval = 5000; // 5 seconds
String ipAddress;

const long updateInterval = 600000; // 10min in milliseconds
unsigned long lastUpdateTime = 0;

float temp = 0.0;
int humi = 0;
String hexCount = "0x000";

// Countdown timer variables
unsigned long countdownStart = 600; // 600 seconds = 10 minutes
unsigned long remainingSeconds = countdownStart;
unsigned long lastCountdownUpdate = 0;

// FPS tracking
unsigned long lastFPSTime = 0;
unsigned int frameCount = 0;
int currentFPS = 0;

// Seconds tracking
unsigned long lastSecondUpdate = 0;
int currentSeconds = 0;


/*************************************************************
********************** HELPER FUNCTIONS **********************
**************************************************************/

// Function to connect to Wi-Fi
void connectWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  ipAddress = WiFi.localIP().toString();
}

// Function to update Open Weather Map data
bool updateOWM() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "," + String(countryCode) + "&units=metric&appid=" + String(owmAPI);
  
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      http.end();
      return false;
    }

    temp = doc["main"]["temp"];
    humi = doc["main"]["humidity"];
    http.end();

    // Update temperature (round to 2 decimal places)
    _ui_label_set_property(ui_Temp, _UI_LABEL_PROPERTY_TEXT, String(temp, 2).c_str());
  
    // Update humidity
    _ui_label_set_property(ui_Humidity, _UI_LABEL_PROPERTY_TEXT, String(humi).c_str());

    return true;
  }
  http.end();
  _ui_label_set_property(ui_Temp, _UI_LABEL_PROPERTY_TEXT, "00.00"); // set temp to 00.00
  _ui_label_set_property(ui_Humidity, _UI_LABEL_PROPERTY_TEXT, "00"); // set humidity to 00
  return false;
}

// Function to update WiFi quality indicator
void updateWiFiQuality() {
  lv_style_selector_t selector = LV_STATE_DEFAULT;
  
  int rssi = WiFi.RSSI();

  if (WiFi.status() != WL_CONNECTED) {
    // Not connected - grey
    lv_obj_set_style_bg_color(ui_PanelWiFiQuality, lv_color_hex(0x808080), selector);
    _ui_label_set_property(ui_RSSI, _UI_LABEL_PROPERTY_TEXT, "N/C"); // Not Connected
    return;
  }

  // Update the RSSI strength value with dBm unit
  _ui_label_set_property(ui_RSSI, _UI_LABEL_PROPERTY_TEXT, (String(rssi) + "dBm").c_str());

  // Update the RSSI indicator circle colours
  if (rssi >= -50) {
    lv_obj_set_style_bg_color(ui_PanelWiFiQuality, lv_color_hex(0x41C241), selector); // excellent - green
  }
  else if (rssi >= -60) {
    lv_obj_set_style_bg_color(ui_PanelWiFiQuality, lv_color_hex(0xACC241), selector); // good - light green
  }
  else if (rssi >= -70) {
    lv_obj_set_style_bg_color(ui_PanelWiFiQuality, lv_color_hex(0xBC9534), selector); // fair - yellow
  }
  else {
    lv_obj_set_style_bg_color(ui_PanelWiFiQuality, lv_color_hex(0xCE2C2C), selector); // poor - red
  }
}

// Function to update countdown timer and hex display
void updateCountdown() {
  if (millis() - lastCountdownUpdate >= 1000) { // Update every second
    if (remainingSeconds > 0) {
      remainingSeconds--;
      
      // Convert remaining seconds to 3-digit hex string
      char hexStr[7]; // "0x000" + null terminator
      snprintf(hexStr, sizeof(hexStr), "0x%03X", (int)remainingSeconds);
      hexCount = String(hexStr);
      
      _ui_label_set_property(ui_LabelHexTime, _UI_LABEL_PROPERTY_TEXT, hexCount.c_str());
    }
    else {
      // Reset counter
      remainingSeconds = countdownStart;
    }
    lastCountdownUpdate = millis();
  }
}

// Function to update seconds display using millis()
void updateSeconds() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSecondUpdate >= 1000) {
    currentSeconds = (currentSeconds + 1) % 60;
    
    // Format seconds with leading zero
    char secStr[3];
    snprintf(secStr, sizeof(secStr), "%02d", currentSeconds);
    _ui_label_set_property(ui_sec, _UI_LABEL_PROPERTY_TEXT, secStr);
    
    lastSecondUpdate = currentMillis;
  }
}

// Function to update the R2D2 LED colour
void updateR2D2LED() {
  lv_style_selector_t selector = LV_STATE_DEFAULT;
  static uint8_t lastRed = 0, lastGreen = 0, lastBlue = 0;
  
  // Only change colour if the NeoPixel colour has changed
  if(currentRed != lastRed || currentGreen != lastGreen || currentBlue != lastBlue) {
    // Convert NeoPixel RGB values to LVGL colour, swapping red and green channels
    lv_color_t led_color = lv_color_make(currentGreen, currentRed, currentBlue);
    
    // Set the background colour of the R2D2 LED panel
    lv_obj_set_style_bg_color(ui_R2D2LED, led_color, selector);
    
    // Update the last colours
    lastRed = currentRed;
    lastGreen = currentGreen;
    lastBlue = currentBlue;
  }
}

/*************************************************************
*********************** MAIN FUNCTIONS ***********************
**************************************************************/

// SETUP
void setup() {
  LCD_Init();
  Lvgl_Init();
  Set_Backlight(75);
  
  ui_init();
  
  // Connect to WiFi and get first weather data
  connectWifi();
  if(updateOWM()) {
    lastUpdateTime = millis();
  }
  
  // Update IP Address
  _ui_label_set_property(ui_IPadd, _UI_LABEL_PROPERTY_TEXT, ipAddress.c_str());
  
  // Initialize countdown display
  updateCountdown();

  // Initialize R2D2 LED to match initial NeoPixel colour
  Set_Color(200, 200, 2);
  updateR2D2LED();
}

// MAIN LOOP
void loop() {
  Timer_Loop(); // call lv_timer_handler & 5ms delay via LVGL Driver

  // FPS calculation (runs every second)
  frameCount++;
  if (millis() - lastFPSTime >= 1000) {
    currentFPS = frameCount / ((millis() - lastFPSTime) / 1000);
    frameCount = 0;
    lastFPSTime = millis();
    
    // Update FPS display
    _ui_label_set_property(ui_FPS, _UI_LABEL_PROPERTY_TEXT, String(currentFPS).c_str());
  }
  
  // Update seconds display
  updateSeconds();
  
  // Update countdown timer
  updateCountdown();
  
  // Update weather data every 10min
  if (millis() - lastUpdateTime >= updateInterval) {
    if(updateOWM()) {
      lastUpdateTime = millis();
    }
  }

  // WiFi and IP updates (runs every 5 seconds)
  if (millis() - lastWiFiUpdate >= wifiUpdateInterval) {
    updateWiFiQuality();
    
    // Only update IP if it's changed (Wi-Fi reconnected)
    String newIP = WiFi.localIP().toString();
    if (newIP != ipAddress) {
      ipAddress = newIP;
      _ui_label_set_property(ui_IPadd, _UI_LABEL_PROPERTY_TEXT, ipAddress.c_str());
    }
    
    lastWiFiUpdate = millis();
  }

  // Handle the NeoPixel LED
  NeoPixel_Loop(3);
  updateR2D2LED();
}