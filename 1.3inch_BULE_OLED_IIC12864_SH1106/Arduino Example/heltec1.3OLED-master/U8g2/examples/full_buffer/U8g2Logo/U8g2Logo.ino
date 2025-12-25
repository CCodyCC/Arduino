/*

  U8g2Logo.ino

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <DHTesp.h>  // ESP32 專用的 DHT 庫
#include "thingProperties.h"  // Arduino Cloud 設定檔

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// WiFi 設定
const char* ssid = "Chou";
const char* password = "0935963000";

// 溫溼度感應器設定（接在擴展板 pin 4）
#define DHT_PIN 4

// LED 控制（接在 pin 19）
#define LED_PIN 19

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 初始化 DHT 感應器（使用 DHTesp 庫）
DHTesp dht;

// 溫溼度變數（本地變數，用於顯示）
float localTemperature = 0.0;
float localHumidity = 0.0;
// Arduino Cloud 變數在 thingProperties.h 中定義（temperature 和 humidity）
unsigned long lastDHTRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000;  // 每 2 秒讀取一次（用於本地顯示）

// Arduino Cloud 上傳間隔設定
unsigned long lastCloudUpload = 0;
const unsigned long CLOUD_UPLOAD_INTERVAL = 300000;  // 每 5 分鐘（300000 毫秒）上傳一次到 Cloud
unsigned long wifiConnectedTime = 0;  // WiFi 連接成功的時間
bool initialUploadDone = false;  // 是否已完成初始上傳

//#define MINI_LOGO

void setup(void) {
  Serial.begin(115200);
  Serial.println("ESP32 SH1106 WiFi 訊號顯示器");

  // 初始化 OLED 顯示器
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 25, "Initializing...");  // 在 18 像素之後顯示
  u8g2.sendBuffer();
  
  // 初始化 DHT 溫溼度感應器（DHTesp 庫）
  dht.setup(DHT_PIN, DHTesp::DHT11);  // 如果是 DHT22，改為 DHTesp::DHT22
  Serial.println("DHT sensor initialized");
  
  // 初始化 LED（pin 19）
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // 初始狀態為關閉
  Serial.println("LED initialized on pin 19");
  
  // 連接到 WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 25, "Connecting WiFi");  // 在 18 像素之後顯示
  u8g2.drawStr(0, 40, ssid);
  u8g2.sendBuffer();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // WiFi 連線成功，立即切換到正常顯示（不等待 Arduino Cloud）
    // 記錄 WiFi 連接成功的時間
    wifiConnectedTime = millis();
    initialUploadDone = false;
    
    // 初始化 Arduino Cloud（在背景連接，不阻塞顯示）
    Serial.println("Initializing Arduino Cloud (background connection)...");
    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    
    // 立即開始顯示數據（不等待 Cloud 連接）
    Serial.println("Starting display immediately...");
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");
    
    u8g2.clearBuffer();
    u8g2.drawStr(0, 25, "WiFi Failed!");  // 在 18 像素之後顯示
    u8g2.sendBuffer();
    delay(2000);
  }
}

void drawLogo(void)
{
    u8g2.setFontMode(1);	// Transparent
#ifdef MINI_LOGO

    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb16_mf);
    u8g2.drawStr(0, 22, "U");
    
    u8g2.setFontDirection(1);
    u8g2.setFont(u8g2_font_inb19_mn);
    u8g2.drawStr(14,8,"8");
    
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb16_mf);
    u8g2.drawStr(36,22,"g");
    u8g2.drawStr(48,22,"\xb2");
    
    u8g2.drawHLine(2, 25, 34);
    u8g2.drawHLine(3, 26, 34);
    u8g2.drawVLine(32, 22, 12);
    u8g2.drawVLine(33, 23, 12);
#else

    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb24_mf);
    u8g2.drawStr(0, 30, "U");
    
    u8g2.setFontDirection(1);
    u8g2.setFont(u8g2_font_inb30_mn);
    u8g2.drawStr(21,8,"8");
        
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb24_mf);
    u8g2.drawStr(51,30,"g");
    u8g2.drawStr(67,30,"\xb2");
    
    u8g2.drawHLine(2, 35, 47);
    u8g2.drawHLine(3, 36, 47);
    u8g2.drawVLine(45, 32, 12);
    u8g2.drawVLine(46, 33, 12);
    
#endif
}

// 繪製 WiFi 訊號強度圖示（限制在上半部分的前 16 像素範圍內，寬度 0-127）
void drawWiFiSignal(int x, int y, int rssi) {
  // 將 RSSI 轉換為訊號強度等級 (0-4)
  // RSSI 範圍：-100 到 0 dBm
  // -100 到 -80: 等級 0 (無訊號)
  // -80 到 -60: 等級 1 (弱)
  // -60 到 -40: 等級 2 (中)
  // -40 到 -20: 等級 3 (強)
  // -20 到 0: 等級 4 (最強)
  
  int level = 0;
  if (rssi > -50) level = 4;
  else if (rssi > -60) level = 3;
  else if (rssi > -70) level = 2;
  else if (rssi > -80) level = 1;
  else level = 0;
  
  // 繪製 WiFi 訊號圖示（從右到左，由小到大）
  // 調整大小以適合 0-16 像素範圍，寬度不超過 127
  int barWidth = 2;  // 條狀寬度
  int barSpacing = 1;
  int startX = x;      // 從右邊開始
  int startY = y;      // 底部對齊（y = 16）
  
  // 繪製 4 個訊號條，總高度不超過 16 像素，總寬度不超過 127
  for (int i = 0; i < 4; i++) {
    int barHeight = (i + 1) * 2 + 1;  // 每個條狀高度遞增，總高度約 13 像素
    int barX = startX - (i * (barWidth + barSpacing)) - barWidth;
    int barY = startY - barHeight;  // 從底部向上繪製
    
    // 確保不超出左邊界（x >= 0）
    if (barX < 0) break;
    
    if (i < level) {
      // 繪製實心條狀（有訊號的部分）
      u8g2.drawBox(barX, barY, barWidth, barHeight);
    } else {
      // 繪製空心條狀（無訊號的部分）
      u8g2.drawFrame(barX, barY, barWidth, barHeight);
    }
  }
}

// 繪製 LED 燈泡圖示（在 WiFi 訊號圖示左邊）
void drawLEDBulb(int x, int y, bool isOn) {
  // 燈泡中心位置
  int centerX = x;
  int centerY = y;
  int radius = 5;  // 燈泡半徑
  
  if (isOn) {
    // LED 亮時：填滿燈泡（黃色在單色 OLED 上顯示為白色）
    u8g2.drawDisc(centerX, centerY, radius);
    // 繪製燈泡外框（稍微大一點，形成邊框效果）
    u8g2.drawCircle(centerX, centerY, radius);
  } else {
    // LED 不亮時：只有外框
    u8g2.drawCircle(centerX, centerY, radius);
  }
  
  // 繪製燈泡底部的螺紋
  int threadWidth = 3;  
  int threadHeight = 1;  
  int threadX = centerX - threadWidth / 2;
  int threadY = centerY + radius;
  
  if (isOn) {
    u8g2.drawBox(threadX, threadY, threadWidth, threadHeight);
  } else {
    u8g2.drawFrame(threadX, threadY, threadWidth, threadHeight);
  }
}

void drawWiFiInfo(void) {
  // 限制在上半部分的前 16 像素範圍內（0-16），寬度 0-127
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    
    // 順序：WiFi 訊號圖示（從 x=0 開始）→ "CHOU" → LED 燈泡（最右邊）
    int startX = 0;  // 從 x=0 開始
    
    // 1. 繪製 WiFi 訊號圖示（從 x=0 開始，往上移2）
    // 訊號圖示寬度約 16 像素，從右向左繪製
    int signalRightX = startX + 16;  // 訊號圖示右邊位置
    drawWiFiSignal(signalRightX, 14, rssi);  // y = 14（
    
    // 2. 在訊號圖示後面直接顯示 "CHOU"（縮小字體）
    int ssidX = signalRightX + 2;  // 訊號圖示後 2 像素間距
    u8g2.setFont(u8g2_font_6x10_tr);  // 使用較小的大字體（適合 0-16 範圍）
    String ssidText = String(ssid);
    int ssidWidth = u8g2.getStrWidth(ssidText.c_str());
    
    // 計算 LED 燈泡位置（最右邊，往左移2）
    int ledX = 127 - 6 - 2;  // 最右邊，預留 6 像素空間，再往左移2
    
    // 確保 SSID 不會與 LED 燈泡重疊
    int maxSSIDWidth = ledX - ssidX - 2;  // 預留空間給 LED 燈泡
    if (ssidWidth > maxSSIDWidth) {
      // 如果 SSID 太長，截斷顯示
      String shortSSID = ssidText.substring(0, 6) + "...";
      u8g2.drawStr(ssidX, 11, shortSSID.c_str());  // y = 11
    } else {
      u8g2.drawStr(ssidX, 11, ssidText.c_str());  // y = 11
    }
    
    // 4. 在最右邊繪製 LED 燈泡圖示（往左移2）
    drawLEDBulb(ledX, 9, LED_switch);  // y = 9
  } else {
    // 顯示未連接狀態
    // 順序：WiFi 訊號位置（X 標記，從 x=0 開始）→ "Disc" → LED 燈泡（最右邊）
    int startX = 0;  // 從 x=0 開始
    
    // 1. 在 WiFi 訊號圖示位置繪製 X 標記表示無訊號（往上移2）
    // X 標記位置與 WiFi 訊號圖示位置相同
    int signalRightX = startX + 16;  // 訊號圖示右邊位置
    int signalCenterX = startX + 8;  // 訊號圖示中心位置
    int signalCenterY = 7;  // 訊號圖示中心 Y 位置（往上移2）
    int xSize = 6;  // X 標記大小
    
    // 繪製 X 標記（在 WiFi 訊號圖示的位置）
    u8g2.drawLine(signalCenterX - xSize/2, signalCenterY - xSize/2, 
                  signalCenterX + xSize/2, signalCenterY + xSize/2);
    u8g2.drawLine(signalCenterX - xSize/2, signalCenterY + xSize/2, 
                  signalCenterX + xSize/2, signalCenterY - xSize/2);
    
    // 2. 在 X 標記後面顯示 "Disc"（Disconnected 的縮寫）
    int disconnectedX = signalRightX + 2;  // X 標記後 2 像素間距
  u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setFontMode(1);
    u8g2.drawStr(disconnectedX, 11, "Disc");  // y = 11
    
    // 4. 在最右邊繪製 LED 燈泡圖示
    int ledX = 127 - 8;  // 最右邊，預留 8 像素空間
    drawLEDBulb(ledX, 9, LED_switch);  // y = 9
  }
}

// 讀取溫溼度感應器數據
void readDHT() {
  unsigned long currentMillis = millis();
  
  // 每 2 秒讀取一次
  if (currentMillis - lastDHTRead >= DHT_READ_INTERVAL) {
    lastDHTRead = currentMillis;
    
    // 讀取溫溼度數據（DHTesp 庫）
    TempAndHumidity data = dht.getTempAndHumidity();
    
    // 檢查讀取是否成功
    if (!isnan(data.temperature) && !isnan(data.humidity)) {
      localTemperature = data.temperature;
      localHumidity = data.humidity;
      
      // 序列埠輸出（用於除錯）
      Serial.print("Temperature: ");
      Serial.print(localTemperature);
      Serial.print("°C, Humidity: ");
      Serial.print(localHumidity);
      Serial.println("%");
      
      // 每 5 分鐘更新一次 Arduino Cloud 變數（節省免費方案配額）
      unsigned long currentMillis = millis();
      if (currentMillis - lastCloudUpload >= CLOUD_UPLOAD_INTERVAL) {
        lastCloudUpload = currentMillis;
        
        // 更新 Arduino Cloud 變數（變數名稱必須與 Cloud 中的定義一致）
        temp = data.temperature;
        humidity = data.humidity;
        
        Serial.print("Data uploaded to Arduino Cloud - Temp: ");
        Serial.print(temp);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
      } else {
        // 計算距離下次上傳還有多久
        unsigned long timeUntilUpload = CLOUD_UPLOAD_INTERVAL - (currentMillis - lastCloudUpload);
        Serial.print("Next Cloud upload in: ");
        Serial.print(timeUntilUpload / 1000);
        Serial.println(" seconds");
      }
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }
}

// 顯示溫溼度數據（在 18 像素之後）
void drawTemperatureHumidity(void) {
  // 在 18 像素之後顯示溫溼度
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFontMode(1); // Transparent
  
  // 顯示溫度（使用本地變數）
  String tempStr = "Temp: " + String(localTemperature, 1) + " C";
  u8g2.drawStr(2, 28, tempStr.c_str());
  
  // 顯示濕度（使用本地變數）
  String humStr = "Hum: " + String(localHumidity, 1) + " %";
  u8g2.drawStr(2, 43, humStr.c_str());
}

void loop(void) {
  // 更新 Arduino Cloud（必須在 loop 中調用）
  ArduinoCloud.update();
  
  // 檢查 WiFi 連線狀態
  if (WiFi.status() != WL_CONNECTED) {
    // 如果斷線，嘗試重新連接
    WiFi.begin(ssid, password);
    wifiConnectedTime = 0;  // 重置連接時間
    initialUploadDone = false;  // 重置上傳標記
  } else {
    // WiFi 已連接，檢查是否需要進行初始上傳
    if (wifiConnectedTime > 0 && !initialUploadDone && ArduinoCloud.connected()) {
      // 檢查是否已過 10 秒
      if (millis() - wifiConnectedTime >= 10000) {
        Serial.println("Uploading initial data to Arduino Cloud...");
        TempAndHumidity data = dht.getTempAndHumidity();
        
        if (!isnan(data.temperature) && !isnan(data.humidity)) {
          // 更新本地變數
          localTemperature = data.temperature;
          localHumidity = data.humidity;
          
          // 上傳到 Arduino Cloud
          temp = data.temperature;
          humidity = data.humidity;
          
          // 更新上傳時間戳記
          lastCloudUpload = millis();
          
          // 確保資料同步到 Cloud
          ArduinoCloud.update();
          
          Serial.print("Initial data uploaded - Temp: ");
          Serial.print(temp);
          Serial.print("°C, Humidity: ");
          Serial.print(humidity);
          Serial.println("%");
          
          initialUploadDone = true;  // 標記已完成初始上傳
        }
      }
    }
  }
  
  // 讀取溫溼度感應器數據
  readDHT();
  
  // 更新 Arduino Cloud 變數（變數會在 thingProperties.h 中定義）
  // 這些變數會自動同步到 Cloud
  
  u8g2.clearBuffer();
  
  // 繪製 WiFi 訊號強度圖示（在右上角，0-17 像素範圍）
  drawWiFiInfo();
  
  // 繪製溫溼度數據（在 18 像素之後）
  drawTemperatureHumidity();
  
  // 繪製 Logo（可選，如果不需要可以註解掉）
  // drawLogo();
  
  u8g2.sendBuffer();
  delay(500); // 每 500ms 更新一次顯示
}

// LED 開關改變時的回調函數（當 Arduino Cloud 中的 LED_switch 值改變時會自動調用）
void onLEDSwitchChange() {
  if (LED_switch) {
    digitalWrite(LED_PIN, HIGH);  // 開啟 LED
    Serial.println("LED turned ON (from Cloud)");
  } else {
    digitalWrite(LED_PIN, LOW);   // 關閉 LED
    Serial.println("LED turned OFF (from Cloud)");
  }
}


