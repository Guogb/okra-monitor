# OKRA Monitor

ESP32 固件，运行在 CYD（Cheap Yellow Display）2.8" 320×240 ILI9341 屏幕上，实时显示 PC 硬件监控仪表盘（CPU / GPU / 内存 / 网络），数据通过 SSE 长连接从配套 PC 服务器获取。WiFi 经蓝牙 BLE 配网，无需硬编码 SSID。

- 屏幕 UI：LVGL 8.4
- 显示驱动：TFT_eSPI
- 蓝牙配网：NimBLE
- 数据解析：ArduinoJson + 自定义 SSE 解析器

> 本仓库**只含显示端固件**。设备不产生硬件数据，只消费配套服务器推送的 SSE 数据流。配套 PC 服务器（读取硬件传感器并输出 SSE）不在本仓库，详见下方[数据源 / 配套服务器](#数据源--配套服务器)。

---

## 硬件配置

| 项目 | 说明 |
|------|------|
| MCU | ESP32（esp32dev 板） |
| 屏幕 | 2.8" 320×240 横屏（`tft.setRotation(1)`），ILI9341（`ILI9341_2_DRIVER`） |
| 触屏 | XPT2046 电阻屏，走 HSPI |
| 背光 | GPIO 21，高电平有效 |
| SPI 时钟 | 40 MHz |

TFT 与触屏使用**两条独立的 SPI 总线**：TFT 走 VSPI（默认），触屏走 HSPI。

### TFT 引脚（来自 [lib/TFT_eSPI/User_Setup.h](lib/TFT_eSPI/User_Setup.h)）

| 信号 | GPIO |
|------|------|
| TFT_MOSI | 13 |
| TFT_MISO | 12 |
| TFT_SCLK | 14 |
| TFT_CS | 15 |
| TFT_DC | 2 |
| TFT_RST | -1（接板子 RST） |
| TFT_BL | 21 |

### 触屏引脚（来自 [src/main.cpp](src/main.cpp)）

| 信号 | GPIO |
|------|------|
| XPT2046_IRQ | 36 |
| XPT2046_MOSI | 32 |
| XPT2046_MISO | 39 |
| XPT2046_CLK | 25 |
| XPT2046_CS | 33 |

---

## 构建与烧录

需要 PlatformIO CLI（`pip install platformio` 或 VS Code 的 PlatformIO 插件）。

```bash
pio pkg install              # 安装依赖库（TFT_eSPI / lvgl / NimBLE / ArduinoJson / XPT2046）
pio run                      # 编译
pio run --target upload      # 烧录到 ESP32
pio device monitor           # 串口监视器（115200 波特率）
```

`pio run` 打印 `[SUCCESS]` 即编译通过。

---

## WiFi 配网（BLE Provisioning）

设备没有硬编码 SSID，开机后通过蓝牙接收 WiFi 配置：

1. 首次启动（NVS 无配置），或开机时按住 BOOT 键 3 秒清除 NVS 后，设备以 `OKRAWORKS_<MAC末6位>` 为名广播 BLE，含厂商 ID `0x04D2`。
2. 配网客户端扫描厂商 ID `0x04D2`，连接以下 BLE GATT：
   - 服务 UUID：`53020f00-319c-4d97-a2b1-9e706baba77a`
   - 特征 UUID：`f87709b3-63a7-4605-9bb5-73c383462296`
3. 向特征写入 JSON（UTF-8 字符串）：
   ```json
   {"ssid":"你的WiFi名","password":"你的WiFi密码","server_ip":"PC服务器IP"}
   ```
4. 设备将配置存入 NVS，连接 WiFi，再连接 `server_ip` 指定的服务器。

> **配网客户端不在本仓库**（原始版本是微信小程序）。你可以用任意 BLE 调试工具（如 nRF Connect / LightBlue）按上面的 UUID 写入 JSON，或自行实现一个简单的 BLE GATT 写入端。

配置写入后设备会自动重连；之后开机无需再次配网。如需重配，开机时按住 BOOT 键 3 秒清空 NVS 即可。

---

## 数据源 / 配套服务器

WiFi 连上后，固件向服务器发起 SSE 长连接：

- 地址：`http://<server_ip>:1919/sse`（端口 **1919**，路径 **/sse**，明文 HTTP）
- 每条 `data:` 行格式：
  ```
  data: Page0|{|}Simple1|cpu_usage: 2%{|}Simple2|cpu_temp: 39°C{|}...
  ```
  即 `Page0|` 前缀，`{|}` 分隔各段，每段 `<标签>|<key>: <值><单位>`。

固件识别的 key（来自 [src/data_fetcher.cpp](src/data_fetcher.cpp)）：

| key | 含义 |
|-----|------|
| `cpu_usage` | CPU 使用率 (%) |
| `cpu_temp` | CPU 温度 (°C) |
| `cpu_fun` | CPU 风扇转速 (%) |
| `cpu_volt` | CPU 电压 (V) |
| `gpu_core_usage` | GPU 核心使用率 (%) |
| `gpu_temp` | GPU 温度 (°C) |
| `gpu_vram_usage` | GPU 显存使用率 (%) |
| `gpu_fun` | GPU 风扇转速 (%) |
| `fps` | 帧率 |
| `mem_usage` | 内存使用率 (%) |
| `mem_freq` | 内存频率（固件内部会 ×2 显示） |
| `mem_volt` | 内存电压 (V) |
| `net_down` | 网络下行速率 |
| `net_up` | 网络上行速率 |

> **配套服务器不在本仓库**。你需要自行实现一个 PC 端服务：读取硬件传感器（例如 Windows 上用 LibreHardwareMonitor / OpenHardwareMonitor），按上面的 SSE 格式推流。没有服务器时，设备能正常启动、连上 WiFi，但仪表盘无数据。

---

## NTP 时间同步

设备连上 WiFi 后从 `ntp.aliyun.com`、`ntp.tencent.com`、`pool.ntp.org` 同步时间，时区固定为 UTC+8（中国时区）。其他时区需修改 [src/main.cpp](src/main.cpp) 中的 `configTime(8 * 3600, 0, ...)`。

---

## 项目结构

```
src/main.cpp                       入口：setup/loop、LVGL flush、触屏、networkTask
src/ble_config.{cpp,h}             BLE WiFi 配网（NimBLE）
src/data_fetcher.{cpp,h}           SSE 客户端 + 解析
src/ui/ui_monitor_ultimate.{c,h}   LVGL 仪表盘 UI
src/fonts/*.c                      Orbitron / Montserrat / 图标 / Logo 字体（预生成）
lib/TFT_eSPI/User_Setup.h          CYD ILI9341 引脚/驱动配置（由 build_flags 强制生效）
lib/lvgl/lv_conf.h                 LVGL 8.4 配置
update_firmware.sh                 合并固件打包脚本（esptool）
platformio.ini
```

### 关于 User_Setup.h 的重要说明

TFT_eSPI 库自带的 `User_Setup.h` 默认配置不是 CYD 的硬件参数。本项目通过 [platformio.ini](platformio.ini) 的 `build_flags` 用 `-include` 强制注入本地 [lib/TFT_eSPI/User_Setup.h](lib/TFT_eSPI/User_Setup.h)，并用 `-DUSER_SETUP_LOADED=1` 让库跳过其自带的默认副本。

**修改屏幕引脚/驱动只改 `lib/TFT_eSPI/User_Setup.h`，不要去改 `.pio/libdeps/` 里的副本**（那个文件不被 git 追踪，`pio pkg install` 会覆盖）。

LVGL 配置在 [lib/lvgl/lv_conf.h](lib/lvgl/lv_conf.h)，改色彩深度、字体等在此调整。

---

## 打包合并固件（用于网页刷机）

`pio run` 编译后，运行打包脚本生成单个合并固件：

```bash
./update_firmware.sh          # 默认版本号 1.1
./update_firmware.sh 1.2      # 指定版本号
```

脚本用 `esptool` 把 bootloader + 分区表 + app 合并成 `firmware/firmware.bin`，并生成 `manifest.json`。可用 esptool 直接刷：

```bash
esptool.py -p /dev/cu.usbserial-XXXX write-flash 0x0 firmware/firmware.bin
```

如需网页一键刷机（ESP Web Tools），需自行准备一个引用 `manifest.json` 的 `install.html`（本仓库未含）。

---

## 故障排查

| 现象 | 排查 |
|------|------|
| 白屏 / 花屏 / 颜色错 | 检查 [lib/TFT_eSPI/User_Setup.h](lib/TFT_eSPI/User_Setup.h) 的驱动是否为 `ILI9341_2_DRIVER`、引脚是否正确，确认 [platformio.ini](platformio.ini) 的 `-include` 行还在 |
| 触摸位置偏 | 开机时按住 BOOT 键 3 秒清空 NVS，重新进行两点触屏校准（校准数据存 NVS `touch` 命名空间） |
| WiFi 连上但无数据 | 说明配套服务器没运行或 `server_ip` 错误；看串口日志 `SSE 开始连接: <ip>:1919/sse` |
| 中文字符/图标显示成方框 | 确认 [src/fonts/](src/fonts/) 下 6 个 `.c` 字体文件齐全（在 `src/` 下会自动编译） |
| 串口无输出 / 乱码 | 波特率应为 115200；若用某些 ESP32 板子需检查 `ARDUINO_USB_CDC_ON_BOOT` 设置 |

---

## License

MIT，详见 [LICENSE](LICENSE)。
