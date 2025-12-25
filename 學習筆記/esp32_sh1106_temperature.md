# ESP32S + SH1106 OLED + 溫度感應器系統

## 修正/驗證/比對紀錄
- **建立日期**：2025/12/25
- **最後異動**：2025/12/25
- **驗證日期**：2025/12/25

## 模組概述

本模組實作 ESP32S 微控制器搭配擴展板，使用 SH1106 OLED 顯示器（I2C 介面）顯示溫度感應器讀數的系統。溫度感應器接在擴展板的 pin 39（類比輸入），透過 ADC 讀取類比電壓值並轉換為溫度顯示在 OLED 螢幕上。

## 核心功能

### 1. 溫度感應器讀取
- **功能描述**：從擴展板 pin 39 讀取類比溫度感應器數值
- **主要特色**：
  - 使用 ESP32 的 12 位元 ADC（解析度 0-4095）
  - 支援 TMP36 和 LM35 等類比溫度感應器
  - 每 1 秒讀取一次，避免過度頻繁讀取
  - 自動將類比數值轉換為電壓和溫度

### 2. SH1106 OLED 顯示
- **功能描述**：在 SH1106 OLED 顯示器上顯示溫度資訊
- **主要特色**：
  - 使用 I2C 介面連接（預設地址 0x3C）
  - 顯示溫度值（大字體，24pt）
  - 顯示類比讀數和電壓值（小字體，10pt）
  - 每 500ms 更新一次顯示，確保資訊即時性
  - 支援螢幕翻轉功能（可選）

### 3. 資料轉換與處理
- **功能描述**：將類比讀數轉換為實際溫度值
- **主要特色**：
  - 支援 TMP36 溫度感應器（預設）
  - 可調整為支援 LM35 溫度感應器
  - 溫度範圍限制在 -50°C 到 125°C
  - 提供電壓值計算和顯示

## 重要修正記錄

### 2025/12/25 - 初始版本建立
**修改概述**：建立 ESP32S + SH1106 OLED + 溫度感應器系統

#### 問題描述
需要建立一個使用 ESP32S 微控制器、擴展板、SH1106 OLED 顯示器和溫度感應器的系統。

#### 修正內容
1. **SH1106 OLED 顯示器初始化**：
 ```cpp
 SH1106Wire display(0x3c, 21, 22);  // ADDRESS, SDA, SCL
 display.init();
 display.flipScreenVertically();  // 可選：翻轉螢幕
 ```

2. **溫度感應器讀取**：
 ```cpp
 const int TEMP_SENSOR_PIN = 39;  // 擴展板 pin 39
 analogReadResolution(12);  // 設定 12 位元解析度
 sensorValue = analogRead(TEMP_SENSOR_PIN);
 ```

3. **溫度轉換公式**：
 ```cpp
 // TMP36 轉換公式
 float voltage = (sensorValue / ADC_RESOLUTION) * VOLTAGE_REF;
 temperature = ((voltage * 100.0) - 50.0);
 ```

#### 修正理由
1. **使用 SH1106Wire 庫**：根據 GitHub 文檔，SH1106 需要使用專用的 SH1106Wire 類別，而非 SSD1306Wire
2. **ESP32 ADC 設定**：ESP32 的 ADC 預設為 12 位元，需要明確設定解析度
3. **溫度轉換公式**：根據 TMP36 感應器的特性，使用標準的電壓轉溫度公式

## 資料結構

### 硬體連接
- **溫度感應器**：擴展板 pin 39（類比輸入，ADC1_CH3）
- **SH1106 OLED**：
  - I2C 地址：0x3C（預設）
  - SDA：GPIO 21（ESP32）
  - SCL：GPIO 22（ESP32）

### 變數定義
```cpp
const int TEMP_SENSOR_PIN = 39;     // 溫度感應器腳位
const float ADC_RESOLUTION = 4095.0;  // ESP32 ADC 解析度（12位元）
const float VOLTAGE_REF = 3.3;        // ESP32 參考電壓（3.3V）
const float TEMP_OFFSET = -50.0;      // TMP36 溫度偏移
const float TEMP_SCALE = 175.0;       // TMP36 溫度比例
```

## 技術實作

### 1. SH1106 OLED 初始化
```cpp
#include <Wire.h>
#include "SH1106Wire.h"

SH1106Wire display(0x3c, 21, 22);  // I2C 地址, SDA, SCL
display.init();
display.flipScreenVertically();  // 可選：根據硬體安裝方向
```

### 2. 溫度感應器讀取
```cpp
// 設定 ADC 解析度
analogReadResolution(12);

// 讀取類比數值
sensorValue = analogRead(TEMP_SENSOR_PIN);

// 轉換為電壓
float voltage = (sensorValue / ADC_RESOLUTION) * VOLTAGE_REF;

// 轉換為溫度（TMP36）
temperature = ((voltage * 100.0) - 50.0);
```

### 3. OLED 顯示更新
```cpp
display.clear();
display.setFont(ArialMT_Plain_24);
display.setTextAlignment(TEXT_ALIGN_CENTER);
display.drawString(64, 25, String(temperature, 1) + " C");
display.display();
```

## 使用方式

### 1. 硬體連接
1. 將 SH1106 OLED 顯示器連接到 ESP32S：
   - VCC → 3.3V
   - GND → GND
   - SDA → GPIO 21
   - SCL → GPIO 22

2. 將溫度感應器連接到擴展板 pin 39：
   - 正極 → 3.3V（或 5V，視感應器而定）
   - 負極 → GND
   - 訊號 → pin 39

### 2. 程式庫安裝
在 Arduino IDE 中安裝以下程式庫：
- **SH1106 OLED 驅動程式**：
  - 在 Arduino Library Manager 中搜尋 "ESP8266 and ESP32 Oled Driver for SSD1306 display"
  - 或從 GitHub 安裝：https://github.com/Xinyuan-LilyGO/esp32-oled-sh1106

### 3. 程式碼設定
1. 開啟 `esp32_sh1106_temperature.ino`
2. 確認 I2C 地址（預設 0x3C）
3. 確認 SDA 和 SCL 腳位（預設 GPIO 21 和 22）
4. 根據您的溫度感應器類型調整轉換公式：
   - **TMP36**：使用預設公式 `temperature = ((voltage * 100.0) - 50.0);`
   - **LM35**：使用公式 `temperature = voltage * 100.0;`

### 4. 上傳與執行
1. 選擇開發板：工具 → 開發板 → ESP32 Arduino → ESP32S Dev Module
2. 選擇正確的 COM 埠
3. 上傳程式碼
4. 開啟序列監視器（115200 baud）查看除錯資訊

## 注意事項

### 1. I2C 地址確認
- SH1106 OLED 的預設 I2C 地址為 0x3C
- 如果顯示器無法正常運作，請使用 I2C 掃描工具確認實際地址
- 某些顯示器可能使用 0x3D 地址

### 2. 溫度感應器類型
- **TMP36**：輸出電壓範圍 0V（-50°C）到 3.3V（125°C），公式：`溫度 = (電壓 - 0.5) * 100`
- **LM35**：輸出電壓範圍 0V（0°C）到 3.3V（330°C），公式：`溫度 = 電壓 * 100`
- 請根據實際使用的感應器類型調整轉換公式

### 3. ESP32 ADC 注意事項
- ESP32 的 ADC 解析度為 12 位元（0-4095）
- 參考電壓為 3.3V
- ADC 讀數可能會有雜訊，建議進行多次讀取取平均值

### 4. 螢幕方向
- 如果顯示內容上下顛倒，可以移除或註解 `display.flipScreenVertically();`
- 如果顯示內容左右顛倒，可以使用 `display.mirrorScreen();`

### 5. 電源供應
- 確保 ESP32S 和擴展板有足夠的電源供應
- OLED 顯示器需要穩定的 3.3V 電源
- 溫度感應器根據型號可能需要 3.3V 或 5V

## 相關檔案

### 主要檔案
- `esp32_sh1106_temperature.ino` - 主程式檔案

### 相關檔案
- `lcd_i2c_expansion.ino` - 舊版 LCD 顯示系統（參考）

### 學習筆記
- `esp32_sh1106_temperature.md` - 本說明文件

## 參考資源

### GitHub 資源
- [ESP32 OLED SH1106 驅動程式](https://github.com/Xinyuan-LilyGO/esp32-oled-sh1106)
- 使用 SH1106Wire 類別進行 I2C 通訊
- 支援 128x64 像素顯示

### 技術文件
- ESP32 ADC 規格：12 位元，參考電壓 3.3V
- SH1106 OLED 規格：128x64 像素，I2C 介面
- TMP36 溫度感應器：類比輸出，-50°C 到 125°C

## 未來擴展

### 1. 功能增強
- 支援多種溫度感應器類型自動識別
- 添加溫度歷史記錄功能
- 添加溫度警報功能（高溫/低溫警示）
- 支援華氏度顯示

### 2. 使用體驗優化
- 添加圖形化溫度顯示（溫度計圖示）
- 優化顯示更新頻率
- 添加按鈕控制功能（切換顯示模式）

### 3. 開發體驗改善
- 添加更詳細的錯誤處理
- 添加感應器連接檢測
- 添加 I2C 通訊錯誤處理

### 4. 整合優化
- 整合 WiFi 功能，將溫度資料上傳到雲端
- 整合其他感應器（濕度、壓力等）
- 整合時間顯示功能（RTC 或 NTP）

---

> 本文件建立於 2025/12/25，隨系統發展持續更新維護。最後更新：2025/12/25


