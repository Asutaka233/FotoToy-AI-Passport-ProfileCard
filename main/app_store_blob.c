// main/app_store_blob.c —— 持久化记录序列化(纯逻辑)。
#include "app_store_blob.h"
#include <stddef.h>
#include <string.h>

uint32_t app_store_crc32(const void *data, size_t len)
{
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int32_t)(crc & 1));
        }
    }
    return ~crc;
}

static void copy_field(char *dst, size_t dst_cap, const char *src)
{
    // 保证结果以 '\0' 结尾,截断只发生在字符边界之后(整段复制)。
    strncpy(dst, src, dst_cap - 1);
    dst[dst_cap - 1] = '\0';
}

void app_store_blob_from_model(const app_model_t *model, app_store_blob_t *blob)
{
    memset(blob, 0, sizeof(*blob));
    blob->magic = APP_STORE_MAGIC;
    blob->version = APP_STORE_VERSION;
    blob->size = (uint16_t)sizeof(*blob);

    copy_field(blob->name, sizeof(blob->name), model->card.name);
    copy_field(blob->unit, sizeof(blob->unit), model->card.unit);
    copy_field(blob->position, sizeof(blob->position), model->card.position);
    copy_field(blob->signature, sizeof(blob->signature), model->card.signature);
    blob->image_present = model->image_present;

    blob->crc32 = app_store_crc32(blob, offsetof(app_store_blob_t, crc32));
}

bool app_store_blob_to_model(const app_store_blob_t *blob, app_model_t *model)
{
    if (blob->magic != APP_STORE_MAGIC || blob->version != APP_STORE_VERSION ||
        blob->size != sizeof(*blob) ||
        blob->crc32 != app_store_crc32(blob, offsetof(app_store_blob_t, crc32))) {
        return false;
    }

    copy_field(model->card.name, sizeof(model->card.name), blob->name);
    copy_field(model->card.unit, sizeof(model->card.unit), blob->unit);
    copy_field(model->card.position, sizeof(model->card.position), blob->position);
    copy_field(model->card.signature, sizeof(model->card.signature), blob->signature);
    model->image_present = blob->image_present;
    return true;
}
