// main/app_store.c —— NVS 持久化。记录为一个带 CRC 校验的 blob,版本不符/损坏时回退默认。
#include "app_store.h"
#include "app_store_blob.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "app_store";
static nvs_handle_t s_nvs;
static bool s_ready;

esp_err_t app_store_init(app_model_t *model)
{
    if (s_ready) return ESP_OK;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 需要恢复: %s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err == ESP_OK) err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 初始化失败: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 打开失败: %s", esp_err_to_name(err));
        return err;
    }

    app_store_blob_t blob;
    size_t size = sizeof(blob);
    err = nvs_get_blob(s_nvs, "state", &blob, &size);
    if (err == ESP_OK) {
        if (size == sizeof(blob)) {
            if (!app_store_blob_to_model(&blob, model)) {
                ESP_LOGW(TAG, "已存记录无效,使用默认值");
            }
        } else {
            // 旧版本记录尺寸不同(v1 的 identity 拆分为 v2 的 unit+position),无法复用,
            // 擦掉旧键避免每次启动都告警,下次保存会写入新尺寸。
            ESP_LOGW(TAG, "记录尺寸不符(旧 %u → 当前 %u),忽略并清除旧数据",
                     (unsigned)size, (unsigned)sizeof(blob));
            nvs_erase_key(s_nvs, "state");
            nvs_commit(s_nvs);
        }
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "读取记录失败: %s", esp_err_to_name(err));
    }

    s_ready = true;
    return ESP_OK;
}

esp_err_t app_store_save(const app_model_t *model)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;

    app_store_blob_t blob;
    app_store_blob_from_model(model, &blob);

    esp_err_t err = nvs_set_blob(s_nvs, "state", &blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "保存记录失败: %s", esp_err_to_name(err));
    }
    return err;
}
