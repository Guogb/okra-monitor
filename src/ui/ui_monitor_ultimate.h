#ifndef UI_MONITOR_ULTIMATE_H
#define UI_MONITOR_ULTIMATE_H

#ifdef __cplusplus
extern "C" { /* Use C linkage for C++ */
#endif

#include "lvgl.h"

// 外部字体声明（需在项目中包含转换后的字体文件）
// 建议使用 LVGL Font Converter 将 .ttf 换换为 C 数组文件
extern const lv_font_t font_orbitron_12;    // 例如：Orbitron Bold 12px for titles/values
extern const lv_font_t font_orbitron_10;    // 例如：Orbitron Bold 10px for values
extern const lv_font_t font_montserrat_10;  // 例如：Montserrat Regular 10px for labels
extern const lv_font_t font_montserrat_8;   // 例如：Montserrat Regular 8px for smaller labels
extern const lv_font_t font_iconfont_12;    // 例如：图标字体 12px

// WiFi 状态枚举
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED
} wifi_state_t;

// 服务器连接状态枚举
typedef enum {
    SERVER_STATE_DISCONNECTED = 0,
    SERVER_STATE_CONNECTING,
    SERVER_STATE_CONNECTED
} server_state_t;

/**
 * @brief 初始化 OKRA MONITOR UI
 * 在 LVGL 显示器和输入设备初始化后调用。
 */
void ui_monitor_init_ultimate(void);

/**
 * @brief 更新服务器连接状态指示器
 * @param state 服务器连接状态 (server_state_t)
 */
void ui_monitor_set_server_state(server_state_t state);

/**
 * @brief 更新 WiFi 状态指示器
 * @param state WiFi 连接状态 (wifi_state_t)
 */
void ui_monitor_set_wifi_state(wifi_state_t state);

/**
 * @brief 更新时间显示
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 */
void ui_monitor_set_time(int hour, int minute);

/**
 * @brief 显示配网页面
 */
void ui_monitor_show_config_page(void);

/**
 * @brief 隐藏配网页面
 */
void ui_monitor_hide_config_page(void);

/**
 * @brief 更新 OKRA MONITOR UI 上的数据
 * @param cpu_usage CPU 使用率 (0-100)
 * @param cpu_temp CPU 温度 (摄氏度)
 * @param cpu_volt CPU 电压 (V)
 * @param cpu_fan CPU 风扇转速 (0-100%)
 * @param gpu_core_usage GPU 核心使用率 (0-100)
 * @param gpu_vram_usage GPU 显存使用率 (0-100)
 * @param gpu_temp GPU 温度 (摄氏度)
 * @param gpu_fan GPU 风扇转速 (0-100%)
 * @param fps 帧率
 * @param mem_usage 内存使用率 (0-100)
 * @param mem_freq 内存频率 (MHz)
 * @param mem_volt 内存电压 (V)
 * @param net_down 网络下行速率 (MB/s)
 * @param net_up 网络上行速率 (MB/s)
 * @param wifi_state WiFi 连接状态 (wifi_state_t)
 */
void ui_monitor_update_ultimate(
    float cpu_usage, float cpu_temp, float cpu_volt, int cpu_fan,
    float gpu_core_usage, float gpu_vram_usage, float gpu_temp, int gpu_fan, int fps,
    float mem_usage, int mem_freq, float mem_volt,
    float net_down, float net_up,
    wifi_state_t wifi_state
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_MONITOR_ULTIMATE_H */