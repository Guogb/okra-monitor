/**
 * @file data_fetcher.h
 * @brief 数据获取模块 - 使用 SSE 长连接获取硬件监控数据
 */

#ifndef DATA_FETCHER_H
#define DATA_FETCHER_H

#include <Arduino.h>
#include <WiFiClient.h>

#ifdef __cplusplus
extern "C" {
#endif

// 监控数据结构
typedef struct {
    // CPU
    float cpu_usage;
    float cpu_temp;
    float cpu_volt;
    int cpu_fan;

    // GPU
    float gpu_core_usage;
    float gpu_vram_usage;
    float gpu_temp;
    int gpu_fan;
    int fps;

    // Memory
    float mem_usage;
    int mem_freq;
    float mem_volt;

    // Network
    float net_down;
    float net_up;

} monitor_data_t;

/**
 * @brief 启动 SSE 连接（非阻塞）
 *
 * 调用后立即返回，需要通过 sse_process() 继续处理连接过程
 *
 * @param server_ip 服务器IP地址
 */
void sse_begin_connect(const char* server_ip);

/**
 * @brief 处理 SSE 连接过程（非阻塞）
 *
 * 每次调用处理一小部分工作，确保不阻塞 UI
 *
 * @return true 如果连接成功，false 如果还在连接中或失败
 */
bool sse_process(void);

/**
 * @brief 检查 SSE 是否正在连接中
 */
bool sse_is_connecting(void);

/**
 * @brief 初始化 SSE 连接（兼容旧接口，已改为非阻塞）
 * @param server_ip 服务器IP地址
 * @return true 如果连接成功（立即返回时通常为 false）
 */
bool sse_init(const char* server_ip);

/**
 * @brief 检查 SSE 连接状态
 * @return true 如果连接正常
 */
bool sse_is_connected(void);

/**
 * @brief 尝试读取 SSE 数据（非阻塞）
 * @param data 数据结构指针
 * @return true 如果有新数据并解析成功
 */
bool sse_read_data(monitor_data_t* data);

/**
 * @brief 关闭 SSE 连接
 */
void sse_close(void);

#ifdef __cplusplus
}
#endif

/**
 * @brief 获取底层 WiFiClient（用于检测断开）
 * @return WiFiClient 指针（仅 C++ 可用）
 */
WiFiClient* sse_get_client();

#endif // DATA_FETCHER_H