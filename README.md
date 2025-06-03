# Waveshare ESP32-C6-LCD-1.47 OWM Temperature & Humidity Monitor with SquareLine Studio UI

## Description:
This code displays temperature & humidity with Wi-Fi connectivity on the Waveshare ESP32-C6 1.47" LCD in portrait mode. The project combines real-time weather data, a countdown timer, and Wi-Fi monitoring. The UI was created with Squareline Studio and features OpenWeatherMap integration, hex countdown display, and visual indicators for Wi-Fi strength.

## How it works:
- Initialization: Sets up display, LVGL, and WiFi connection
- Weather Updates: Fetches current temperature & humidity from OpenWeatherMap API every 10min
- Countdown Timer: 10-minute OWM update countdown displayed in hexadecimal (from 0x258 to 0X000 -- 600 to 0)
- Wi-Fi Monitoring: Regularly checks and displays connection quality
- LED Synchronization: Matches NeoPixel LED color with on-screen R2D2-LED panel

## Notes:
- Requires [OpenWeatherMap API key](https://openweathermap.org) (free tier available)
- Edit these variables in the code:
  - Wi-Fi SSID & password
  - City name (if your city/town name has spaces - use: *%20*)
  - Country code
  - API key
- Uses metric units for temperature
- Backlight brightness is set to 75% by default
- Had to edit line 4 to *#include "lv_conf.h"* and comment out line 5 in LVGL_Driver.h

## SLS Project Files:
- This repository includes the SquareLine Studio project files in: **'.\Waveshare-ESP32-C6-LCD-1.47-R2D2_Monitor\sls_files'**
- In the **'sls_files'** folder, there are 2 subfolders: **'export'** & **'project'**
- Open SquareLine_Project in the **'project'** folder with Squareline Studio to make changes to the UI.
- You may need to update the SLS project settings **Project Export Root** & **UI Files Export Path** locations to reflect where you have saved the Arduino project **before exporting**.
- Export project files to the **'export'** folder & copy all, then replace all files in the **root** of the Arduino project folder.
- **Do not export into the root Arduino project folder as SLS will erase the folder contents before exporting!**
- **NB!!** Every time a change is made in SLS, & the UI files have been replaced in the Arduino project folder - you **must** edit the ui.h file on line 30: **FROM** *#include "screens/ui_MainScreen.h"* **TO** *#include "ui_MainScreen.h"* (remove *'screens/'* -- I'm not sure why SLS is exporting this way as the flat export option is selected.)

## Credits:
This project is inspired by [Volos Projects - waveshareBoards](https://github.com/VolosR/waveshareBoards) modified S3 example for the Waveshare ESP32-C6-LCD-1.47
