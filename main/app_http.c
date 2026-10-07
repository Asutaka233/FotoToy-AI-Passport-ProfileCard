// main/app_http.c —— 编辑页与上传接口。
// 上传图像采用流式写入(边收边写 flash),避免为整帧 150KB 在堆上开大缓冲。
#include "app_http.h"
#include "app_images.h"
#include "app_store.h"
#include "app_ui.h"
#include "app_wifi.h"
#include "app_config.h"
#include "bsp_display.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "app_http";

static httpd_handle_t s_server;
static app_model_t *s_model;

// 流式写入的单次块大小(4KB,与 flash 擦写对齐)。
#define HTTP_CHUNK 4096
static uint8_t s_rxbuf[HTTP_CHUNK];

// EMBED_FILES("edit_page.html") 生成的符号(见 main/CMakeLists.txt)。
extern const uint8_t edit_page_html_start[] asm("_binary_edit_page_html_start");
extern const uint8_t edit_page_html_end[] asm("_binary_edit_page_html_end");

static esp_err_t send_err(httpd_req_t *req, httpd_err_code_t code, const char *msg)
{
    return httpd_resp_send_err(req, code, msg);
}

// —— GET / —— 编辑页 ------------------------------------------------------

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)edit_page_html_start,
                    edit_page_html_end - edit_page_html_start);
    return ESP_OK;
}

// —— GET /api/state —— 返回当前名片(供编辑页回填)---------------------------

static esp_err_t state_get_handler(httpd_req_t *req)
{
    char name[APP_CARD_NAME_MAX];
    char unit[APP_CARD_UNIT_MAX];
    char position[APP_CARD_POSITION_MAX];
    char signature[APP_CARD_SIGNATURE_MAX];
    uint8_t present = 0;

    if (bsp_lvgl_lock(500)) {
        strncpy(name, s_model->card.name, sizeof(name) - 1); name[sizeof(name) - 1] = '\0';
        strncpy(unit, s_model->card.unit, sizeof(unit) - 1); unit[sizeof(unit) - 1] = '\0';
        strncpy(position, s_model->card.position, sizeof(position) - 1); position[sizeof(position) - 1] = '\0';
        strncpy(signature, s_model->card.signature, sizeof(signature) - 1); signature[sizeof(signature) - 1] = '\0';
        present = s_model->image_present;
        bsp_lvgl_unlock();
    } else {
        memset(name, 0, sizeof(name));
        memset(unit, 0, sizeof(unit));
        memset(position, 0, sizeof(position));
        memset(signature, 0, sizeof(signature));
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "unit", unit);
    cJSON_AddStringToObject(root, "position", position);
    cJSON_AddStringToObject(root, "signature", signature);
    cJSON_AddNumberToObject(root, "image_present", present);
    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "{}");
    cJSON_free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

// —— POST /api/card —— 更新名片文字 ---------------------------------------

static void copy_field(char *dst, size_t cap, const cJSON *item)
{
    if (item && cJSON_IsString(item)) {
        strncpy(dst, item->valuestring, cap - 1);
        dst[cap - 1] = '\0';
    } else {
        dst[0] = '\0';
    }
}

static esp_err_t card_post_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= sizeof(s_rxbuf)) {
        return send_err(req, HTTPD_400_BAD_REQUEST, "请求体过大");
    }
    int r = httpd_req_recv(req, (char *)s_rxbuf, req->content_len);
    if (r <= 0) return send_err(req, HTTPD_400_BAD_REQUEST, "读取失败");
    s_rxbuf[r] = '\0';

    cJSON *root = cJSON_Parse((const char *)s_rxbuf);
    if (!root) {
        return send_err(req, HTTPD_400_BAD_REQUEST, "JSON 解析失败");
    }

    char name[APP_CARD_NAME_MAX];
    char unit[APP_CARD_UNIT_MAX];
    char position[APP_CARD_POSITION_MAX];
    char signature[APP_CARD_SIGNATURE_MAX];
    copy_field(name, sizeof(name), cJSON_GetObjectItemCaseSensitive(root, "name"));
    copy_field(unit, sizeof(unit), cJSON_GetObjectItemCaseSensitive(root, "unit"));
    copy_field(position, sizeof(position), cJSON_GetObjectItemCaseSensitive(root, "position"));
    copy_field(signature, sizeof(signature), cJSON_GetObjectItemCaseSensitive(root, "signature"));
    cJSON_Delete(root);

    if (bsp_lvgl_lock(500)) {
        strncpy(s_model->card.name, name, sizeof(s_model->card.name) - 1);
        s_model->card.name[sizeof(s_model->card.name) - 1] = '\0';
        strncpy(s_model->card.unit, unit, sizeof(s_model->card.unit) - 1);
        s_model->card.unit[sizeof(s_model->card.unit) - 1] = '\0';
        strncpy(s_model->card.position, position, sizeof(s_model->card.position) - 1);
        s_model->card.position[sizeof(s_model->card.position) - 1] = '\0';
        strncpy(s_model->card.signature, signature, sizeof(s_model->card.signature) - 1);
        s_model->card.signature[sizeof(s_model->card.signature) - 1] = '\0';
        app_store_save(s_model);
        app_ui_sync();
        bsp_lvgl_unlock();
    }

    ESP_LOGI(TAG, "名片已更新");
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// —— POST /api/avatar 与 /api/image/N —— 流式写入图像 -----------------------

static esp_err_t upload_image(httpd_req_t *req, int slot)
{
    size_t expected = app_images_slot_size(slot);
    if (req->content_len != expected) {
        return send_err(req, HTTPD_400_BAD_REQUEST, "图像长度不符");
    }

    esp_err_t err = app_images_erase_slot(slot);
    if (err != ESP_OK) {
        return send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "擦除失败");
    }

    size_t written = 0;
    while (written < expected) {
        size_t want = expected - written;
        if (want > sizeof(s_rxbuf)) want = sizeof(s_rxbuf);
        int r = httpd_req_recv(req, (char *)s_rxbuf, want);
        if (r <= 0) {
            return send_err(req, HTTPD_400_BAD_REQUEST, "传输中断");
        }
        err = app_images_write_slot_part(slot, written, s_rxbuf, (size_t)r);
        if (err != ESP_OK) {
            return send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "写入失败");
        }
        written += (size_t)r;
    }

    app_model_set_image(s_model, slot, true);
    app_store_save(s_model);
    if (bsp_lvgl_lock(500)) {
        app_ui_sync();
        bsp_lvgl_unlock();
    }

    ESP_LOGI(TAG, "图像槽 %d 已上传(%u 字节)", slot, (unsigned)written);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t avatar_post_handler(httpd_req_t *req)
{
    return upload_image(req, 0);
}

static esp_err_t image_post_handler(httpd_req_t *req)
{
    const char *slash = strrchr(req->uri, '/');
    if (!slash) return send_err(req, HTTPD_400_BAD_REQUEST, "非法路径");
    int slot = atoi(slash + 1);
    if (slot < 1 || slot > APP_IMAGE_PAGE_COUNT) {
        return send_err(req, HTTPD_400_BAD_REQUEST, "页号越界");
    }
    return upload_image(req, slot);
}

// —— 生命周期 -------------------------------------------------------------

esp_err_t app_http_start(app_model_t *model)
{
    if (s_server) return ESP_OK;
    s_model = model;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 12;
    cfg.lru_purge_enable = true;
    // /api/image/N 是通配符路由;默认 uri_match_fn 只做整串精确匹配,通配符形同虚设,
    // 上传展示图会因找不到处理器而返回 404。显式启用通配匹配即可命中。
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP 服务器启动失败: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t routes[] = {
        { .uri = "/",           .method = HTTP_GET,  .handler = root_get_handler },
        { .uri = "/api/state",  .method = HTTP_GET,  .handler = state_get_handler },
        { .uri = "/api/card",   .method = HTTP_POST, .handler = card_post_handler },
        { .uri = "/api/avatar", .method = HTTP_POST, .handler = avatar_post_handler },
        { .uri = "/api/image/*",.method = HTTP_POST, .handler = image_post_handler },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        err = httpd_register_uri_handler(s_server, &routes[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "注册路由 %s 失败: %s", routes[i].uri, esp_err_to_name(err));
            httpd_stop(s_server);
            s_server = NULL;
            return err;
        }
    }

    ESP_LOGI(TAG, "编辑页已就绪: http://%s/", app_wifi_ap_ip());
    return ESP_OK;
}

void app_http_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
