// main/main.c —— 名片玩法:首页个人名片 + 7 张全屏图像,长按 OK 进入热点编辑。
//
// 按键语义:
//   上/下 短按   切换页面(0=名片,1..7=图像页)
//   OK   长按   进入/退出热点上传编辑模式
//
// 名片文字与图像(头像/全屏图)通过热点 Web 页上传,写入 NVS + 图像分区,掉电不丢。
#include "app_config.h"
#include "app_fonts.h"
#include "app_http.h"
#include "app_images.h"
#include "app_model.h"
#include "app_store.h"
#include "app_ui.h"
#include "app_wifi.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "main";

#define INPUT_QUEUE_DEPTH 8
#define CMD_QUEUE_DEPTH   4

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t event;
} input_event_t;

typedef enum {
    UPLOAD_ENTER = 0,
    UPLOAD_EXIT,
} upload_cmd_t;

static app_model_t s_model;

static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static QueueHandle_t s_cmd_queue;
static TaskHandle_t s_upload_task;
static TaskHandle_t s_battery_task;
static volatile bool s_input_ready;

// —— 上传模式(热点 + HTTP)------------------------------------------------

static void do_upload_enter(void)
{
    esp_err_t err = app_wifi_ap_start();
    if (err == ESP_OK) err = app_http_start(&s_model);

    char msg[192];
    if (err == ESP_OK) {
        snprintf(msg, sizeof(msg),
                 "热点: %s\n密码: %s\n地址: http://%s/\n\n浏览器打开编辑页\n长按 OK 退出",
                 APP_AP_SSID, APP_AP_PASSWORD, app_wifi_ap_ip());
    } else {
        snprintf(msg, sizeof(msg), "热点启动失败: %s\n\n长按 OK 退出", esp_err_to_name(err));
    }

    if (bsp_lvgl_lock(500)) {
        app_ui_upload_status(msg);
        bsp_lvgl_unlock();
    }
}

static void do_upload_exit(void)
{
    app_http_stop();
    app_wifi_ap_stop();
    if (bsp_lvgl_lock(500)) {
        app_ui_leave_upload();
        bsp_lvgl_unlock();
    }
}

static void upload_task(void *arg)
{
    (void)arg;
    upload_cmd_t cmd;
    for (;;) {
        if (xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            if (cmd == UPLOAD_ENTER) do_upload_enter();
            else do_upload_exit();
        }
    }
}

// —— 电量轮询 -------------------------------------------------------------

static void battery_task(void *arg)
{
    (void)arg;
    for (;;) {
        int soc = bsp_battery_soc();
        if (soc != s_model.battery_soc) {
            s_model.battery_soc = soc;
            if (bsp_lvgl_lock(500)) {
                app_ui_sync();
                bsp_lvgl_unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// —— 按键分发 -------------------------------------------------------------

static void process_input(const input_event_t *in)
{
    if (in->btn == BSP_BTN_OK && in->event == BSP_BTN_LONG) {
        if (!s_model.upload_mode) {
            s_model.upload_mode = true;
            if (bsp_lvgl_lock(500)) {
                app_ui_enter_upload();
                app_ui_upload_status("正在开启热点...");
                bsp_lvgl_unlock();
            }
            upload_cmd_t cmd = UPLOAD_ENTER;
            (void)xQueueSend(s_cmd_queue, &cmd, 0);
        } else {
            s_model.upload_mode = false;
            upload_cmd_t cmd = UPLOAD_EXIT;
            (void)xQueueSend(s_cmd_queue, &cmd, 0);
        }
        return;
    }

    if (in->event != BSP_BTN_CLICK) return;
    if (s_model.upload_mode) return;  // 上传模式里上下键不切换页面
    if (in->btn != BSP_BTN_UP && in->btn != BSP_BTN_DOWN) return;

    if (!bsp_lvgl_lock(500)) return;
    if (in->btn == BSP_BTN_UP) app_model_page_prev(&s_model);
    else app_model_page_next(&s_model);
    app_ui_show_page(s_model.current_page);
    bsp_lvgl_unlock();
}

static void input_task(void *arg)
{
    (void)arg;
    input_event_t in;
    for (;;) {
        if (xQueueReceive(s_input_queue, &in, portMAX_DELAY) == pdTRUE) {
            process_input(&in);
        }
    }
}

// button 回调运行于 esp_timer 任务:只入队,立即返回。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!s_input_ready || !s_input_queue) return;
    const input_event_t in = { .btn = btn, .event = ev };
    (void)xQueueSend(s_input_queue, &in, 0);
}

static esp_err_t start_tasks(void)
{
    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    s_cmd_queue = xQueueCreate(CMD_QUEUE_DEPTH, sizeof(upload_cmd_t));
    if (!s_input_queue || !s_cmd_queue) return ESP_ERR_NO_MEM;

    if (xTaskCreate(input_task, "app_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(upload_task, "app_upload", 6144, NULL, 4, &s_upload_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(battery_task, "app_battery", 3072, NULL, 3, &s_battery_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "名片玩法启动");

    bsp_i2c_init();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,无法启动");
        return;
    }
    bsp_display_backlight(100);

    app_model_init(&s_model);
    app_fonts_init();

    esp_err_t err = app_store_init(&s_model);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "加载已存名片失败(%s),使用默认值", esp_err_to_name(err));
    }
    err = app_images_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "图像分区不可用(%s),仅显示占位", esp_err_to_name(err));
    }

    app_ui_init(&s_model);

    if (bsp_battery_init() == ESP_OK) {
        s_model.battery_soc = bsp_battery_soc();
    } else {
        ESP_LOGW(TAG, "电量计不可用,电量显示 [--]%%");
    }

    if (start_tasks() != ESP_OK) {
        ESP_LOGE(TAG, "任务创建失败,无法启动");
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败");
    }

    if (bsp_lvgl_lock(1000)) {
        app_ui_sync();   // 首帧电量/文字刷新
        bsp_lvgl_unlock();
    }
    s_input_ready = true;

    ESP_LOGI(TAG, "就绪");
}
