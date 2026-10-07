// main/app_store.h —— 把名片文字与图像存在位图持久化到 NVS(掉电不丢)。
#pragma once

#include "esp_err.h"
#include "app_model.h"

// 初始化 NVS 并载入已存记录到 model(card + image_present)。无记录时保持默认。
esp_err_t app_store_init(app_model_t *model);

// 把 model 的 card + image_present 写入 NVS 并提交。
esp_err_t app_store_save(const app_model_t *model);
