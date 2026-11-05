# STM32 Practice

STM32 韌體與電子電路實作紀錄，整理硬體測試結果、除錯過程與學習重點。各實作以獨立 STM32CubeIDE 專案保存。

## 實驗環境

- 開發板：以下已驗收 Lab 使用 NUCLEO-F446RE。
- 開發工具：STM32CubeIDE、STM32 HAL。
- UART：USART2，115200 baud / 8N1，透過 ST-LINK Virtual COM Port 與 MobaXterm 測試。
- RC 量測：邏輯分析儀搭配 Logic 2；接線與擷取條件見 Lab 文件。

## 已驗收 Lab

| Lab | 實測成果 | 驗收文件 |
|---|---|---|
| USART2 RX Interrupt | UART 中斷接收與指令辨識；Callback 通知事件、Main Loop 執行 LED 動作。`A1?` 觸發 LD2 切換三次、`A2?` 無動作，且 `A2? → A1?` 後仍正常接收，皆為實機 PASS。 | [UART Lab](F446RE_UART_Echo_Lab/README.md) |
| EE Foundations · GPIO RC Timing | PA8 輸出約 10 Hz 方波，完成三組 R/C 配置的邊緣延遲量測；10 kΩ + 100 nF 的上升／下降延遲為 427／828 µs。 | [RC Lab](F446RE_EE_Lab03_RC_GPIO/README.md) |

## 驗收範圍

- UART DMA、Receive-to-Idle、FreeRTOS 同步、高流量連續接收與 UART 錯誤復原尚未驗收。
- RC 結果為邏輯分析儀的數位門檻交越延遲，不代表直接量得類比波形或時間常數 τ；上升／下降不對稱仍待示波器確認。
- 其他專案目錄保留為練習紀錄；目錄存在不代表已完成驗收。詳細證據與限制以各 Lab 文件為準。
