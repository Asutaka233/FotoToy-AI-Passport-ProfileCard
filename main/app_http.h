// main/app_http.h —— 热点 Web 编辑页 + 上传接口(名片文字 / 头像 / 全屏图像)。
// 浏览器访问 http://<app_wifi_ap_ip()>/ 即可编辑;数据经 NVS + 图像分区持久化。
#pragma once

#include "esp_err.h"
#include "app_model.h"

// 启动 HTTP 服务器。回调中会把 model 改动落盘并刷新 LVGL 界面。
esp_err_t app_http_start(app_model_t *model);

// 停止并释放 HTTP 服务器(退出上传模式时调用)。
void app_http_stop(void);
