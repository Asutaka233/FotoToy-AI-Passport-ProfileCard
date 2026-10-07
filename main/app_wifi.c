// main/app_wifi.c —— SoftAP 热点:上传编辑期间提供 192.168.4.1 的 Web 服务入口。
#include "app_wifi.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_wifi";

static esp_netif_t *s_ap_netif;
static bool s_netif_init;
static bool s_event_loop_init;
static bool s_wifi_init;
static bool s_wifi_started;
static char s_ip[16] = APP_AP_IP_FALLBACK;

static esp_err_t ensure_network(void)
{
    if (!s_netif_init) {
        esp_err_t err = esp_netif_init();
        if (err != ESP_OK) return err;
        s_netif_init = true;
    }
    if (!s_event_loop_init) {
        esp_err_t err = esp_event_loop_create_default();
        if (err != ESP_OK) return err;
        s_event_loop_init = true;
    }
    return ESP_OK;
}

static void teardown_ap(void)
{
    if (s_wifi_started) {
        esp_wifi_stop();
        s_wifi_started = false;
    }
    if (s_wifi_init) {
        esp_wifi_deinit();
        s_wifi_init = false;
    }
    if (s_ap_netif) {
        esp_netif_destroy_default_wifi(s_ap_netif);
        s_ap_netif = NULL;
    }
    strcpy(s_ip, APP_AP_IP_FALLBACK);
}

esp_err_t app_wifi_ap_start(void)
{
    if (s_ap_netif || s_wifi_started) return ESP_OK;  // 已在运行

    esp_err_t err = ensure_network();
    if (err != ESP_OK) return err;

    // 用分步创建以便失败时优雅回退,而不是让便捷接口直接 abort。
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_WIFI_AP();
    s_ap_netif = esp_netif_new(&netif_cfg);
    if (!s_ap_netif) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    err = esp_netif_attach_wifi_ap(s_ap_netif);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_default_wifi_ap_handlers();
    if (err != ESP_OK) goto fail;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) goto fail;
    s_wifi_init = true;

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) goto fail;

    wifi_config_t ap = { 0 };
    strncpy((char *)ap.ap.ssid, APP_AP_SSID, sizeof(ap.ap.ssid) - 1);
    ap.ap.ssid_len = strlen(APP_AP_SSID);
    strncpy((char *)ap.ap.password, APP_AP_PASSWORD, sizeof(ap.ap.password) - 1);
    ap.ap.max_connection = APP_AP_MAX_CONN;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap.ap.channel = 1;

    err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_start();
    if (err != ESP_OK) goto fail;
    s_wifi_started = true;

    // 取实际分配的 IPv4 地址(默认 192.168.4.1)。
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(s_ap_netif, &ip_info) == ESP_OK) {
        esp_ip4addr_ntoa(&ip_info.ip, s_ip, sizeof(s_ip));
    }

    ESP_LOGI(TAG, "热点已开启: SSID=%s IP=%s", APP_AP_SSID, s_ip);
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "热点启动失败: %s", esp_err_to_name(err));
    teardown_ap();
    return err;
}

void app_wifi_ap_stop(void)
{
    teardown_ap();
    ESP_LOGI(TAG, "热点已关闭");
}

const char *app_wifi_ap_ip(void)
{
    return s_ip;
}
