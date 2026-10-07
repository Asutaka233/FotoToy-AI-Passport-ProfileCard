// main/app_store_blob.h —— 名片持久化记录的序列化/反序列化(纯逻辑,主机可测)。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "app_model.h"

// 落盘记录:名片文字 + 图像存在位图。crc32 覆盖其之前的所有字段。
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    char name[APP_CARD_NAME_MAX];
    char unit[APP_CARD_UNIT_MAX];
    char position[APP_CARD_POSITION_MAX];
    char signature[APP_CARD_SIGNATURE_MAX];
    uint8_t image_present;
    uint8_t reserved[3];
    uint32_t crc32;
} app_store_blob_t;

#define APP_STORE_MAGIC   0x43415244U  // "CARD"
#define APP_STORE_VERSION 2   // v2:身份拆分为单位/职务两字段,记录尺寸随之改变

// 标准 CRC-32(与 zlib 一致)。len 覆盖除 crc32 字段外的数据。
uint32_t app_store_crc32(const void *data, size_t len);

// 用 model 的 card + image_present 填充 blob(含 magic/version/size/crc)。
void app_store_blob_from_model(const app_model_t *model, app_store_blob_t *blob);

// 校验 magic/version/size/crc 并解出 model 的 card + image_present。
// 失败返回 false 且不修改 model。
bool app_store_blob_to_model(const app_store_blob_t *blob, app_model_t *model);
