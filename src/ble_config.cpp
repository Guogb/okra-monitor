/**
 * 蓝牙配网模块实现 (使用 NimBLE)
 *
 * 与微信小程序配合：
 * - 小程序扫描厂商 ID 为 0x04D2 的设备
 * - 连接后向特征值写入 JSON 配置
 * - ESP32 解析 JSON 并连接 WiFi
 */

#include "ble_config.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include "ui/ui_monitor_ultimate.h"  // 新的 UI 模块

// ============================================================================
// NVS 存储命名空间
// ============================================================================

#define NVS_NAMESPACE "wifi_config"

// ============================================================================
// 全局变量
// ============================================================================

static NimBLEServer* pServer = nullptr;
static cyd_wifi_config_t wifiConfig;
static bool isConfiguring = false;
static bool wifiConnecting = false;
static bool wifiConnected = false;

// 配置更新标志（通知 main.cpp 重新加载配置）
static bool configUpdated = false;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * 获取设备唯一短 ID（MAC 地址后6位）
 */
static String getDeviceShortId() {
    String mac = WiFi.macAddress();           // 例如: AA:BB:CC:DD:EE:FF
    mac.replace(":", "");                     // AABBCCDDEEFF
    return mac.substring(mac.length() - 6);   // 后6位：DDEEFF
}

/**
 * 保存 WiFi 配置到 NVS
 */
static void save_config(const char* ssid, const char* password, const char* server_ip) {
    Preferences preferences;

    Serial.println(">>> 开始保存 WiFi 配置到 NVS...");

    bool ok = preferences.begin(NVS_NAMESPACE, false);
    if (!ok) {
        Serial.println("❌ NVS 打开失败！");
        return;
    }

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("server_ip", server_ip);
    preferences.putBool("configured", true);

    preferences.end();

    // 更新内存中的配置
    strncpy(wifiConfig.ssid, ssid, sizeof(wifiConfig.ssid) - 1);
    wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';

    strncpy(wifiConfig.password, password, sizeof(wifiConfig.password) - 1);
    wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';

    strncpy(wifiConfig.server_ip, server_ip, sizeof(wifiConfig.server_ip) - 1);
    wifiConfig.server_ip[sizeof(wifiConfig.server_ip) - 1] = '\0';

    wifiConfig.configured = true;

    Serial.println("✅ WiFi 配置已保存到 NVS");
    Serial.printf("   SSID: %s\n", ssid);
    Serial.printf("   Server IP: %s\n", server_ip);
}

/**
 * 从 NVS 加载 WiFi 配置
 */
static bool load_config(void) {
    Preferences preferences;

    preferences.begin(NVS_NAMESPACE, true);

    bool configured = preferences.getBool("configured", false);

    if (configured) {
        String ssid = preferences.getString("ssid", "");
        String password = preferences.getString("password", "");
        String server_ip = preferences.getString("server_ip", "");

        strncpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid) - 1);
        wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';

        strncpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password) - 1);
        wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';

        strncpy(wifiConfig.server_ip, server_ip.c_str(), sizeof(wifiConfig.server_ip) - 1);
        wifiConfig.server_ip[sizeof(wifiConfig.server_ip) - 1] = '\0';

        wifiConfig.configured = true;

        Serial.printf("已加载配置: SSID=%s, ServerIP=%s\n", wifiConfig.ssid, wifiConfig.server_ip);
    } else {
        wifiConfig.configured = false;
        Serial.println("未找到配置");
    }

    preferences.end();

    return configured;
}

/**
 * 尝试连接 WiFi（非阻塞，只保存配置）
 * 实际连接在 networkTask 中处理
 */
static void tryConnectWiFi(const char* ssid, const char* password, const char* server_ip) {
    // 保存配置到内存
    strncpy(wifiConfig.ssid, ssid, sizeof(wifiConfig.ssid) - 1);
    wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';
    strncpy(wifiConfig.password, password, sizeof(wifiConfig.password) - 1);
    wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';
    strncpy(wifiConfig.server_ip, server_ip, sizeof(wifiConfig.server_ip) - 1);
    wifiConfig.server_ip[sizeof(wifiConfig.server_ip) - 1] = '\0';
    wifiConfig.configured = true;

    // 保存到 NVS
    save_config(ssid, password, server_ip);

    // 设置配置更新标志，通知 main.cpp 重新加载
    configUpdated = true;

    Serial.printf("WiFi 配置已保存: %s\n", ssid);
    Serial.printf("Server IP: %s\n", server_ip);
    Serial.println("将在后台连接...");
}

// ============================================================================
// BLE 回调类
// ============================================================================

/**
 * BLE 写特征值回调
 */
class ProvisioningCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        Serial.println(">>> BLE onWrite 回调触发 <<<");

        std::string jsonStr = pCharacteristic->getValue();
        Serial.printf("收到配网数据长度: %d\n", jsonStr.length());
        Serial.printf("收到配网数据: %s\n", jsonStr.c_str());

        if (jsonStr.length() == 0) {
            Serial.println("❌ 收到空数据");
            return;
        }

        // 解析 JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonStr);

        if (error) {
            Serial.printf("❌ JSON 解析失败: %s\n", error.c_str());
            return;
        }

        const char* ssid = doc["ssid"];
        const char* password = doc["password"];
        const char* server_ip = doc["server_ip"] | "";  // 服务器 IP 字段，默认为空

        if (ssid && password) {
            Serial.printf("SSID: %s\n", ssid);
            Serial.printf("Password: %s\n", password);
            Serial.printf("Server IP: %s\n", server_ip);
            tryConnectWiFi(ssid, password, server_ip);
        } else {
            Serial.println("❌ 缺少 ssid 或 password 字段");
            Serial.printf("ssid 存在: %s\n", ssid ? "是" : "否");
            Serial.printf("password 存在: %s\n", password ? "是" : "否");
        }
    }
};

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * 初始化蓝牙配网模块
 */
bool ble_config_init(void) {
    // 尝试加载已有配置
    if (load_config()) {
        isConfiguring = false;
        return true;
    }

    isConfiguring = true;
    return false;
}

/**
 * 启动 BLE 配网服务
 */
extern "C" void ble_config_start(void) {
    Serial.println("启动 BLE 配网服务...");

    // 如果已经初始化过，重新启动广播
    if (pServer) {
        Serial.println("BLE 已初始化，重新启动广播...");
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->start();
        Serial.println("BLE 广播已重新启动");
        isConfiguring = true;
        return;
    }

    // 初始化 NimBLE
    String deviceName = "OKRAWORKS_" + getDeviceShortId();
    NimBLEDevice::init(deviceName.c_str());  // 设备名称包含短 ID

    // 创建 BLE 服务器
    pServer = NimBLEDevice::createServer();

    // 创建 GATT 服务和特征值
    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    // 创建特征值并设置属性
    NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ
    );
    pCharacteristic->setCallbacks(new ProvisioningCallback());

    // 启动服务（必须先启动服务再开始广播）
    pService->start();
    Serial.printf("BLE 服务已启动，UUID: %s\n", SERVICE_UUID);
    Serial.printf("特征值 UUID: %s\n", CHARACTERISTIC_UUID);
    Serial.printf("特征值 handle: %d\n", pCharacteristic->getHandle());

    // 等待服务完全注册
    delay(100);

    // 配置广播数据
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);  // 启用扫描响应

    // 构造厂商自定义数据：前2字节厂商ID（小端序），后面跟设备短ID字符串
    String shortId = getDeviceShortId();
    std::string mfrData;
    mfrData += (char)(MANUFACTURER_ID & 0xFF);        // 低字节: 0xD2
    mfrData += (char)((MANUFACTURER_ID >> 8) & 0xFF); // 高字节: 0x04
    mfrData += shortId.c_str();

    // 调试：打印原始厂商数据
    Serial.println("========== BLE 广播数据 ==========");
    Serial.printf("设备名称: %s\n", deviceName.c_str());
    Serial.printf("厂商ID: 0x%04X\n", MANUFACTURER_ID);
    Serial.printf("设备短ID: %s\n", shortId.c_str());
    Serial.println("==================================");

    pAdvertising->setManufacturerData(mfrData);
    pAdvertising->start();

    Serial.printf("🔵 BLE 配网广播已启动，设备ID: %s\n", shortId.c_str());
    Serial.println("请使用微信小程序进行配网");

    isConfiguring = true;
}

/**
 * 停止 BLE 配网服务
 */
void ble_config_stop(void) {
    if (pServer) {
        NimBLEDevice::getAdvertising()->stop();
        Serial.println("蓝牙广播已停止");
    }
    isConfiguring = false;
}

/**
 * 获取 WiFi 配置
 */
bool ble_get_wifi_config(cyd_wifi_config_t *config) {
    if (wifiConfig.configured) {
        memcpy(config, &wifiConfig, sizeof(cyd_wifi_config_t));
        return true;
    }
    return false;
}

/**
 * 清除 WiFi 配置
 */
void ble_clear_config(void) {
    Preferences preferences;

    preferences.begin(NVS_NAMESPACE, false);
    preferences.clear();
    preferences.end();

    memset(&wifiConfig, 0, sizeof(wifiConfig));
    wifiConfig.configured = false;

    Serial.println("WiFi 配置已清除");
}

/**
 * 检查是否正在配网
 */
bool ble_is_configuring(void) {
    return isConfiguring;
}

/**
 * 检查 WiFi 是否已连接
 */
bool ble_is_wifi_connected(void) {
    return wifiConnected && (WiFi.status() == WL_CONNECTED);
}

/**
 * 检查配置是否已更新（配网成功后调用）
 * @return true 如果有新配置
 */
bool ble_is_config_updated(void) {
    return configUpdated;
}

/**
 * 清除配置更新标志
 */
void ble_clear_config_updated(void) {
    configUpdated = false;
}

/**
 * 检查是否有 WiFi 配置
 */
bool ble_has_wifi_config(void) {
    return wifiConfig.configured;
}
