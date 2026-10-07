// main/app_wifi.h —— 上传模式的 Wi-Fi 热点(SoftAP)。
// 名片/图像编辑通过浏览器访问热点 Web 页完成,无需路由器。
#pragma once

#include "esp_err.h"

// 启动热点(SSID/密码见 app_config.h)。可重复进入;失败返回错误码。
esp_err_t app_wifi_ap_start(void);

// 停止热点并释放 Wi-Fi/网络资源。
void app_wifi_ap_stop(void);

// 热点 IPv4 地址的点分十进制字符串;未就绪时返回 APP_AP_IP_FALLBACK。
const char *app_wifi_ap_ip(void);
