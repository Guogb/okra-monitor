/**
 * 蓝牙配网模块 (使用 NimBLE)
 *
 * 功能说明：
 * - 通过 BLE 接收 WiFi 配置信息
 * - 与微信小程序配合使用
 * - 配置信息保存到 NVS
 *
 * 关键配置：
 * - 厂商 ID: 0x04D2 (必须与小程序一致)
 * - 服务 UUID: 53020f00-319c-4d97-a2b1-9e706baba77a
 * - 特征 UUID: f87709b3-63a7-4605-9bb5-73c383462296
 * - 数据格式: JSON {"ssid":"xxx","password":"xxx"}
 */

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// 配置常量
// ============================================================================

// 厂商 ID（必须与小程序中的 MANUFACTURER_ID 一致）
#define MANUFACTURER_ID       0x04D2

// BLE 服务和特征 UUID
#define SERVICE_UUID          "53020f00-319c-4d97-a2b1-9e706baba77a"
#define CHARACTERISTIC_UUID   "f87709b3-63a7-4605-9bb5-73c383462296"

// ============================================================================
// 配置结构
// ============================================================================

/**
 * WiFi 配置信息
 */
typedef struct {
    char ssid[32];      // WiFi 名称
    char password[64];  // WiFi 密码
    char server_ip[48]; // 服务器 IP 地址
    bool configured;    // 是否已配置
} cyd_wifi_config_t;

// ============================================================================
// 公共函数
// ============================================================================

/**
 * 初始化蓝牙配网模块
 *
 * @return true 如果已配网，false 如果需要配网
 */
bool ble_config_init(void);

/**
 * 启动 BLE 配网服务
 */
#ifdef __cplusplus
extern "C" {
#endif
void ble_config_start(void);
#ifdef __cplusplus
}
#endif

/**
 * 停止 BLE 配网服务
 */
#ifdef __cplusplus
extern "C" {
#endif
void ble_config_stop(void);
#ifdef __cplusplus
}
#endif

/**
 * 获取 WiFi 配置
 *
 * @param config 配置结构指针
 * @return true 如果配置有效，false 如果未配置
 */
bool ble_get_wifi_config(cyd_wifi_config_t *config);

/**
 * 清除 WiFi 配置
 */
void ble_clear_config(void);

/**
 * 检查是否正在配网
 *
 * @return true 如果正在等待配网
 */
bool ble_is_configuring(void);

/**
 * 检查 WiFi 是否已连接
 *
 * @return true 如果已连接
 */
bool ble_is_wifi_connected(void);

/**
 * 检查是否有 WiFi 配置
 *
 * @return true 如果有配置
 */
bool ble_has_wifi_config(void);

/**
 * 检查配置是否已更新（配网成功后调用）
 *
 * @return true 如果有新配置
 */
bool ble_is_config_updated(void);

/**
 * 清除配置更新标志
 */
void ble_clear_config_updated(void);

#endif // BLE_CONFIG_H
