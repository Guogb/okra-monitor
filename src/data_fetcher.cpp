/**
 * @file data_fetcher.cpp
 * @brief 数据获取模块实现 - 使用 SSE 长连接
 */

#include <Arduino.h>
#include <WiFi.h>
#include "data_fetcher.h"

// SSE 连接状态
static WiFiClient sse_client;
static bool sse_connected = false;
static String sse_buffer;

// 非阻塞连接状态
static enum {
    SSE_IDLE,           // 空闲
    SSE_CONNECTING,     // TCP 连接中
    SSE_WAITING_HEADER, // 等待 HTTP 响应头
    SSE_READY           // 连接就绪
} sse_state = SSE_IDLE;
static unsigned long sse_timeout = 0;
static String sse_server_ip;

// 从 "cpu_usage: 1%" 格式中提取数值
static float extract_value(const String& text) {
    // 找到冒号后的空格
    int colon_pos = text.indexOf(':');
    if (colon_pos < 0) return 0;

    String value_part = text.substring(colon_pos + 1);
    value_part.trim();

    // 提取数字部分
    int num_end = 0;
    for (int i = 0; i < value_part.length(); i++) {
        if (isdigit(value_part[i]) || value_part[i] == '.' || value_part[i] == '-') {
            num_end = i;
        } else {
            break;
        }
    }

    if (num_end == 0 && !isdigit(value_part[0])) return 0;

    String num_str = value_part.substring(0, num_end + 1);
    return num_str.toFloat();
}

// 从 "cpu_usage: 1%" 格式中提取 key
static String extract_key(const String& text) {
    int colon_pos = text.indexOf(':');
    if (colon_pos < 0) return "";
    String key = text.substring(0, colon_pos);
    key.trim();
    return key;
}

// 解析 SSE 数据行
// 格式: data: Page0|{|}Simple1|cpu_usage: 2%{|}Simple2|cpu_temp: 39°C{|}...
static void parse_sse_data(const String& sse_line, monitor_data_t* data) {
    int data_pos = sse_line.indexOf("data: ");
    if (data_pos < 0) {
        return;
    }

    String content = sse_line.substring(data_pos + 6);  // 跳过 "data: "

    // 删除 "Page0|"
    int page_pos = content.indexOf("Page0|");
    if (page_pos >= 0) {
        content = content.substring(page_pos + 6);
    }

    // 循环提取 {|} 之间的内容
    int count = 0;
    while (content.length() > 0) {
        // 找第一个 {|}
        int start = content.indexOf("{|}");
        if (start < 0) break;

        // 从 start+3 开始找下一个 {|}
        int end = content.indexOf("{|}", start + 3);

        String segment;
        if (end < 0) {
            // 没有下一个 {|}，取剩余部分
            segment = content.substring(start + 3);
            content = "";
        } else {
            // 提取两个 {|} 之间的内容
            segment = content.substring(start + 3, end);
            content = content.substring(end);  // 从下一个 {|} 开始继续
        }

        if (segment.length() == 0) break;

        // 用 | 分割，取 [1]
        int pipe_pos = segment.indexOf('|');
        if (pipe_pos >= 0) {
            String value_text = segment.substring(pipe_pos + 1);  // "cpu_usage: 2%"

            // 用 : 分割出 key 和 value
            int colon_pos = value_text.indexOf(':');
            if (colon_pos >= 0) {
                String key = value_text.substring(0, colon_pos);
                key.trim();

                String val_str = value_text.substring(colon_pos + 1);
                val_str.trim();

                // 提取数字
                float value = 0;
                for (int i = 0; i < val_str.length(); i++) {
                    if (isdigit(val_str[i]) || val_str[i] == '.' || val_str[i] == '-') {
                        int num_end = i;
                        for (int j = i; j < val_str.length(); j++) {
                            if (isdigit(val_str[j]) || val_str[j] == '.' || val_str[j] == '-') {
                                num_end = j;
                            } else {
                                break;
                            }
                        }
                        String num_str = val_str.substring(i, num_end + 1);
                        value = num_str.toFloat();
                        break;
                    }
                }

                // 匹配 key
                if (key == "cpu_usage") data->cpu_usage = value;
                else if (key == "cpu_temp") data->cpu_temp = value;
                else if (key == "cpu_fun") data->cpu_fan = (int)value;
                else if (key == "cpu_volt") data->cpu_volt = value;
                else if (key == "gpu_core_usage") data->gpu_core_usage = value;
                else if (key == "gpu_temp") data->gpu_temp = value;
                else if (key == "gpu_vram_usage") data->gpu_vram_usage = value;
                else if (key == "gpu_fun") data->gpu_fan = (int)value;
                else if (key == "fps") data->fps = (int)value;
                else if (key == "mem_usage") data->mem_usage = value;
                else if (key == "mem_freq") data->mem_freq = (int)(value * 2);  // 内存频率 *2
                else if (key == "mem_volt") data->mem_volt = value;
                else if (key == "net_down") data->net_down = value;
                else if (key == "net_up") data->net_up = value;
            }
        }
    }
}

/**
 * @brief 启动 SSE 连接（非阻塞）
 *
 * 调用后立即返回，需要通过 sse_process() 继续处理连接过程
 *
 * @param server_ip 服务器IP地址
 */
void sse_begin_connect(const char* server_ip) {
    if (WiFi.status() != WL_CONNECTED) {
        sse_state = SSE_IDLE;
        sse_connected = false;
        return;
    }

    // 关闭旧连接
    if (sse_connected || sse_state != SSE_IDLE) {
        sse_client.stop();
        sse_connected = false;
        sse_state = SSE_IDLE;
    }

    sse_server_ip = String(server_ip);
    Serial.printf("SSE 开始连接: %s:1919/sse\n", server_ip);

    // 设置超时为 500ms（减少阻塞时间）
    sse_client.setTimeout(500);

    // 启动连接
    if (sse_client.connect(server_ip, 1919)) {
        // 发送 HTTP 请求
        sse_client.print(String("GET /sse HTTP/1.1\r\n") +
                         "Host: " + String(server_ip) + ":1919\r\n" +
                         "Accept: text/event-stream\r\n" +
                         "Cache-Control: no-cache\r\n" +
                         "Connection: keep-alive\r\n\r\n");
        sse_state = SSE_WAITING_HEADER;
        sse_timeout = millis() + 3000;  // 3秒超时
        Serial.println("SSE HTTP 请求已发送，等待响应...");
    } else {
        Serial.println("SSE TCP 连接失败");
        sse_state = SSE_IDLE;
        sse_connected = false;
    }
}

/**
 * @brief 处理 SSE 连接过程（非阻塞）
 *
 * 每次调用处理一小部分工作，确保不阻塞 UI
 *
 * @return true 如果连接成功，false 如果还在连接中或失败
 */
bool sse_process(void) {
    switch (sse_state) {
        case SSE_IDLE:
            return false;

        case SSE_WAITING_HEADER:
            // 检查超时
            if (millis() > sse_timeout) {
                Serial.println("SSE 响应超时");
                sse_client.stop();
                sse_state = SSE_IDLE;
                sse_connected = false;
                return false;
            }

            // 检查是否有响应
            if (sse_client.available() > 0) {
                // 跳过 HTTP 响应头（读到空行为止）
                while (sse_client.available()) {
                    String line = sse_client.readStringUntil('\n');
                    if (line.length() <= 2) {  // "\r" 或 ""
                        break;
                    }
                }
                sse_state = SSE_READY;
                sse_connected = true;
                sse_buffer = "";
                Serial.println("SSE 连接成功，等待数据...");
                return true;
            }
            return false;

        case SSE_READY:
            return sse_connected;

        default:
            return false;
    }
}

/**
 * @brief 检查 SSE 是否正在连接中
 */
bool sse_is_connecting(void) {
    return sse_state == SSE_CONNECTING || sse_state == SSE_WAITING_HEADER;
}

bool sse_init(const char* server_ip) {
    // 兼容旧接口：启动非阻塞连接
    sse_begin_connect(server_ip);
    return sse_connected;
}

bool sse_is_connected() {
    return sse_connected && sse_client.connected();
}

bool sse_read_data(monitor_data_t* data) {
    if (!sse_connected || !sse_client.connected()) {
        sse_connected = false;
        return false;
    }

    // 非阻塞读取
    while (sse_client.available()) {
        char c = sse_client.read();
        if (c == '\n') {
            // 检测 HTTP 响应头开始
            if (sse_buffer.startsWith("HTTP/1.1")) {
                // 跳过响应头，读到空行为止
                while (sse_client.available()) {
                    String line = sse_client.readStringUntil('\n');
                    if (line.length() <= 2) {
                        break;
                    }
                }
                sse_buffer = "";
                continue;
            }

            if (sse_buffer.startsWith("data: ")) {
                parse_sse_data(sse_buffer, data);
                sse_buffer = "";
                return true;
            }
            sse_buffer = "";
        } else if (c != '\r') {
            sse_buffer += c;
        }
    }

    return false;
}

void sse_close() {
    if (sse_connected || sse_state != SSE_IDLE) {
        sse_client.stop();
        sse_connected = false;
        sse_state = SSE_IDLE;
        sse_buffer = "";
        Serial.println("SSE 连接已关闭");
    }
}

WiFiClient* sse_get_client() {
    return &sse_client;
}
