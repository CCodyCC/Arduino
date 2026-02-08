# ThingsBoard 控制 ESP32 完整教學

## 概述

本文件說明如何在 ThingsBoard 儀表板上：
1. 使用 **Toggle button** 控制抽水馬達 (`pumping_motor`)
2. 使用 **Value stepper** 或 **Slider** 設定水位閾值 (`water_threshold`)，當 `water_sensor` 低於設定值時紅燈閃爍

---

## 一、水位閾值設定（water_threshold → 紅燈閃爍＋蜂鳴器）

### 功能說明

- **water_sensor**：水位值 0~100，Liquid level、Slider 皆用此值
- **water_threshold**：Slider 閾值（0~100），與 water_sensor 比對
- 當 **water_sensor < water_threshold** 時：紅色 LED **閃爍** ＋ 蜂鳴器 **每 5 秒響一聲**
- 當水位 **高於或等於** 閾值時：紅燈依馬達狀態顯示

---

## 一之二、光強度閾值設定（light_threshold → 綠燈）

### 功能說明

- **light_sensor**：HS-20B 光強度值 0~4095（值愈大愈亮），上傳至 ThingsBoard
- **light_threshold**：Slider 閾值（0~4095），與 light_sensor 比對
- 當 **light_sensor < light_threshold** 時：**綠色 LED 亮**（光線不足提醒）
- 綠燈不再表示 WiFi 連線狀態

### ThingsBoard Slider 設定（light_threshold）

| 欄位 | 設定值 |
|-----|-------|
| **Target device** | 選 **Device**，再選你的 ESP32 裝置 |
| **Data key** | `light_threshold`（Shared attribute） |
| **Min value** | 0 |
| **Max value** | 4095 |
| **Step** | 100 或 50 |

**首次使用**：到 Device → Attributes → Shared attributes → Add attribute，新增 `light_threshold`（數值型，0~4095，例如 500）

### 支援的 Widget（皆可寫入 Shared attribute）

| Widget | 分類 | 優點 | 建議 |
|--------|------|------|------|
| **Slider** | Input widgets | 拖曳調整、直覺 | ★ 推薦 |
| **Value stepper** | Input widgets | +/- 按鈕、精確 | 可選 |
| **Knob** | Input widgets | 旋鈕式 | 可選 |

三種皆在 **Input widgets** 分類下，都能將調整後的值寫入 `water_threshold` shared attribute。

---

### 方法 A：使用 Slider（推薦）

1. 開啟 Dashboard → **Edit** → **+ Add widget**
2. 搜尋 **Input** 或 **Slider**
3. 選擇 **Slider**（在 Input widgets 分類下）

**設定：**

| 欄位 | 設定值 |
|-----|-------|
| **Target device** | 選 **Device**，再選你的 ESP32 裝置 |
| **Data key** | `water_threshold`（Shared attribute） |
| **Min value** | 0 |
| **Max value** | 100 |
| **Step** | 5 或 10 |

> **注意**：water_threshold 與 water_sensor 同範圍（0~100）。

---

### 方法 B：使用 Value stepper

1. 開啟 Dashboard → **Edit** → **+ Add widget**
2. 搜尋 **Input** 或 **Value stepper**
3. 選擇 **Value stepper**

**設定：**

| 欄位 | 設定值 |
|-----|-------|
| **Target device** | 選 **Device**，再選你的 ESP32 裝置 |
| **Data key** | `water_threshold` |
| **Min value** | 0 |
| **Max value** | 100 |
| **Step** | 5 或 10 |

---

### Target device 怎麼選？

| 選項 | 適用 | 操作 |
|-----|------|------|
| **Device** | 單一裝置 | 直接選裝置，較簡單 |
| **Entity alias** | 多裝置或共用 | 需先建立 alias（Filter type: Entity type → Type: Device） |

**建議**：只有一個 ESP32 時，選 **Device** 即可。

---

### Data key：water_threshold 填在哪裡？

在 Widget 的 **Data** 設定區塊中：

- 找到 **Data key** / **Key** / **Attribute key**
- 輸入或選擇：`water_threshold`
- 確認類型為 **Attribute**、Scope 為 **Shared**

---

### 首次使用：先新增 water_threshold

若裝置尚無此 attribute，Widget 無法綁定。請先：

1. **Device** → 選你的 ESP32 裝置
2. **Attributes** → **Shared attributes** → **+ Add attribute**
3. Key：`water_threshold`
4. Value：`20`（表示低於 20% 時警示。可依需求設 10~100）

---

## 二、Toggle button 設定（抽水馬達）

### 1. 選擇合適的 Widget

| Widget 類型 | 適用情境 | 說明 |
|------------|---------|------|
| **Command button** | 單一動作 | 每次點擊執行同一個動作（開或關） |
| **Toggle button** | 開關切換 | 內建開/關兩種狀態，自動切換 |
| **Power button** | 電源開關 | 視覺化 ON/OFF 按鈕 |

**建議**：對抽水馬達使用 **Toggle button**，一個按鈕即可開關。

---

### 2. 新增按鈕 Widget

1. 開啟你的 Dashboard，點擊 **Edit**（編輯模式）
2. 點擊 **+ Add widget**
3. 搜尋或選擇 **Buttons** 分類
4. 選擇 **Command button** 或 **Toggle button**
5. 在 **Data** 標籤：
   - **Entity**：選擇你的裝置（Device）
   - 若需要顯示狀態，可加 Data key：`pumping_motor`（latest value）

---

### 3. 設定 Behavior（行為）

點擊 **Behavior** 區塊中的 **On click** 旁的鉛筆圖示進行編輯。

#### 方法 A：單一 Command button（開馬達）

| 欄位 | 設定值 |
|-----|-------|
| Action | **Set attribute** |
| Attribute scope | **Shared** |
| Attribute key | `pumping_motor` |
| Value | **Constant** → **Boolean** → **True** |

#### 方法 B：單一 Command button（關馬達）

| 欄位 | 設定值 |
|-----|-------|
| Action | **Set attribute** |
| Attribute scope | **Shared** |
| Attribute key | `pumping_motor` |
| Value | **Constant** → **Boolean** → **False** |

#### 方法 C：Toggle button（一個按鈕開關切換）★ 推薦

Toggle button 有 **Checked**（勾選/開）和 **Unchecked**（未勾選/關）兩種狀態，需分別設定：

| 狀態 | 對應動作 | 設定值 |
|-----|---------|-------|
| **Checked** | 馬達運轉 | `pumping_motor` = `true` |
| **Unchecked** | 馬達停止 | `pumping_motor` = `false` |

**操作步驟：**

1. 點擊 **Behavior** → 找到 **Checked** 與 **Unchecked** 兩個區塊
2. **Checked（馬達運轉時）**：
   - 點鉛筆圖示編輯
   - Action：**Set attribute**
   - Attribute scope：**Shared**
   - Attribute key：`pumping_motor`
   - Value：**Constant** → **Boolean** → **True** ✓
3. **Unchecked（馬達停止時）**：
   - 點鉛筆圖示編輯
   - Action：**Set attribute**
   - Attribute scope：**Shared**
   - Attribute key：`pumping_motor`
   - Value：**Constant** → **Boolean** → **False** ✓

---

### 4. 設定重點整理

| 項目 | 必須設定 |
|-----|---------|
| **Attribute scope** | 一定要選 **Shared**（裝置才能讀取） |
| **Attribute key** | 輸入 `pumping_motor` |
| **Value 類型** | Boolean（布林值） |
| **Value** | `true` = 馬達運轉，`false` = 馬達停止 |

---

## 二之二、Toggle button 設定（綠燈 LED_switch）

### 功能說明

- **LED_switch**：強制綠燈亮，由 Toggle 控制
- **Checked（開）**：綠燈強制亮，不論光感測器
- **Unchecked（關）**：依光感自動（light_sensor < light_threshold 時亮）
- Telemetry 會上傳 `LED_switch`、`led_green`，可綁定顯示目前狀態

### 設定步驟（與 pumping_motor 相同方式）

1. **Device** → Attributes → Shared attributes → Add attribute
   - Key：`LED_switch`
   - Value：`false`（預設關）

2. 新增 **Toggle button**，Behavior 設定：
   - **Checked**：Set attribute → Shared → `LED_switch` = `true`
   - **Unchecked**：Set attribute → Shared → `LED_switch` = `false`

3. 若要在 Toggle 顯示目前狀態：Data key 綁定 `LED_switch`（latest value 或 attribute）

---

## 三、ESP32 端設定（必須）

ThingsBoard 按鈕、Slider、Value stepper 會把值寫入裝置的 **shared attributes**。ESP32 使用 HTTP 連線時，**不會主動收到**這個更新，必須自己定期去讀取。

### 已實作功能 ✓

1. **water_sensor**：水位 0~100，供 Liquid level、Slider 使用
2. **light_sensor**：HS-20B 光強度 0~4095（GPIO 32），上傳至 ThingsBoard
3. **定期呼叫 Attributes API** 取得 shared attributes（每 1 秒）
4. **解析 JSON** 更新 `pumping_motor`、`LED_switch`、`water_threshold`、`light_threshold`
5. **水位低於閾值**：紅燈閃爍 ＋ 蜂鳴器每 5 秒響一聲（150ms）
6. **綠燈**：LED_switch 強制亮 **或** 光強度低於閾值（light_sensor < light_threshold）
7. **Telemetry** 上傳 `LED_switch`、`led_green` 供 Toggle 顯示目前狀態

### API 格式

```
GET https://thingsboard.cloud/api/v1/{ACCESS_TOKEN}/attributes?sharedKeys=pumping_motor,LED_switch,water_threshold,light_threshold
```

回傳範例：
```json
{"pumping_motor": false, "LED_switch": false, "water_threshold": 20, "light_threshold": 500}
```

> `water_threshold` 與 water_sensor 同範圍（0~100）。`light_threshold` 與 light_sensor 同範圍（0~4095）。

### 建議讀取間隔

- 每 **2 秒** 讀取一次（反應速度與 API 負載的平衡）
- 可依需求調整為 1~5 秒

---

## 四、完整流程圖

### Slider / Value stepper（水位閾值 + 紅燈閃爍）

```
[使用者調整 Slider 或 Value stepper]
         ↓
[ThingsBoard 寫入 shared attribute: water_threshold = 數值]
         ↓
[ESP32 定期 GET /attributes?sharedKeys=water_threshold]
         ↓
[ESP32 解析 JSON，更新 water_threshold]
         ↓
[若 water_sensor < water_threshold → 紅燈閃爍＋蜂鳴器每 5 秒響]
[否則 → 紅燈依 pumping_motor 顯示]
```

### Toggle button（抽水馬達）

```
[使用者點擊 ThingsBoard Toggle]
         ↓
[ThingsBoard 寫入 shared attribute: pumping_motor = true/false]
         ↓
[ESP32 定期 GET /attributes?sharedKeys=pumping_motor]
         ↓
[ESP32 更新 pumping_motor，控制繼電器]
```

---

## 五、常見問題

### Q1：按了按鈕沒反應？

**原因**：ESP32 沒有讀取 shared attributes。

**解決**：在程式中加入 `fetchAttributesFromThingsBoard()` 並在 `loop()` 中定期呼叫。

---

### Q2：水位 14% 低於閾值，但紅燈沒閃、蜂鳴器沒響？

**常見原因**：`water_threshold` 在 ThingsBoard 被設成 **0**。

- 閾值 0 = 只有水位 < 0% 才觸發（永遠不會）
- 請將閾值設為 **20** 或 **100**（0~100%）

**檢查步驟**：
1. 打開 **Serial Monitor**（115200 baud），每 5 秒會印出：`water: X% | threshold: Y% | low: YES/NO`
2. 若 `threshold: 0` → 到 ThingsBoard 將 `water_threshold` 改為 **20** 或 **100**
3. 若 `Fetch attributes failed` → 檢查 WiFi 與 Access Token 是否正確

---

### Q3：Attribute key 在哪裡設定？

**Toggle button**：Behavior → On click → 點鉛筆編輯 → **Attribute key** 欄位

**Slider / Value stepper**：Data 區塊 → **Data key** 或 **Key** 欄位，輸入 `water_threshold`

---

### Q4：Slider / Value stepper 找不到 water_threshold？

**原因**：裝置尚未有該 shared attribute。

**解決**：先到 Device → Attributes → Shared attributes → Add attribute，新增 `water_threshold`（數值型，0~100，例如 20），Widget 才能綁定並寫入。

---

### Q5：Slider 調到 30% 但後台還是回傳 0？

**原因**：Slider 的 **On value change** 沒有成功寫入 shared attribute（常見：選錯 Action 或 Request timeout）。

**檢查 Slider 設定：**

1. 編輯 Slider → 找 **Behavior** 或 **On value change**
2. 確認 **Action** = **Set attribute**（不要選 RPC）
3. **Attribute scope** = **Shared**
4. **Attribute key** = `water_threshold`
5. **Value** = 滑桿的當前值（依版本可能為 `$entity`、`$value` 或類似）

**若仍失敗**：改用 **Value stepper**，或手動到 Device → Shared attributes 修改 `water_threshold`。

**除錯**：Serial Monitor 每 30 秒會印 `TB attributes: {...}`，可確認 ThingsBoard 實際回傳內容。

---

## 六、接線參考

| 元件 | GPIO | 說明 |
|-----|------|------|
| 水位感測器 | 34（ADC） | 輸出 0~100，Liquid level、Slider 皆用 water_sensor |
| HS-20B 光強度感測器 | **32（ADC）** | 類比 0~4095，值愈大愈亮，上傳為 light_sensor |
| 蜂鳴器 | 18 | 水位低時每 5 秒響一聲 |

### 三色 LED 狀態

| 燈色 | 狀態 |
|-----|------|
| **綠** | LED_switch 強制亮 **或** 光強度低於閾值（light_sensor < light_threshold） |
| **黃** | 馬達運轉中（每 3 秒閃爍一次） |
| **紅** | 水位低於閾值（快速閃爍＋蜂鳴器） |

---

## 七、相關檔案

- `U8g2Logo.ino` - 主程式（water_sensor、light_sensor、Liquid level、Slider、紅/綠燈、蜂鳴器 ✓）
- `thingsBoardConfig.h` - Access Token 設定
- `esp32_sh1106_temperature.md` - 溫度感應器相關筆記

---

> 最後更新：2025/02/07
