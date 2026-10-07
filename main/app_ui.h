// main/app_ui.h —— 名片应用界面(深色名片 + 全屏图像页 + 上传模式屏)。
// 所有界面操作都假定在 LVGL 任务内、或已持有 bsp_lvgl_lock()。
#pragma once

#include "app_model.h"

// 构建全部页面并载入首页。须在 LVGL/字体/图像分区初始化后调用一次。
void app_ui_init(app_model_t *model);

// 载入指定页(0=名片,1..N=图像页)的屏幕。
void app_ui_show_page(int page);

// 依据 model 刷新电量、名片文字与图像存在状态。
void app_ui_sync(void);

// 进入/退出上传模式屏幕。
void app_ui_enter_upload(void);
void app_ui_leave_upload(void);
// 更新上传模式屏幕的状态文本(多行)。
void app_ui_upload_status(const char *text);
