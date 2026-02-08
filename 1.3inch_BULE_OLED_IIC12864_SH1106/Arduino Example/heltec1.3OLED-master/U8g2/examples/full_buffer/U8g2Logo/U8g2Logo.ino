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
#include <HTTPClient.h>
#include <DHTesp.h>  // ESP32 專用的 DHT 庫
#include "thingsBoardConfig.h"

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

// 三色 LED 控制（GND GYR 共陰極，接 GPIO）
#define LED_R_PIN 5   // 紅
#define LED_Y_PIN 17  // 黃
#define LED_G_PIN 16  // 綠

// 抽水馬達控制（接在 GPIO 13，經 HS-F04A 繼電器）
#define PUMP_PIN 13

// 類比式水位感測器（接在 GPIO 34，ADC1，輸出 0~100）
#define WATER_SENSOR_PIN 34

// Steam/土壤感測器（接在 GPIO 35，ADC1，類比 0~4095）
#define STEAM_SENSOR_PIN 35

// HS-20B 光強度感測器（接在 GPIO 32，ADC1，類比 0~4095，值愈大愈亮）
#define LIGHT_SENSOR_PIN 32

// 蜂鳴器（接在 GPIO 18，有源/無源皆可）
#define BUZZER_PIN 18

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 初始化 DHT 感應器（使用 DHTesp 庫）
DHTesp dht;

// 溫溼度變數（本地變數，用於顯示）
float localTemperature = 0.0;
float localHumidity = 0.0;
unsigned long lastDHTRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000;  // 每 2 秒讀取一次

// ThingsBoard 上傳間隔（溫度、溼度、水位）
unsigned long lastTBUpload = 0;
const unsigned long TB_UPLOAD_INTERVAL = 10000;  // 每 10 秒上傳一次

// ThingsBoard 屬性讀取間隔（Slider、Toggle 控制）
unsigned long lastTBAttrFetch = 0;
const unsigned long TB_ATTR_FETCH_INTERVAL = 1000;  // 每 1 秒讀取，Slider 變更更快生效

// 水位感測器 water_sensor（0~100），Liquid level、Slider 皆用此值
int waterLevel = 0;
unsigned long lastWaterRead = 0;
const unsigned long WATER_READ_INTERVAL = 1000;  // 每 1 秒讀取一次

// Steam 感測器（GPIO 35，類比 0~4095，值愈大愈有水）
int steamSensorValue = 0;
unsigned long lastSteamRead = 0;
const unsigned long STEAM_READ_INTERVAL = 500;  // 每 0.5 秒讀取一次

// HS-20B 光強度感測器（GPIO 32，類比 0~4095，值愈大愈亮）
int lightSensorValue = 0;
unsigned long lastLightRead = 0;
const unsigned long LIGHT_READ_INTERVAL = 500;  // 每 0.5 秒讀取一次

// 本地控制變數（ThingsBoard 可透過 shared attributes 控制）
bool pumping_motor = false;
bool LED_switch = false;  // Toggle 強制綠燈亮，true=強制亮、false=依光感自動
int water_threshold = 100;  // 預設值，2 秒內會被 Slider 寫入的 shared attribute 覆蓋
int light_threshold = 500;  // 光強度閾值（0~4095），低於此值時綠燈亮
int wifi_rssi = 0;  // WiFi 強度 0~100%

// 紅燈閃爍用（水位低時）
unsigned long lastRedLedToggle = 0;
const unsigned long RED_LED_BLINK_INTERVAL = 300;  // 閃爍間隔 ms

// 黃燈閃爍用（馬達運轉時，每 3 秒閃一次）
unsigned long lastYellowLedToggle = 0;
const unsigned long YELLOW_LED_BLINK_INTERVAL = 3000;  // 3 秒

// 蜂鳴器：每 5 秒響一聲（水位低時）
unsigned long lastBuzzerBeep = 0;
const unsigned long BUZZER_BEEP_INTERVAL = 5000;  // 5 秒
const unsigned long BUZZER_BEEP_DURATION = 150;   // 單次響聲 150ms
bool buzzerBeeping = false;

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
  
  // 初始化三色 LED（R=5, Y=17, G=16，共陰極 GND GYR）
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_Y_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  digitalWrite(LED_R_PIN, LOW);
  digitalWrite(LED_Y_PIN, LOW);
  digitalWrite(LED_G_PIN, LOW);
  Serial.println("3-color LED initialized (R=5, Y=17, G=16)");

  // 初始化抽水馬達（GPIO 13，經 HS-F04A 低電平觸發繼電器）
  pinMode(PUMP_PIN, OUTPUT);
  pumping_motor = false;  // 預設關閉
  digitalWrite(PUMP_PIN, HIGH);  // 低電平觸發：HIGH=繼電器關閉，馬達不運轉
  Serial.println("Pumping motor initialized on GPIO 13 (default OFF)");

  // 初始化類比式水位感測器（GPIO 34）、Steam（GPIO 35）、HS-20B 光感測器（GPIO 32）
  analogReadResolution(12);  // ESP32 使用 12-bit ADC，0~4095
  Serial.println("Water: GPIO 34, Steam: GPIO 35, Light: GPIO 32");
  
  // 初始化蜂鳴器（GPIO 18）
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Buzzer initialized on GPIO 18");
  
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
    Serial.println("ThingsBoard telemetry ready.");
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

void drawWiFiInfo(void) {
  // 限制在上半部分的前 16 像素範圍內（0-16），寬度 0-127
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    
    // 順序：WiFi 訊號圖示（從 x=0 開始）→ SSID + 強度百分比
    int startX = 0;
    int signalRightX = startX + 16;
    drawWiFiSignal(signalRightX, 14, rssi);
    
    int ssidX = signalRightX + 2;
    u8g2.setFont(u8g2_font_6x10_tr);
    int rssiPct = constrain(map(WiFi.RSSI(), -100, 0, 0, 100), 0, 100);
    String ssidText = String(ssid) + " " + String(rssiPct) + "%";
    int maxSSIDWidth = 127 - ssidX - 2;
    int ssidWidth = u8g2.getStrWidth(ssidText.c_str());
    if (ssidWidth > maxSSIDWidth) {
      String shortSSID = String(ssid).substring(0, 4) + " " + String(rssiPct) + "%";
      u8g2.drawStr(ssidX, 11, shortSSID.c_str());
    } else {
      u8g2.drawStr(ssidX, 11, ssidText.c_str());
    }
  } else {
    int startX = 0;
    int signalRightX = startX + 16;
    int signalCenterX = startX + 8;
    int signalCenterY = 7;
    int xSize = 6;
    
    u8g2.drawLine(signalCenterX - xSize/2, signalCenterY - xSize/2, 
                  signalCenterX + xSize/2, signalCenterY + xSize/2);
    u8g2.drawLine(signalCenterX - xSize/2, signalCenterY + xSize/2, 
                  signalCenterX + xSize/2, signalCenterY - xSize/2);
    
    int disconnectedX = signalRightX + 2;
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setFontMode(1);
    u8g2.drawStr(disconnectedX, 11, "Disc");
  }
}

// 傳送 telemetry 到 ThingsBoard
void sendToThingsBoard() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(TB_TELEMETRY_URL);
  http.addHeader("Content-Type", "application/json");
  
  // 組裝 JSON telemetry（含 led_green 供 Toggle 顯示目前綠燈狀態）
  bool ledGreenOn = LED_switch || (lightSensorValue < light_threshold);
  String json = "{\"temperature\":" + String(localTemperature, 1) + 
                ",\"humidity\":" + String(localHumidity, 1) + 
                ",\"wifi_rssi\":" + String(wifi_rssi) + 
                ",\"water_sensor\":" + String(waterLevel) + 
                ",\"steam_sensor\":" + String(steamSensorValue) + 
                ",\"light_sensor\":" + String(lightSensorValue) + 
                ",\"pumping_motor\":" + String(pumping_motor ? "true" : "false") + 
                ",\"LED_switch\":" + String(LED_switch ? "true" : "false") + 
                ",\"led_green\":" + String(ledGreenOn ? "true" : "false") + "}";
  
  int httpCode = http.POST(json);
  
  if (httpCode == 200) {
    Serial.println("ThingsBoard telemetry OK");
  } else {
    Serial.print("ThingsBoard POST failed: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 從 ThingsBoard 讀取 shared attributes（pumping_motor）
// 讓 Toggle 按鈕的控制能傳到 ESP32
void fetchAttributesFromThingsBoard() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = "http://thingsboard.cloud/api/v1/" + String(TB_ACCESS_TOKEN) + "/attributes?sharedKeys=pumping_motor,LED_switch,water_threshold,light_threshold";
  http.begin(url);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    // 除錯：每 30 秒印一次 API 回傳（確認 Slider 是否有寫入）
    // 若需即時除錯，可取消下行註解，改為每次請求都印
    static unsigned long lastAttrDebug = 0;
    if (millis() - lastAttrDebug >= 30000) {
      lastAttrDebug = millis();
      Serial.print("TB attributes: ");
      Serial.println(payload);
    }
    // Serial.print("TB attributes: "); Serial.println(payload);  // 即時除錯用
    
    // 解析 pumping_motor
    if (payload.indexOf("\"pumping_motor\":true") >= 0) {
      pumping_motor = true;
    } else if (payload.indexOf("\"pumping_motor\":false") >= 0) {
      pumping_motor = false;
    }
    
    // 解析 LED_switch（Toggle 強制綠燈亮）
    if (payload.indexOf("\"LED_switch\":true") >= 0) {
      LED_switch = true;
    } else if (payload.indexOf("\"LED_switch\":false") >= 0) {
      LED_switch = false;
    }
    
    // 解析 water_threshold（Slider 設定的 0~100%）
    // 支援 {"water_threshold":20} 或 {"water_threshold":"20"} 格式
    int idx = payload.indexOf("\"water_threshold\":");
    if (idx >= 0) {
      int start = idx + 18;  // "\"water_threshold\":" 長度 = 18
      int end = payload.indexOf(",", start);
      if (end < 0) end = payload.indexOf("}", start);
      if (end > start) {
        String val = payload.substring(start, end);
        val.trim();
        if (val.startsWith("\"")) val = val.substring(1, val.length() - 1);  // 去掉字串引號
        int th = val.toInt();
        if (th >= 0 && th <= 100) {
          water_threshold = th;  // 0 = 關閉警示，1~100 = 低於此值時觸發
        }
      }
    }
    
    // 解析 light_threshold（光強度閾值 0~4095，低於此值時綠燈亮）
    int idx2 = payload.indexOf("\"light_threshold\":");
    if (idx2 >= 0) {
      int start = idx2 + 18;
      int end = payload.indexOf(",", start);
      if (end < 0) end = payload.indexOf("}", start);
      if (end > start) {
        String val = payload.substring(start, end);
        val.trim();
        if (val.startsWith("\"")) val = val.substring(1, val.length() - 1);
        int lt = val.toInt();
        if (lt >= 0 && lt <= 4095) {
          light_threshold = lt;
        }
      }
    }
  } else {
    Serial.print("Fetch attributes failed: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 讀取溫溼度感應器數據
void readDHT() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastDHTRead >= DHT_READ_INTERVAL) {
    lastDHTRead = currentMillis;
    
    TempAndHumidity data = dht.getTempAndHumidity();
    
    if (!isnan(data.temperature) && !isnan(data.humidity)) {
      localTemperature = data.temperature;
      localHumidity = data.humidity;
      
      Serial.print("Temperature: ");
      Serial.print(localTemperature);
      Serial.print("°C, Humidity: ");
      Serial.print(localHumidity);
      Serial.println("%");
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }
}

// 讀取水位感測器（0~100，供 Liquid level、Slider 使用）
void readWaterSensor() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastWaterRead >= WATER_READ_INTERVAL) {
    lastWaterRead = currentMillis;
    waterLevel = analogRead(WATER_SENSOR_PIN);
    waterLevel = constrain(waterLevel, 0, 100);  // 感測器輸出 0~100
  }
}

// 讀取 Steam 感測器（GPIO 35，類比 0~4095）
void readSteamSensor() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSteamRead >= STEAM_READ_INTERVAL) {
    lastSteamRead = currentMillis;
    steamSensorValue = analogRead(STEAM_SENSOR_PIN);
  }
}

// 讀取 HS-20B 光強度感測器（GPIO 32，類比 0~4095，值愈大愈亮）
void readLightSensor() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastLightRead >= LIGHT_READ_INTERVAL) {
    lastLightRead = currentMillis;
    lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
  }
}

// 顯示溫溼度、水位、WiFi RSSI 數據
void drawTemperatureHumidity(void) {
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFontMode(1); // Transparent
  
  // 顯示溫度（使用本地變數）
  String tempStr = "Temp: " + String(localTemperature, 1) + " C";
  u8g2.drawStr(2, 28, tempStr.c_str());
  
  // 顯示濕度（使用本地變數）
  String humStr = "Hum: " + String(localHumidity, 1) + " %";
  u8g2.drawStr(2, 43, humStr.c_str());

  // 顯示水位（water_sensor 0~100）與光強度（light_sensor 0~4095）
  String waterStr = "W:" + String(waterLevel) + " L:" + String(lightSensorValue);
  u8g2.drawStr(2, 55, waterStr.c_str());
}

void loop(void) {
  // 檢查 WiFi 連線狀態
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  } else {
    // 更新 wifi_rssi（0~100%）
    wifi_rssi = constrain(map(WiFi.RSSI(), -100, 0, 0, 100), 0, 100);
  }
  
  // 讀取感測器
  readDHT();
  readWaterSensor();
  readSteamSensor();
  readLightSensor();

  // 定時上傳到 ThingsBoard
  if (WiFi.status() == WL_CONNECTED && (millis() - lastTBUpload >= TB_UPLOAD_INTERVAL)) {
    lastTBUpload = millis();
    sendToThingsBoard();
  }

  // 定時從 ThingsBoard 讀取 shared attributes（Toggle 按鈕控制 pumping_motor）
  if (WiFi.status() == WL_CONNECTED && (millis() - lastTBAttrFetch >= TB_ATTR_FETCH_INTERVAL)) {
    lastTBAttrFetch = millis();
    fetchAttributesFromThingsBoard();
  }

  // 三色 LED：綠=LED_switch 強制亮 或 光強度低於閾值、黃=馬達運轉、紅=水位低閃爍
  unsigned long now = millis();
  bool lightLow = (lightSensorValue < light_threshold);
  bool greenLedOn = LED_switch || lightLow;  // Toggle 強制亮 或 光感自動
  digitalWrite(LED_G_PIN, greenLedOn ? HIGH : LOW);
  
  // 黃燈：馬達運轉時每 3 秒閃爍一次
  if (pumping_motor) {
    if (now - lastYellowLedToggle >= YELLOW_LED_BLINK_INTERVAL) {
      lastYellowLedToggle = now;
      static bool yellowLedState = false;
      yellowLedState = !yellowLedState;
      digitalWrite(LED_Y_PIN, yellowLedState ? HIGH : LOW);
    }
  } else {
    digitalWrite(LED_Y_PIN, LOW);
  }
  
  // 紅燈＋蜂鳴器：water_sensor 低於 Slider 閾值時觸發
  bool waterLow = (waterLevel < water_threshold);
  // 除錯：每 5 秒印一次
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 5000) {
    lastDebug = now;
    Serial.print("water:");
    Serial.print(waterLevel);
    Serial.print("|th:");
    Serial.print(water_threshold);
    Serial.print(" light:");
    Serial.print(lightSensorValue);
    Serial.print("|th:");
    Serial.print(light_threshold);
    Serial.println(waterLow ? " waterLow" : "");
  }
  
  if (waterLow) {
    // 紅燈閃爍
    if (now - lastRedLedToggle >= RED_LED_BLINK_INTERVAL) {
      lastRedLedToggle = now;
      static bool redLedState = false;
      redLedState = !redLedState;
      digitalWrite(LED_R_PIN, redLedState ? HIGH : LOW);
    }
    // 蜂鳴器：每 5 秒響一聲（150ms）
    if (buzzerBeeping) {
      if (now - lastBuzzerBeep >= BUZZER_BEEP_DURATION) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerBeeping = false;
      }
    } else if (now - lastBuzzerBeep >= BUZZER_BEEP_INTERVAL) {
      digitalWrite(BUZZER_PIN, HIGH);
      lastBuzzerBeep = now;
      buzzerBeeping = true;
    }
  } else {
    digitalWrite(LED_R_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerBeeping = false;
  }
  
  digitalWrite(PUMP_PIN, pumping_motor ? LOW : HIGH);  // 低電平觸發
  
  u8g2.clearBuffer();
  drawWiFiInfo();
  drawTemperatureHumidity();
  u8g2.sendBuffer();
  delay(500);
}


