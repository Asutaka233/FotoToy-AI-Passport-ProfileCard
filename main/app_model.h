// main/app_model.h —— 名片应用的纯数据模型与页面导航状态机。
// 与 ESP-IDF/LVGL 完全解耦,可在主机上直接编译测试。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "app_config.h"

// 名片文字(UTF-8,固定缓冲)。
typedef struct {
    char name[APP_CARD_NAME_MAX];
    char unit[APP_CARD_UNIT_MAX];
    char position[APP_CARD_POSITION_MAX];
    char signature[APP_CARD_SIGNATURE_MAX];
} app_card_t;

typedef struct {
    app_card_t card;
    // 图像存在位图:bit0=头像,bit1..7=图像页 1..7。
    uint8_t image_present;
    int current_page;   // 0 = 名片页;1..APP_IMAGE_PAGE_COUNT = 图像页
    int battery_soc;    // 0..100;-1 表示电量计不可用
    bool upload_mode;   // 是否处于热点上传模式
} app_model_t;

void app_model_init(app_model_t *model);

// 上/下键切换页面(0..APP_PAGE_COUNT-1 循环)。
void app_model_page_next(app_model_t *model);
void app_model_page_prev(app_model_t *model);

bool app_model_is_card_page(const app_model_t *model);

// 当前页对应的图像槽位:名片页=0(头像),图像页=页号(1..N)。
int app_model_page_slot(const app_model_t *model);

// 图像存在位管理(slot 范围 0..APP_IMAGE_PAGE_COUNT)。
void app_model_set_image(app_model_t *model, int slot, bool present);
bool app_model_has_image(const app_model_t *model, int slot);

// 把电量格式化为 "[85]%" 或 "[--]%"(不可用)。写入不超过 cap 字节(含结尾 '\0')。
// 返回写入(不含 '\0')的字节数。
size_t app_model_format_soc(char *buf, size_t cap, int soc);
