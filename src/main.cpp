/**
 * @file main.cpp
 * @brief ESP32 主程序入口
 *
 * 架构说明：
 * - Core 1 (loop): LVGL UI 刷新 + 触摸屏处理
 * - Core 0 (networkTask): WiFi 连接 + SSE 数据获取
 *
 * 使用 FreeRTOS 任务分离 UI 和网络操作，确保 UI 响应不受网络阻塞影响
 */

#include <Arduino.h>      // ESP32 Arduino 核心库
#include <SPI.h>          // SPI 通信库
#include <WiFi.h>         // WiFi 库
#include <nvs_flash.h>    // NVS 非易失性存储
#include <TFT_eSPI.h>     // TFT 屏幕驱动库
#include <XPT2046_Touchscreen.h>  // 触摸屏驱动
#include "ui/ui_monitor_ultimate.h"  // 新的UI模块
#include "ble_config.h"   // 蓝牙配网模块
#include "data_fetcher.h" // 数据获取模块

// FreeRTOS 任务相关
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// NTP 时间同步
#include <time.h>
#include <lwip/apps/sntp.h>

/* ============================================================================
 * 触摸屏配置
 * ============================================================================ */

// CYD 触摸屏引脚（使用 HSPI）
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

// 触摸校准参数
typedef struct {
    float xCalM;
    float yCalM;
    float xCalC;
    float yCalC;
} TouchCalibration;

static TouchCalibration touchCal = {1.0f, 1.0f, 0.0f, 0.0f};
static bool touchCalibrated = false;

/* ============================================================================
 * 全局变量
 * ============================================================================ */

/** TFT_eSPI 显示对象（硬件驱动） */
TFT_eSPI tft = TFT_eSPI();

/** WiFi 配置信息 */
static cyd_wifi_config_t wifiConfig;

/** WiFi 连接状态 */
static bool wifiConnected = false;

/** LVGL 显示缓冲区 */
static lv_disp_draw_buf_t draw_buf;

/** 颜色缓冲区（20行，提高刷新效率） */
static lv_color_t buf[320 * 20];

/** LVGL 输入设备驱动 */
static lv_indev_drv_t indev_drv;

/** 网络任务句柄 */
static TaskHandle_t networkTaskHandle = NULL;

/** UI 更新数据（从网络任务传递到主循环） */
static volatile wifi_state_t ui_wifi_state = WIFI_STATE_DISCONNECTED;
static volatile server_state_t ui_server_state = SERVER_STATE_DISCONNECTED;
static volatile int ui_time_hour = 0;
static volatile int ui_time_min = 0;
static volatile bool ui_time_updated = false;
static volatile bool ui_wifi_state_changed = false;
static volatile bool ui_server_state_changed = false;
static monitor_data_t ui_monitor_data;
static volatile bool ui_data_updated = false;

/* ============================================================================
 * 前置声明
 * ============================================================================ */

void networkTask(void *parameter);

/* ============================================================================
 * 显示驱动回调
 * ============================================================================ */

/**
 * @brief LVGL 刷新回调
 *
 * 当 LVGL 完成一帧的绘制后调用此函数，将像素数据发送到 TFT 屏幕。
 *
 * @param disp    显示驱动设备
 * @param area    要刷新的区域（坐标范围）
 * @param color_p 颜色数据指针
 *
 * @note 这是 ESP32 特有的实现，使用 TFT_eSPI 的 SPI 传输
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    /* 计算刷新区域的宽高 */
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    /* 开始 SPI 传输 */
    tft.startWrite();

    /* 设置 TFT GRAM 写入地址窗口 */
    tft.setAddrWindow(area->x1, area->y1, w, h);

    /* 通过 SPI 发送像素数据到 TFT */
    /* pushColors 会自动处理字节序转换 */
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);

    /* 结束 SPI 传输 */
    tft.endWrite();

    /* 通知 LVGL 刷新完成，可以继续下一帧 */
    lv_disp_flush_ready(disp);
}

/* ============================================================================
 * 触摸屏校准
 * ============================================================================ */

/**
 * @brief 执行触摸屏校准
 *
 * 校准流程：
 * 1. 显示左上角十字标记，等待点击
 * 2. 显示右下角十字标记，等待点击
 * 3. 计算校准参数并保存到 NVS
 */
void touch_calibrate() {
    Serial.println("开始触摸屏校准...");
    Serial.println("请点击屏幕上出现的十字标记");

    TS_Point p;
    int16_t x1, y1, x2, y2;

    // 清屏
    tft.fillScreen(TFT_BLACK);

    // 第一个点：左上角 (20, 20)
    tft.drawFastHLine(10, 20, 20, TFT_WHITE);
    tft.drawFastVLine(20, 10, 20, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(30, 25);
    tft.print("点击此处");

    Serial.println("请点击左上角十字标记...");
    while (!touch.tirqTouched() || !touch.touched()) {
        delay(10);
    }
    delay(50);  // 稳定一下
    p = touch.getPoint();
    x1 = p.x;
    y1 = p.y;
    Serial.printf("左上角原始坐标: x=%d, y=%d\n", x1, y1);

    // 清除第一个标记
    tft.drawFastHLine(10, 20, 20, TFT_BLACK);
    tft.drawFastVLine(20, 10, 20, TFT_BLACK);
    tft.fillRect(30, 25, 80, 10, TFT_BLACK);

    // 等待松开
    while (touch.touched()) {
        delay(10);
    }
    delay(500);

    // 第二个点：右下角 (300, 220)
    tft.drawFastHLine(290, 220, 20, TFT_WHITE);
    tft.drawFastVLine(300, 210, 20, TFT_WHITE);
    tft.setCursor(220, 215);
    tft.print("点击此处");

    Serial.println("请点击右下角十字标记...");
    while (!touch.tirqTouched() || !touch.touched()) {
        delay(10);
    }
    delay(50);
    p = touch.getPoint();
    x2 = p.x;
    y2 = p.y;
    Serial.printf("右下角原始坐标: x=%d, y=%d\n", x2, y2);

    // 清除第二个标记
    tft.drawFastHLine(290, 220, 20, TFT_BLACK);
    tft.drawFastVLine(300, 210, 20, TFT_BLACK);
    tft.fillRect(220, 215, 80, 10, TFT_BLACK);

    // 计算校准参数
    // 屏幕坐标差值：280 (320-40) 和 200 (240-40)
    int16_t xDist = 320 - 40;
    int16_t yDist = 240 - 40;

    touchCal.xCalM = (float)xDist / (float)(x2 - x1);
    touchCal.xCalC = 20.0 - ((float)x1 * touchCal.xCalM);
    touchCal.yCalM = (float)yDist / (float)(y2 - y1);
    touchCal.yCalC = 20.0 - ((float)y1 * touchCal.yCalM);

    Serial.printf("校准参数: xM=%.3f, xC=%.3f, yM=%.3f, yC=%.3f\n",
                  touchCal.xCalM, touchCal.xCalC, touchCal.yCalM, touchCal.yCalC);

    // 保存到 NVS
    nvs_handle_t handle;
    if (nvs_open("touch", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "cal", &touchCal, sizeof(touchCal));
        nvs_commit(handle);
        nvs_close(handle);
        Serial.println("校准参数已保存");
    }

    touchCalibrated = true;

    // 显示校准完成
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(80, 110);
    tft.print("Calibrated!");
    delay(1000);
    tft.fillScreen(TFT_BLACK);

    Serial.println("触摸屏校准完成");
}

/**
 * @brief 从 NVS 加载校准参数
 */
bool load_touch_calibration() {
    nvs_handle_t handle;
    if (nvs_open("touch", NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t size = sizeof(touchCal);
    if (nvs_get_blob(handle, "cal", &touchCal, &size) != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    touchCalibrated = true;
    Serial.printf("已加载校准参数: xM=%.3f, xC=%.3f, yM=%.3f, yC=%.3f\n",
                  touchCal.xCalM, touchCal.xCalC, touchCal.yCalM, touchCal.yCalC);
    return true;
}

/* ============================================================================
 * 触摸屏驱动回调
 * ============================================================================ */

/**
 * @brief LVGL 触摸读取回调
 */
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    // 只检查 touched()，不检查 tirqTouched()
    if (touch.touched()) {
        TS_Point p = touch.getPoint();

        // 使用校准参数转换坐标
        int16_t x = (int16_t)((p.x * touchCal.xCalM) + touchCal.xCalC);
        int16_t y = (int16_t)((p.y * touchCal.yCalM) + touchCal.yCalC);

        // 确保坐标在屏幕范围内
        if (x < 0) x = 0;
        if (x > 319) x = 319;
        if (y < 0) y = 0;
        if (y > 239) y = 239;

        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* ============================================================================
 * WiFi 连接函数
 * ============================================================================ */

/**
 * @brief 连接 WiFi（阻塞版本，仅用于 setup）
 *
 * @param ssid WiFi 名称
 * @param password WiFi 密码
 *
 * @return true 如果连接成功
 */
bool connect_wifi(const char* ssid, const char* password) {
    Serial.printf("连接 WiFi: %s\n", ssid);

    WiFi.begin(ssid, password);

    // 等待连接（最多 10 秒）
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi 连接成功!");
        Serial.printf("IP 地址: %s\n", WiFi.localIP().toString().c_str());
        wifiConnected = true;

        // 初始化 NTP 时间同步（中国时区 UTC+8）
        configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
        Serial.println("NTP 时间同步已启动");

        return true;
    } else {
        Serial.println("\nWiFi 连接失败!");
        wifiConnected = false;
        return false;
    }
}

/* ============================================================================
 * Arduino 生命周期
 * ============================================================================ */

/**
 * @brief 初始化函数
 *
 * Arduino 上电或复位后调用一次，执行所有初始化工作。
 */
void setup()
{
    /* 初始化串口用于调试输出 */
    Serial.begin(115200);
    Serial.println("\n========================================");
    Serial.println("CYD Monitor 启动中...");
    Serial.println("========================================");

    /* 初始化 NVS（非易失性存储） */
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        Serial.printf("NVS 初始化失败: %s, 正在擦除...\n", esp_err_to_name(ret));
        nvs_flash_erase();
        ret = nvs_flash_init();
        if (ret == ESP_OK) {
            Serial.println("NVS 擦除并重新初始化成功");
        } else {
            Serial.printf("NVS 初始化仍然失败: %s\n", esp_err_to_name(ret));
        }
    } else {
        Serial.println("NVS 初始化完成");
    }

    /* 初始化背光 (GPIO 21) */
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    Serial.println("背光已开启");

    /* 初始化 TFT 屏幕 */
    Serial.println("初始化 TFT...");
    tft.init();
    tft.setRotation(1);
    Serial.printf("TFT 初始化完成: %dx%d\n", tft.width(), tft.height());

    /* 显示白色背景 */
    tft.fillScreen(TFT_WHITE);

    /* 初始化 BOOT 按钮 (GPIO 0) */
    pinMode(0, INPUT_PULLUP);

    /* 提前初始化 LVGL 以显示 Logo */
    lv_init();

    /* 初始化显示缓冲区 */
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * 10);

    /* 注册显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* 显示 Logo */
    LV_FONT_DECLARE(logo_40);
    lv_obj_t *logo_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(logo_label, &logo_40, 0);
    lv_obj_set_style_text_color(logo_label, lv_color_black(), 0);
    lv_label_set_text(logo_label, "\xEE\x98\x9D");  // 字符 58909
    lv_obj_align(logo_label, LV_ALIGN_CENTER, 0, -20);

    /* 显示提示文字 */
    lv_obj_t *hint_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hint_label, lv_color_black(), 0);
    lv_label_set_text(hint_label, "Press BOOT to clear NVS");
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    // 刷新显示
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(10);
    }

    /* 等待 3 秒，检测 BOOT 按钮按下 */
    Serial.println(">>> 等待 3 秒，按 BOOT 清除 NVS <<<");
    unsigned long startWait = millis();
    while (millis() - startWait < 3000) {
        if (digitalRead(0) == LOW) {  // BOOT 按钮按下
            Serial.println(">>> 检测到 BOOT 按下，清除 NVS <<<");
            nvs_flash_erase();
            nvs_flash_init();

            tft.fillScreen(TFT_BLACK);
            tft.setTextSize(2);
            tft.setCursor(80, 100);
            tft.print("NVS Cleared!");
            delay(1500);

            ESP.restart();
        }
        lv_timer_handler();  // 保持 LVGL 刷新
        delay(50);
    }

    Serial.println("3 秒等待结束，继续启动...");
    for (int i = 0; i < 10; i++) {
        lv_timer_handler();
        delay(10);
    }

    /* 初始化触摸屏 */
    Serial.println("初始化触摸屏...");
    touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    if (touch.begin(touchSPI)) {
        Serial.println("触摸屏初始化成功");
    } else {
        Serial.println("触摸屏初始化失败!");
    }

    /* 检查是否需要校准 */
    if (!load_touch_calibration()) {
        Serial.println("未找到触摸校准数据，开始校准...");
        touch_calibrate();
    } else {
        Serial.println("触摸屏已校准");
    }

    /* 注册触摸输入设备 */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_t *registered_indev = lv_indev_drv_register(&indev_drv);

    if (registered_indev) {
        Serial.println("触摸输入设备注册成功");
    } else {
        Serial.println("触摸输入设备注册失败!");
    }

    /* 初始化 UI 界面（先显示界面） */
    ui_monitor_init_ultimate();
    Serial.println("UI 初始化完成");

    /* 初始化蓝牙配网模块 */
    bool hasConfig = ble_config_init();

    if (hasConfig) {
        /* 有配置，尝试连接 WiFi */
        if (ble_get_wifi_config(&wifiConfig)) {
            Serial.printf("已加载配置: %s\n", wifiConfig.ssid);
            // WiFi 连接放到 loop 中处理，不阻塞 UI 显示
        }
    } else {
        Serial.println("未找到 WiFi 配置");
    }

    /* 设置初始 WiFi 状态指示器颜色 */
    if (!hasConfig) {
        // 灰色 - 无配置
        ui_monitor_set_wifi_state(WIFI_STATE_DISCONNECTED);
    } else {
        // 灰绿闪烁 - 有配置，准备连接
        ui_monitor_set_wifi_state(WIFI_STATE_CONNECTING);
        wifiConnected = false;  // 标记为未连接，loop 中会触发连接
    }

    Serial.println("========================================");
    Serial.println("CYD Monitor 初始化完成！");
    Serial.println("点击右上角状态指示器可打开配网页面");
    Serial.println("========================================");

    /* 创建网络任务（运行在 Core 0） */
    xTaskCreatePinnedToCore(
        networkTask,           // 任务函数
        "NetworkTask",         // 任务名称
        12288,                 // 栈大小（12KB）
        NULL,                  // 任务参数
        2,                     // 优先级（提高）
        &networkTaskHandle,    // 任务句柄
        0                      // Core 0
    );

    Serial.println("网络任务已创建 (Core 0)");
}

/* ============================================================================
 * 网络任务（运行在 Core 0）
 * ============================================================================ */

/**
 * @brief 网络任务函数
 *
 * 运行在 Core 0，处理所有网络相关操作：
 * - WiFi 连接管理
 * - SSE 连接管理
 * - 数据获取
 *
 * @param parameter 任务参数（未使用）
 */
void networkTask(void *parameter) {
    Serial.println("网络任务启动 (Core 0)");

    // WiFi 连接管理状态
    static bool wifiConnecting = false;
    static unsigned long lastConnectAttempt = 0;
    static bool firstConnect = true;

    // SSE 连接管理状态
    static bool sse_initialized = false;
    static unsigned long lastReconnectAttempt = 0;

    // 时间更新
    static unsigned long lastTimeUpdate = 0;

    while (true) {
        // 检查配置是否更新（配网成功后）
        if (ble_is_config_updated()) {
            Serial.println(">>> 检测到新配置，重新加载...");
            ble_clear_config_updated();  // 清除标志

            // 重新加载配置
            if (ble_get_wifi_config(&wifiConfig)) {
                Serial.printf(">>> 新配置: SSID=%s, ServerIP=%s, configured=%d\n",
                    wifiConfig.ssid, wifiConfig.server_ip, wifiConfig.configured);

                // 断开当前 WiFi 连接
                WiFi.disconnect();
                wifiConnected = false;
                wifiConnecting = false;
                firstConnect = true;
                lastConnectAttempt = 0;  // 重置连接计时

                // 关闭旧 SSE 连接
                if (sse_initialized) {
                    sse_close();
                    sse_initialized = false;
                }
            } else {
                Serial.println(">>> 错误: ble_get_wifi_config 返回 false!");
            }
        }

        // 检查 WiFi 连接状态
        static bool lastWifiState = false;
        bool currentWifiState = (WiFi.status() == WL_CONNECTED);

        if (currentWifiState != lastWifiState) {
            if (currentWifiState) {
                Serial.println("========================================");
                Serial.println("✅ WiFi 已连接");
                Serial.printf("   SSID: %s\n", wifiConfig.ssid);
                Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
                Serial.printf("   Server IP: %s\n", wifiConfig.server_ip);
                Serial.println("========================================");
                ui_wifi_state = WIFI_STATE_CONNECTED;
                ui_wifi_state_changed = true;

                // 初始化 NTP 时间同步
                configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
            } else {
                Serial.println("========================================");
                Serial.println("❌ WiFi 已断开");
                Serial.println("========================================");
                ui_wifi_state = WIFI_STATE_DISCONNECTED;
                ui_wifi_state_changed = true;
            }
            lastWifiState = currentWifiState;
            wifiConnected = currentWifiState;
        }

        // WiFi 连接管理
        if (wifiConfig.configured && !wifiConnected && !wifiConnecting) {
            if (firstConnect || millis() - lastConnectAttempt > 10000) {
                if (firstConnect) {
                    Serial.println("首次启动，尝试连接 WiFi...");
                    firstConnect = false;
                } else {
                    Serial.println("WiFi 断开，尝试重连...");
                }
                lastConnectAttempt = millis();
                wifiConnecting = true;
                ui_wifi_state = WIFI_STATE_CONNECTING;
                ui_wifi_state_changed = true;
                Serial.printf(">>> 调用 WiFi.begin(%s, %s)\n", wifiConfig.ssid, wifiConfig.password);
                WiFi.begin(wifiConfig.ssid, wifiConfig.password);
            }
        } else if (!wifiConfig.configured) {
            // 调试：为什么没有连接
            static unsigned long lastDebugTime = 0;
            if (millis() - lastDebugTime > 5000) {
                lastDebugTime = millis();
                Serial.println(">>> 调试: wifiConfig.configured = false，等待配网...");
            }
        }

        // 检查 WiFi 连接结果
        if (wifiConnecting && WiFi.status() == WL_CONNECTED) {
            wifiConnecting = false;
            wifiConnected = true;
        }

        // WiFi 连接超时（10秒）
        if (wifiConnecting && millis() - lastConnectAttempt > 10000) {
            Serial.println("WiFi 连接超时");
            wifiConnecting = false;
        }

        // 时间更新（每秒）
        if (WiFi.status() == WL_CONNECTED && millis() - lastTimeUpdate > 1000) {
            lastTimeUpdate = millis();
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                ui_time_hour = timeinfo.tm_hour;
                ui_time_min = timeinfo.tm_min;
                ui_time_updated = true;
            }
        }

        // SSE 连接管理
        if (WiFi.status() == WL_CONNECTED && wifiConfig.configured) {
            // 启动 SSE 连接
            if (!sse_initialized && !sse_is_connecting()) {
                if (millis() - lastReconnectAttempt > 3000) {
                    lastReconnectAttempt = millis();
                    sse_begin_connect(wifiConfig.server_ip);
                    ui_server_state = SERVER_STATE_CONNECTING;
                    ui_server_state_changed = true;
                }
            }

            // 处理 SSE 连接过程
            if (sse_is_connecting()) {
                if (sse_process()) {
                    sse_initialized = true;
                    ui_server_state = SERVER_STATE_CONNECTED;
                    ui_server_state_changed = true;
                }
            }

            // 检查 SSE 断开
            if (sse_initialized && !sse_is_connected()) {
                sse_initialized = false;
                ui_server_state = SERVER_STATE_DISCONNECTED;
                ui_server_state_changed = true;
            }

            // 读取 SSE 数据
            if (sse_initialized) {
                monitor_data_t monitorData;
                if (sse_read_data(&monitorData)) {
                    ui_monitor_data = monitorData;
                    ui_data_updated = true;
                }
            }
        } else {
            // WiFi 断开，关闭 SSE
            if (sse_initialized || sse_is_connecting()) {
                sse_close();
                sse_initialized = false;
            }
        }

        // 任务延时 50ms（降低 CPU 占用）
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief 主循环（运行在 Core 1）
 *
 * 专注于 LVGL UI 刷新和触摸屏处理
 * 从缓存变量读取网络任务的状态更新，避免跨核 UI 调用
 */
void loop()
{
    /* 处理 LVGL 定时器 */
    lv_timer_handler();

    /* 更新 WiFi 状态（从缓存读取） */
    if (ui_wifi_state_changed) {
        ui_monitor_set_wifi_state((wifi_state_t)ui_wifi_state);
        ui_wifi_state_changed = false;
    }

    /* 更新服务器状态（从缓存读取） */
    if (ui_server_state_changed) {
        ui_monitor_set_server_state((server_state_t)ui_server_state);
        ui_server_state_changed = false;
    }

    /* 更新时间显示（从缓存读取） */
    if (ui_time_updated) {
        ui_monitor_set_time(ui_time_hour, ui_time_min);
        ui_time_updated = false;
    }

    /* 更新监控数据（从缓存读取） */
    if (ui_data_updated) {
        ui_monitor_update_ultimate(
            ui_monitor_data.cpu_usage,
            ui_monitor_data.cpu_temp,
            ui_monitor_data.cpu_volt,
            ui_monitor_data.cpu_fan,
            ui_monitor_data.gpu_core_usage,
            ui_monitor_data.gpu_vram_usage,
            ui_monitor_data.gpu_temp,
            ui_monitor_data.gpu_fan,
            ui_monitor_data.fps,
            ui_monitor_data.mem_usage,
            ui_monitor_data.mem_freq,
            ui_monitor_data.mem_volt,
            ui_monitor_data.net_down,
            ui_monitor_data.net_up,
            (wifi_state_t)ui_wifi_state
        );
        ui_data_updated = false;
    }

    /* 短暂休眠，控制刷新频率 */
    delay(5);
}