// main/app_model.c —— 纯模型/导航实现,无任何外设依赖。
#include "app_model.h"
#include <stdio.h>
#include <string.h>

void app_model_init(app_model_t *model)
{
    memset(model, 0, sizeof(*model));
    model->current_page = 0;
    model->battery_soc = -1;
    model->upload_mode = false;
}

void app_model_page_next(app_model_t *model)
{
    model->current_page = (model->current_page + 1) % APP_PAGE_COUNT;
}

void app_model_page_prev(app_model_t *model)
{
    model->current_page = (model->current_page - 1 + APP_PAGE_COUNT) % APP_PAGE_COUNT;
}

bool app_model_is_card_page(const app_model_t *model)
{
    return model->current_page == 0;
}

int app_model_page_slot(const app_model_t *model)
{
    return model->current_page;
}

void app_model_set_image(app_model_t *model, int slot, bool present)
{
    if (slot < 0 || slot > APP_IMAGE_PAGE_COUNT) return;
    if (present) {
        model->image_present |= (uint8_t)(1u << slot);
    } else {
        model->image_present &= (uint8_t)~(1u << slot);
    }
}

bool app_model_has_image(const app_model_t *model, int slot)
{
    if (slot < 0 || slot > APP_IMAGE_PAGE_COUNT) return false;
    return (model->image_present & (uint8_t)(1u << slot)) != 0;
}

size_t app_model_format_soc(char *buf, size_t cap, int soc)
{
    if (cap == 0) return 0;
    int written;
    if (soc < 0 || soc > 100) {
        written = snprintf(buf, cap, "[--]%%");
    } else {
        written = snprintf(buf, cap, "[%d]%%", soc);
    }
    return written < 0 ? 0 : (size_t)written;
}
