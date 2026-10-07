// main/app_images.c —— 图像分区的 mmap 只读窗口与擦写。
#include "app_images.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_partition.h"

static const char *TAG = "app_images";
static const esp_partition_t *s_part;
static const void *s_mmap;
static esp_partition_mmap_handle_t s_mmap_handle;

esp_err_t app_images_init(void)
{
    if (s_part) return ESP_OK;

    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      APP_IMAGES_SUBTYPE, APP_IMAGES_PARTITION);
    if (!s_part) {
        ESP_LOGE(TAG, "未找到分区 '%s'", APP_IMAGES_PARTITION);
        return ESP_ERR_NOT_FOUND;
    }
    if (s_part->size < app_images_partition_bytes()) {
        ESP_LOGE(TAG, "分区 '%s' 太小(%u < %u)", APP_IMAGES_PARTITION,
                 (unsigned)s_part->size, (unsigned)app_images_partition_bytes());
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = esp_partition_mmap(s_part, 0, app_images_partition_bytes(),
                                       ESP_PARTITION_MMAP_DATA,
                                       &s_mmap, &s_mmap_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mmap 分区失败: %s", esp_err_to_name(err));
        s_part = NULL;
        return err;
    }
    return ESP_OK;
}

const uint8_t *app_images_slot_ptr(int slot)
{
    if (!s_mmap || !app_images_slot_valid(slot)) return NULL;
    return (const uint8_t *)s_mmap + app_images_slot_offset(slot);
}

esp_err_t app_images_erase_slot(int slot)
{
    if (!s_part) return ESP_ERR_INVALID_STATE;
    if (!app_images_slot_valid(slot)) return ESP_ERR_INVALID_ARG;

    esp_err_t err = esp_partition_erase_range(s_part, app_images_slot_offset(slot),
                                              app_images_slot_erase_size(slot));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "擦除槽 %d 失败: %s", slot, esp_err_to_name(err));
    }
    return err;
}

esp_err_t app_images_write_slot_part(int slot, size_t offset,
                                     const uint8_t *data, size_t len)
{
    if (!s_part) return ESP_ERR_INVALID_STATE;
    if (!app_images_slot_valid(slot)) return ESP_ERR_INVALID_ARG;
    if (offset + len > app_images_slot_size(slot)) return ESP_ERR_INVALID_SIZE;

    esp_err_t err = esp_partition_write(s_part, app_images_slot_offset(slot) + offset,
                                        data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "写入槽 %d(+%u,%u) 失败: %s", slot, (unsigned)offset,
                 (unsigned)len, esp_err_to_name(err));
    }
    return err;
}

esp_err_t app_images_write_slot(int slot, const uint8_t *data, size_t len)
{
    if (len != app_images_slot_size(slot)) return ESP_ERR_INVALID_SIZE;

    esp_err_t err = app_images_erase_slot(slot);
    if (err != ESP_OK) return err;
    return app_images_write_slot_part(slot, 0, data, len);
}
