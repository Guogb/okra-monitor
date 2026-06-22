#include "ui_monitor_ultimate.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"  // ESP32 WiFi MAC 地址

#if LV_USE_QRCODE
#include "src/extra/libs/qrcode/lv_qrcode.h"
#endif

/*
 * OKRA MONITOR - ESP32 LVGL UI V13 (最终版)
 * 适配分辨率: 320x240
 */

/* ============================================================================
 * 颜色定义
 * ============================================================================ */

#define COLOR_BG      lv_color_hex(0x010204)
#define COLOR_CARD    lv_color_hex(0x0A1118)
#define COLOR_CPU     lv_color_hex(0x00F2FF)
#define COLOR_GPU     lv_color_hex(0xFF6B00)
#define COLOR_MEM     lv_color_hex(0xFFB800)
#define COLOR_NET_DN  lv_color_hex(0x00FF85)
#define COLOR_NET_UP  lv_color_hex(0x00B2FF)
#define COLOR_TEXT    lv_color_hex(0xFFFFFF)
#define COLOR_MUTED   lv_color_hex(0xA0AEC0)
#define COLOR_ORANGE  lv_color_hex(0xFFA500)

/* ============================================================================
 * 图标定义 (UTF-8 编码)
 * ============================================================================ */

#define ICON_CPU      "\xEE\x98\x9A"  // 字符 58906
#define ICON_GPU      "\xEE\x98\x99"  // 字符 58905
#define ICON_MEM      "\xEE\x98\x98"  // 字符 58904
#define ICON_NET      "\xEE\x98\x9B"  // 字符 58907
#define ICON_WIFI     "\xEF\x87\xAB"  // WiFi 图标

/* 声明外部字体 */
LV_FONT_DECLARE(orbitron_10)
LV_FONT_DECLARE(orbitron_12)
LV_FONT_DECLARE(montserrat_10)
LV_FONT_DECLARE(montserrat_12)
LV_FONT_DECLARE(iconfont_12)

/* 前向声明 */
static void create_config_page_ultimate(void);
static const char* get_device_id(void);
static void config_page_close_cb(lv_event_t *e);
extern void ble_config_start(void);
extern void ble_config_stop(void);

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *wifi_icon;
    lv_obj_t *server_status_dot;  // 服务器状态指示点
    lv_obj_t *time_lbl;           // 时间显示

    // CPU 模块
    lv_obj_t *cpu_arc;
    lv_obj_t *cpu_usage_lbl;
    lv_obj_t *cpu_temp_key;
    lv_obj_t *cpu_temp_lbl;
    lv_obj_t *cpu_volt_key;
    lv_obj_t *cpu_volt_lbl;
    lv_obj_t *cpu_fan_key;
    lv_obj_t *cpu_fan_lbl;
    lv_obj_t *cpu_chart;
    lv_chart_series_t *cpu_ser;

    // GPU 模块
    lv_obj_t *gpu_arc;
    lv_obj_t *gpu_core_lbl;
    lv_obj_t *gpu_vram_key;
    lv_obj_t *gpu_vram_lbl;
    lv_obj_t *gpu_temp_key;
    lv_obj_t *gpu_temp_lbl;
    lv_obj_t *gpu_fan_key;
    lv_obj_t *gpu_fan_lbl;
    lv_obj_t *gpu_fps_lbl;  // FPS 显示
    lv_obj_t *gpu_chart;
    lv_chart_series_t *gpu_ser;

    // MEM 模块
    lv_obj_t *mem_bar;
    lv_obj_t *mem_usage_key;
    lv_obj_t *mem_usage_lbl;
    lv_obj_t *mem_freq_key;
    lv_obj_t *mem_freq_lbl;
    lv_obj_t *mem_volt_key;
    lv_obj_t *mem_volt_lbl;
    lv_obj_t *mem_chart;
    lv_chart_series_t *mem_ser;

    // NET 模块
    lv_obj_t *net_dn_key;
    lv_obj_t *net_dn_lbl;
    lv_obj_t *net_up_key;
    lv_obj_t *net_up_lbl;
    lv_obj_t *net_chart;
    lv_chart_series_t *net_dn_ser;
    lv_chart_series_t *net_up_ser;
} ui_t;

static ui_t ui;

/* 配网页面相关 */
static lv_obj_t *config_page = NULL;
static lv_obj_t *qr_code = NULL;

/* WiFi 状态闪烁定时器 */
static lv_timer_t *wifi_flash_timer = NULL;
static wifi_state_t current_wifi_state = WIFI_STATE_DISCONNECTED;
static bool wifi_flash_toggle = false;

/* WiFi 闪烁定时器回调 */
static void wifi_flash_timer_cb(lv_timer_t *timer) {
    if (current_wifi_state == WIFI_STATE_CONNECTING) {
        // 获取 wifi_icon 的子对象（label）
        lv_obj_t *label = lv_obj_get_child(ui.wifi_icon, 0);
        if (!label) return;

        // 灰橘闪烁
        wifi_flash_toggle = !wifi_flash_toggle;
        if (wifi_flash_toggle) {
            lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREY), 0);  // 灰色
        } else {
            lv_obj_set_style_text_color(label, COLOR_ORANGE, 0);  // 橘色
        }
    }
}

/* WiFi 图标点击回调 */
static void wifi_icon_click_cb(lv_event_t *e) {
    printf(">>> WiFi 图标被点击! <<<\n");
    printf("正在启动配网服务...\n");
    ui_monitor_show_config_page();
    ble_config_start();
    printf("ble_config_start() 已调用\n");
}

/* 辅助：创建卡片基础 */
static lv_obj_t * create_card(lv_obj_t * parent, const char * icon, const char * title, lv_color_t color, int x, int y) {
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, 150, 100);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    // 添加70%透明度（opacity = 255 * 0.3 = 77）
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_border_opa(card, 77, 0);  // 70%透明度
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // 创建图标标签
    lv_obj_t * icon_lbl = lv_label_create(card);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_color(icon_lbl, color, 0);
    lv_obj_set_style_text_font(icon_lbl, &iconfont_12, 0);  // 使用图标字体
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    // 创建标题标签
    lv_obj_t * title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, color, 0);
    lv_obj_set_style_text_font(title_lbl, &orbitron_10, 0);  // 使用 Orbitron 字体
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 16, 1);  // 图标后面偏移一点

    return card;
}

/* 辅助：创建迷你波形图 */
static lv_obj_t * create_mini_chart(lv_obj_t * parent, lv_color_t color) {
    lv_obj_t * chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 138, 35);  // 增加高度到 35px，波动区间更大
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 10);  // 下移 10px
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart, 30);  // 增加点数，更平滑
    lv_obj_set_style_bg_opa(chart, 0, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);  // 保持线条粗细为 2px
    lv_obj_set_style_line_color(chart, color, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);  // 隐藏点
    lv_chart_set_div_line_count(chart, 0, 0);
    return chart;
}

void ui_monitor_init_ultimate(void) {
    ui.screen = lv_scr_act();
    lv_obj_set_style_bg_color(ui.screen, COLOR_BG, 0);

    // 标题
    lv_obj_t * title = lv_label_create(ui.screen);
    lv_label_set_text(title, "OKRA MONITOR");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 5);

    // 服务器状态指示点（紧挨着标题后面）
    ui.server_status_dot = lv_label_create(ui.screen);
    lv_label_set_text(ui.server_status_dot, LV_SYMBOL_BULLET);  // 使用 LVGL 内置符号
    lv_obj_set_style_text_color(ui.server_status_dot, lv_palette_main(LV_PALETTE_RED), 0);  // 默认红色（未连接）
    lv_obj_set_style_text_font(ui.server_status_dot, lv_theme_get_font_small(ui.screen), 0);  // 使用默认字体
    lv_obj_align_to(ui.server_status_dot, title, LV_ALIGN_OUT_RIGHT_MID, 2, 0);  // 紧挨着标题右侧

    // WiFi 图标 - 使用按钮作为点击区域，更容易点击
    ui.wifi_icon = lv_btn_create(ui.screen);
    lv_obj_set_size(ui.wifi_icon, 40, 30);  // 增大点击区域
    lv_obj_align(ui.wifi_icon, LV_ALIGN_TOP_RIGHT, -5, 2);
    lv_obj_set_style_bg_opa(ui.wifi_icon, LV_OPA_TRANSP, 0);  // 透明背景
    lv_obj_set_style_border_width(ui.wifi_icon, 0, 0);  // 无边框
    lv_obj_set_style_shadow_width(ui.wifi_icon, 0, 0);  // 无阴影
    lv_obj_add_event_cb(ui.wifi_icon, wifi_icon_click_cb, LV_EVENT_CLICKED, NULL);

    // WiFi 图标文字
    lv_obj_t *wifi_label = lv_label_create(ui.wifi_icon);
    lv_label_set_text(wifi_label, ICON_WIFI);
    lv_obj_set_style_text_color(wifi_label, COLOR_NET_DN, 0);
    lv_obj_center(wifi_label);

    // 时间显示（WiFi 图标左边）
    ui.time_lbl = lv_label_create(ui.screen);
    lv_label_set_text(ui.time_lbl, "--:--");
    lv_obj_set_style_text_color(ui.time_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ui.time_lbl, &orbitron_12, 0);
    lv_obj_align_to(ui.time_lbl, ui.wifi_icon, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    // --- CPU 模块 ---
    lv_obj_t * cpu_card = create_card(ui.screen, ICON_CPU, "CPU", COLOR_CPU, 6, 30);
    ui.cpu_arc = lv_arc_create(cpu_card);
    lv_obj_set_size(ui.cpu_arc, 48, 48);  // 增加直径
    lv_arc_set_rotation(ui.cpu_arc, 135);  // 从左下开始
    lv_arc_set_bg_angles(ui.cpu_arc, 0, 270);  // 270度，留90度豁口
    lv_arc_set_range(ui.cpu_arc, 0, 100);
    lv_obj_remove_style(ui.cpu_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(ui.cpu_arc, 3, LV_PART_MAIN);      // 背景弧线宽度
    lv_obj_set_style_arc_width(ui.cpu_arc, 3, LV_PART_INDICATOR); // 进度弧线宽度
    lv_obj_set_style_arc_color(ui.cpu_arc, COLOR_CPU, LV_PART_INDICATOR);  // 进度颜色
    lv_obj_set_style_arc_color(ui.cpu_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN);  // 背景颜色
    lv_obj_align(ui.cpu_arc, LV_ALIGN_TOP_LEFT, 0, 20);
    ui.cpu_usage_lbl = lv_label_create(ui.cpu_arc);
    lv_obj_set_style_text_font(ui.cpu_usage_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.cpu_usage_lbl, COLOR_CPU, 0);  // CPU 颜色
    lv_obj_center(ui.cpu_usage_lbl);

    // Temp key-value
    ui.cpu_temp_key = lv_label_create(cpu_card);
    lv_label_set_text(ui.cpu_temp_key, "Temp");
    lv_obj_set_style_text_font(ui.cpu_temp_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.cpu_temp_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.cpu_temp_key, LV_ALIGN_TOP_LEFT, 52, 18);
    ui.cpu_temp_lbl = lv_label_create(cpu_card);
    lv_obj_set_style_text_font(ui.cpu_temp_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.cpu_temp_lbl, COLOR_CPU, 0);  // CPU 颜色
    lv_obj_align(ui.cpu_temp_lbl, LV_ALIGN_TOP_RIGHT, 0, 18);

    // Volt key-value
    ui.cpu_volt_key = lv_label_create(cpu_card);
    lv_label_set_text(ui.cpu_volt_key, "Volt");
    lv_obj_set_style_text_font(ui.cpu_volt_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.cpu_volt_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.cpu_volt_key, LV_ALIGN_TOP_LEFT, 52, 34);
    ui.cpu_volt_lbl = lv_label_create(cpu_card);
    lv_obj_set_style_text_font(ui.cpu_volt_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.cpu_volt_lbl, COLOR_CPU, 0);  // CPU 颜色
    lv_obj_align(ui.cpu_volt_lbl, LV_ALIGN_TOP_RIGHT, 0, 34);

    // Fan key-value (不显示标签，直接显示数值)
    ui.cpu_fan_key = lv_label_create(cpu_card);
    lv_label_set_text(ui.cpu_fan_key, "");  // 空标签
    lv_obj_set_style_text_font(ui.cpu_fan_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.cpu_fan_key, COLOR_MUTED, 0);
    lv_obj_align(ui.cpu_fan_key, LV_ALIGN_TOP_LEFT, 52, 50);
    ui.cpu_fan_lbl = lv_label_create(cpu_card);
    lv_obj_set_style_text_font(ui.cpu_fan_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.cpu_fan_lbl, COLOR_CPU, 0);  // CPU 颜色
    lv_obj_align(ui.cpu_fan_lbl, LV_ALIGN_TOP_RIGHT, 0, 50);
    
    ui.cpu_chart = create_mini_chart(cpu_card, COLOR_CPU);
    ui.cpu_ser = lv_chart_add_series(ui.cpu_chart, COLOR_CPU, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(ui.cpu_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);  // CPU 使用率 0-100%

    // --- GPU 模块 ---
    lv_obj_t * gpu_card = create_card(ui.screen, ICON_GPU, "GPU", COLOR_GPU, 164, 30);

    // FPS 显示（GPU 标题同一行，右侧）
    ui.gpu_fps_lbl = lv_label_create(gpu_card);
    lv_label_set_text(ui.gpu_fps_lbl, "--");
    lv_obj_set_style_text_font(ui.gpu_fps_lbl, &orbitron_12, 0);
    lv_obj_set_style_text_color(ui.gpu_fps_lbl, COLOR_GPU, 0);
    lv_obj_align(ui.gpu_fps_lbl, LV_ALIGN_TOP_RIGHT, 0, 0);

    ui.gpu_arc = lv_arc_create(gpu_card);
    lv_obj_set_size(ui.gpu_arc, 48, 48);  // 增加直径
    lv_arc_set_rotation(ui.gpu_arc, 135);  // 从左下开始
    lv_arc_set_bg_angles(ui.gpu_arc, 0, 270);  // 270度，留90度豁口
    lv_arc_set_range(ui.gpu_arc, 0, 100);
    lv_obj_remove_style(ui.gpu_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(ui.gpu_arc, 3, LV_PART_MAIN);      // 背景弧线宽度
    lv_obj_set_style_arc_width(ui.gpu_arc, 3, LV_PART_INDICATOR); // 进度弧线宽度
    lv_obj_set_style_arc_color(ui.gpu_arc, COLOR_GPU, LV_PART_INDICATOR);  // 进度颜色
    lv_obj_set_style_arc_color(ui.gpu_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN);  // 背景颜色
    lv_obj_align(ui.gpu_arc, LV_ALIGN_TOP_LEFT, 0, 20);
    ui.gpu_core_lbl = lv_label_create(ui.gpu_arc);
    lv_obj_set_style_text_font(ui.gpu_core_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.gpu_core_lbl, COLOR_GPU, 0);  // GPU 颜色
    lv_obj_center(ui.gpu_core_lbl);

    // Temp key-value (第一行)
    ui.gpu_temp_key = lv_label_create(gpu_card);
    lv_label_set_text(ui.gpu_temp_key, "Temp");
    lv_obj_set_style_text_font(ui.gpu_temp_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.gpu_temp_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.gpu_temp_key, LV_ALIGN_TOP_LEFT, 52, 18);
    ui.gpu_temp_lbl = lv_label_create(gpu_card);
    lv_obj_set_style_text_font(ui.gpu_temp_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.gpu_temp_lbl, COLOR_GPU, 0);  // GPU 颜色
    lv_obj_align(ui.gpu_temp_lbl, LV_ALIGN_TOP_RIGHT, 0, 18);

    // VRAM key-value (第二行)
    ui.gpu_vram_key = lv_label_create(gpu_card);
    lv_label_set_text(ui.gpu_vram_key, "VRAM");
    lv_obj_set_style_text_font(ui.gpu_vram_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.gpu_vram_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.gpu_vram_key, LV_ALIGN_TOP_LEFT, 52, 34);
    ui.gpu_vram_lbl = lv_label_create(gpu_card);
    lv_obj_set_style_text_font(ui.gpu_vram_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.gpu_vram_lbl, COLOR_GPU, 0);  // GPU 颜色
    lv_obj_align(ui.gpu_vram_lbl, LV_ALIGN_TOP_RIGHT, 0, 34);

    // Fan key-value (第三行，不显示标签)
    ui.gpu_fan_key = lv_label_create(gpu_card);
    lv_label_set_text(ui.gpu_fan_key, "");  // 空标签
    lv_obj_set_style_text_font(ui.gpu_fan_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.gpu_fan_key, COLOR_MUTED, 0);
    lv_obj_align(ui.gpu_fan_key, LV_ALIGN_TOP_LEFT, 52, 50);
    ui.gpu_fan_lbl = lv_label_create(gpu_card);
    lv_obj_set_style_text_font(ui.gpu_fan_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.gpu_fan_lbl, COLOR_GPU, 0);  // GPU 颜色
    lv_obj_align(ui.gpu_fan_lbl, LV_ALIGN_TOP_RIGHT, 0, 50);

    ui.gpu_chart = create_mini_chart(gpu_card, COLOR_GPU);
    ui.gpu_ser = lv_chart_add_series(ui.gpu_chart, COLOR_GPU, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(ui.gpu_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);  // GPU 使用率 0-100%

    // --- MEM 模块 ---
    lv_obj_t * mem_card = create_card(ui.screen, ICON_MEM, "MEMORY", COLOR_MEM, 6, 136);

    // Usage key-value
    ui.mem_usage_key = lv_label_create(mem_card);
    lv_label_set_text(ui.mem_usage_key, "Usage");
    lv_obj_set_style_text_font(ui.mem_usage_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.mem_usage_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.mem_usage_key, LV_ALIGN_TOP_LEFT, 0, 18);
    ui.mem_usage_lbl = lv_label_create(mem_card);
    lv_obj_set_style_text_font(ui.mem_usage_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.mem_usage_lbl, COLOR_MEM, 0);  // MEM 颜色
    lv_obj_align(ui.mem_usage_lbl, LV_ALIGN_TOP_RIGHT, 0, 18);

    ui.mem_bar = lv_bar_create(mem_card);
    lv_obj_set_size(ui.mem_bar, 138, 4);
    lv_obj_set_style_bg_color(ui.mem_bar, lv_color_hex(0x1A1A1A), LV_PART_MAIN);  // 背景颜色
    lv_obj_set_style_bg_color(ui.mem_bar, COLOR_MEM, LV_PART_INDICATOR);  // 进度颜色
    lv_obj_align(ui.mem_bar, LV_ALIGN_TOP_LEFT, 0, 32);

    // Freq key-value
    ui.mem_freq_key = lv_label_create(mem_card);
    lv_label_set_text(ui.mem_freq_key, "Freq");
    lv_obj_set_style_text_font(ui.mem_freq_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.mem_freq_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.mem_freq_key, LV_ALIGN_TOP_LEFT, 0, 40);
    ui.mem_freq_lbl = lv_label_create(mem_card);
    lv_obj_set_style_text_font(ui.mem_freq_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.mem_freq_lbl, COLOR_MEM, 0);  // MEM 颜色
    lv_obj_align(ui.mem_freq_lbl, LV_ALIGN_TOP_RIGHT, 0, 40);

    // Volt key-value
    ui.mem_volt_key = lv_label_create(mem_card);
    lv_label_set_text(ui.mem_volt_key, "Volt");
    lv_obj_set_style_text_font(ui.mem_volt_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.mem_volt_key, COLOR_MUTED, 0);  // 灰色
    lv_obj_align(ui.mem_volt_key, LV_ALIGN_TOP_LEFT, 0, 52);
    ui.mem_volt_lbl = lv_label_create(mem_card);
    lv_obj_set_style_text_font(ui.mem_volt_lbl, &orbitron_12, 0);  // 使用 Orbitron 字体
    lv_obj_set_style_text_color(ui.mem_volt_lbl, COLOR_MEM, 0);  // MEM 颜色
    lv_obj_align(ui.mem_volt_lbl, LV_ALIGN_TOP_RIGHT, 0, 52);

    ui.mem_chart = create_mini_chart(mem_card, COLOR_MEM);
    ui.mem_ser = lv_chart_add_series(ui.mem_chart, COLOR_MEM, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(ui.mem_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);  // 内存使用率 0-100%

    // --- NET 模块 ---
    lv_obj_t * net_card = create_card(ui.screen, ICON_NET, "NETWORK", COLOR_NET_DN, 164, 136);

    // Net Down key-value
    ui.net_dn_key = lv_label_create(net_card);
    lv_label_set_text(ui.net_dn_key, "Down");
    lv_obj_set_style_text_font(ui.net_dn_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.net_dn_key, COLOR_MUTED, 0);
    lv_obj_align(ui.net_dn_key, LV_ALIGN_TOP_LEFT, 0, 30);
    ui.net_dn_lbl = lv_label_create(net_card);
    lv_obj_set_style_text_font(ui.net_dn_lbl, &orbitron_12, 0);
    lv_obj_set_style_text_color(ui.net_dn_lbl, COLOR_NET_DN, 0);
    lv_obj_align(ui.net_dn_lbl, LV_ALIGN_TOP_RIGHT, 0, 30);

    // Net Up key-value
    ui.net_up_key = lv_label_create(net_card);
    lv_label_set_text(ui.net_up_key, "Upload");
    lv_obj_set_style_text_font(ui.net_up_key, &montserrat_10, 0);
    lv_obj_set_style_text_color(ui.net_up_key, COLOR_MUTED, 0);
    lv_obj_align(ui.net_up_key, LV_ALIGN_TOP_LEFT, 0, 46);
    ui.net_up_lbl = lv_label_create(net_card);
    lv_obj_set_style_text_font(ui.net_up_lbl, &orbitron_12, 0);
    lv_obj_set_style_text_color(ui.net_up_lbl, COLOR_NET_UP, 0);
    lv_obj_align(ui.net_up_lbl, LV_ALIGN_TOP_RIGHT, 0, 46);

    ui.net_chart = create_mini_chart(net_card, COLOR_NET_DN);
    ui.net_dn_ser = lv_chart_add_series(ui.net_chart, COLOR_NET_DN, LV_CHART_AXIS_PRIMARY_Y);
    ui.net_up_ser = lv_chart_add_series(ui.net_chart, COLOR_NET_UP, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(ui.net_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 5000);  // 网络速度 0-5000 KB/s

    // 预创建配网页面（隐藏状态）
    create_config_page_ultimate();
}

// 创建配网页面（隐藏状态）
static void create_config_page_ultimate(void) {
    if (config_page) return;  // 已创建

    // 创建全屏遮罩
    config_page = lv_obj_create(lv_layer_top());
    lv_obj_set_size(config_page, 320, 240);
    lv_obj_set_pos(config_page, 0, 0);
    lv_obj_set_style_bg_color(config_page, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_bg_opa(config_page, LV_OPA_90, 0);
    lv_obj_set_style_border_width(config_page, 0, 0);
    lv_obj_set_style_radius(config_page, 0, 0);

    // 标题
    lv_obj_t *title = lv_label_create(config_page);
    lv_label_set_text(title, "WiFi Provisioning");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // 设备 ID
    lv_obj_t *id_label = lv_label_create(config_page);
    static char id_text[32];
    snprintf(id_text, sizeof(id_text), "Device: %s", get_device_id());
    lv_label_set_text(id_label, id_text);
    lv_obj_set_style_text_color(id_label, lv_color_hex(0xffcc00), 0);
    lv_obj_set_style_text_font(id_label, &lv_font_montserrat_12, 0);
    lv_obj_align(id_label, LV_ALIGN_TOP_MID, 0, 28);

#if LV_USE_QRCODE
    // 创建 QR 码
    qr_code = lv_qrcode_create(config_page, 100, lv_color_hex(0x000000), lv_color_hex(0xffffff));
    const char *qr_data = "https://mp.weixin.qq.com/a/~~zWHCi6-hZMQ~sSDBKrf581Cfu_v7LLXWVg~~";
    lv_qrcode_update(qr_code, qr_data, strlen(qr_data));
    lv_obj_align(qr_code, LV_ALIGN_CENTER, 0, -5);
    lv_obj_set_style_border_color(qr_code, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(qr_code, 2, 0);
#else
    lv_obj_t *hint = lv_label_create(config_page);
    lv_label_set_text(hint, "Scan with Mini Program");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7aa3c0), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);
#endif

    // BLE 状态
    lv_obj_t *status = lv_label_create(config_page);
    lv_label_set_text(status, "BLE Advertising...");
    lv_obj_set_style_text_color(status, lv_color_hex(0x00ff88), 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_10, 0);
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -50);

    // 关闭按钮
    lv_obj_t *close_btn = lv_btn_create(config_page);
    lv_obj_set_size(close_btn, 80, 28);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_event_cb(close_btn, config_page_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(close_btn);
    lv_label_set_text(btn_label, "Cancel");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_10, 0);
    lv_obj_center(btn_label);

    // 初始隐藏
    lv_obj_add_flag(config_page, LV_OBJ_FLAG_HIDDEN);
    printf("配网页面已预创建\n");
}

void ui_monitor_update_ultimate(
    float cpu_usage, float cpu_temp, float cpu_volt, int cpu_fan,
    float gpu_core_usage, float gpu_vram_usage, float gpu_temp, int gpu_fan, int fps,
    float mem_usage, int mem_freq, float mem_volt,
    float net_down, float net_up,
    wifi_state_t wifi_state
) {
    // CPU
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.cpu_arc);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_value);
    lv_anim_set_values(&a, lv_arc_get_value(ui.cpu_arc), (int)cpu_usage);
    lv_anim_set_time(&a, 500);  // 500ms动画时间
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    lv_label_set_text_fmt(ui.cpu_usage_lbl, "%d %%", (int)cpu_usage);

    // 手动格式化温度
    static char temp_buf[16];
    snprintf(temp_buf, sizeof(temp_buf), "%.0f °C", cpu_temp);
    lv_label_set_text(ui.cpu_temp_lbl, temp_buf);

    // 手动格式化电压
    static char volt_buf[16];
    snprintf(volt_buf, sizeof(volt_buf), "%.2f V", cpu_volt);
    lv_label_set_text(ui.cpu_volt_lbl, volt_buf);

    lv_label_set_text_fmt(ui.cpu_fan_lbl, "%d RPM", cpu_fan);
    lv_chart_set_next_value(ui.cpu_chart, ui.cpu_ser, (int)cpu_usage);

    // GPU
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, ui.gpu_arc);
    lv_anim_set_exec_cb(&b, (lv_anim_exec_xcb_t)lv_arc_set_value);
    lv_anim_set_values(&b, lv_arc_get_value(ui.gpu_arc), (int)gpu_core_usage);
    lv_anim_set_time(&b, 500);
    lv_anim_set_path_cb(&b, lv_anim_path_ease_out);
    lv_anim_start(&b);
    lv_label_set_text_fmt(ui.gpu_core_lbl, "%d %%", (int)gpu_core_usage);
    lv_label_set_text_fmt(ui.gpu_vram_lbl, "%d %%", (int)gpu_vram_usage);

    // 手动格式化 GPU 温度
    snprintf(temp_buf, sizeof(temp_buf), "%.0f °C", gpu_temp);
    lv_label_set_text(ui.gpu_temp_lbl, temp_buf);

    lv_label_set_text_fmt(ui.gpu_fan_lbl, "%d RPM", gpu_fan);

    // FPS 显示
    lv_label_set_text_fmt(ui.gpu_fps_lbl, "%d FPS", fps);

    lv_chart_set_next_value(ui.gpu_chart, ui.gpu_ser, (int)gpu_core_usage);

    // MEM
    lv_anim_t c;
    lv_anim_init(&c);
    lv_anim_set_var(&c, ui.mem_bar);
    lv_anim_set_exec_cb(&c, (lv_anim_exec_xcb_t)lv_bar_set_value);
    lv_anim_set_values(&c, lv_bar_get_value(ui.mem_bar), (int)mem_usage);
    lv_anim_set_time(&c, 500);
    lv_anim_set_path_cb(&c, lv_anim_path_ease_out);
    lv_anim_start(&c);

    lv_label_set_text_fmt(ui.mem_usage_lbl, "%d %%", (int)mem_usage);

    // 手动格式化内存频率
    snprintf(temp_buf, sizeof(temp_buf), "%d MHz", mem_freq);
    lv_label_set_text(ui.mem_freq_lbl, temp_buf);

    // 手动格式化内存电压
    snprintf(volt_buf, sizeof(volt_buf), "%.2f V", mem_volt);
    lv_label_set_text(ui.mem_volt_lbl, volt_buf);

    lv_chart_set_next_value(ui.mem_chart, ui.mem_ser, (int)mem_usage);

    // NET - 手动格式化网络速度（整数 KB/s）
    static char net_buf[16];
    snprintf(net_buf, sizeof(net_buf), "%d KB/s", (int)net_down);
    lv_label_set_text(ui.net_dn_lbl, net_buf);

    snprintf(net_buf, sizeof(net_buf), "%d KB/s", (int)net_up);
    lv_label_set_text(ui.net_up_lbl, net_buf);

    // 分别显示下载和上传速度
    lv_chart_set_next_value(ui.net_chart, ui.net_dn_ser, (int)net_down);
    lv_chart_set_next_value(ui.net_chart, ui.net_up_ser, (int)net_up);

    // WiFi
    current_wifi_state = wifi_state;
    switch(wifi_state) {
        case WIFI_STATE_DISCONNECTED:
            // 停止闪烁定时器
            if (wifi_flash_timer) {
                lv_timer_del(wifi_flash_timer);
                wifi_flash_timer = NULL;
            }
            lv_obj_set_style_text_color(ui.wifi_icon, lv_palette_main(LV_PALETTE_GREY), 0);  // 灰色
            break;
        case WIFI_STATE_CONNECTING:
            // 启动闪烁定时器（500ms 间隔）
            if (!wifi_flash_timer) {
                wifi_flash_timer = lv_timer_create(wifi_flash_timer_cb, 500, NULL);
            }
            break;
        case WIFI_STATE_CONNECTED:
            // 停止闪烁定时器
            if (wifi_flash_timer) {
                lv_timer_del(wifi_flash_timer);
                wifi_flash_timer = NULL;
            }
            lv_obj_set_style_text_color(ui.wifi_icon, COLOR_NET_DN, 0);  // 绿色
            break;
    }
}

/**
 * @brief 更新 WiFi 状态指示器
 */
void ui_monitor_set_wifi_state(wifi_state_t state) {
    current_wifi_state = state;
    // 获取 wifi_icon 的子对象（label）
    lv_obj_t *label = lv_obj_get_child(ui.wifi_icon, 0);
    if (!label) return;

    switch(state) {
        case WIFI_STATE_DISCONNECTED:
            // 停止闪烁定时器
            if (wifi_flash_timer) {
                lv_timer_del(wifi_flash_timer);
                wifi_flash_timer = NULL;
            }
            lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), 0);  // 红色
            break;
        case WIFI_STATE_CONNECTING:
            // 停止闪烁定时器，直接显示橘色
            if (wifi_flash_timer) {
                lv_timer_del(wifi_flash_timer);
                wifi_flash_timer = NULL;
            }
            lv_obj_set_style_text_color(label, COLOR_ORANGE, 0);  // 橘色
            break;
        case WIFI_STATE_CONNECTED:
            // 停止闪烁定时器
            if (wifi_flash_timer) {
                lv_timer_del(wifi_flash_timer);
                wifi_flash_timer = NULL;
            }
            lv_obj_set_style_text_color(label, COLOR_NET_DN, 0);  // 绿色
            break;
    }
}

/**
 * @brief 更新服务器连接状态指示器
 */
void ui_monitor_set_server_state(server_state_t state) {
    switch(state) {
        case SERVER_STATE_DISCONNECTED:
            lv_obj_set_style_text_color(ui.server_status_dot, lv_palette_main(LV_PALETTE_RED), 0);  // 红色
            break;
        case SERVER_STATE_CONNECTING:
            lv_obj_set_style_text_color(ui.server_status_dot, COLOR_ORANGE, 0);  // 橘色（连接中）
            break;
        case SERVER_STATE_CONNECTED:
            lv_obj_set_style_text_color(ui.server_status_dot, COLOR_NET_DN, 0);  // 绿色
            break;
    }
}

/**
 * @brief 更新时间显示
 */
void ui_monitor_set_time(int hour, int minute) {
    static char time_buf[8];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour, minute);
    lv_label_set_text(ui.time_lbl, time_buf);
}

/* ============================================================================
 * 配网页面相关函数
 * ============================================================================ */

/**
 * @brief 配网页面关闭回调
 */
static void config_page_close_cb(lv_event_t *e) {
    printf("关闭配网页面\n");
    ui_monitor_hide_config_page();
    ble_config_stop();
}

/**
 * @brief 获取设备短 ID（MAC 后6位）
 */
static const char* get_device_id(void) {
    static char device_id[8] = {0};
    if (device_id[0] == '\0') {
        // 使用 ESP32 的 WiFi MAC 地址
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(device_id, sizeof(device_id), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    }
    return device_id;
}

/**
 * @brief 显示配网页面
 */
void ui_monitor_show_config_page(void) {
    if (config_page) {
        lv_obj_clear_flag(config_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(config_page);
        printf("配网页面已显示\n");
    } else {
        printf("错误：config_page 未创建!\n");
    }
}

/**
 * @brief 隐藏配网页面
 */
void ui_monitor_hide_config_page(void) {
    if (config_page) {
        lv_obj_add_flag(config_page, LV_OBJ_FLAG_HIDDEN);
        printf("配网页面已隐藏\n");
    }
}
