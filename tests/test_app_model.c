// tests/test_app_model.c —— 纯模型/导航/电量格式化测试。
#include <assert.h>
#include <string.h>

#include "app_model.h"

int main(void)
{
    app_model_t m;
    app_model_init(&m);
    assert(m.current_page == 0);
    assert(m.battery_soc == -1);
    assert(!m.upload_mode);
    assert(m.image_present == 0);
    assert(m.card.name[0] == '\0');

    // 页面循环:0..APP_PAGE_COUNT-1。
    for (int i = 0; i < APP_PAGE_COUNT; i++) {
        assert(m.current_page == i);
        app_model_page_next(&m);
    }
    assert(m.current_page == 0);  // 绕回

    app_model_page_prev(&m);
    assert(m.current_page == APP_PAGE_COUNT - 1);  // 反向绕回

    // 名片页判定与槽位映射。
    m.current_page = 0;
    assert(app_model_is_card_page(&m));
    assert(app_model_page_slot(&m) == 0);
    m.current_page = 3;
    assert(!app_model_is_card_page(&m));
    assert(app_model_page_slot(&m) == 3);

    // 图像存在位管理。
    app_model_set_image(&m, 0, true);
    app_model_set_image(&m, 7, true);
    assert(app_model_has_image(&m, 0));
    assert(app_model_has_image(&m, 7));
    assert(!app_model_has_image(&m, 3));
    app_model_set_image(&m, 0, false);
    assert(!app_model_has_image(&m, 0));
    // 越界槽位被忽略。
    app_model_set_image(&m, -1, true);
    app_model_set_image(&m, APP_IMAGE_PAGE_COUNT + 1, true);
    assert(m.image_present == (uint8_t)(1u << 7));

    // 电量格式化。
    char buf[16];
    app_model_format_soc(buf, sizeof(buf), 85);
    assert(strcmp(buf, "[85]%") == 0);
    app_model_format_soc(buf, sizeof(buf), 0);
    assert(strcmp(buf, "[0]%") == 0);
    app_model_format_soc(buf, sizeof(buf), 100);
    assert(strcmp(buf, "[100]%") == 0);
    app_model_format_soc(buf, sizeof(buf), -1);
    assert(strcmp(buf, "[--]%") == 0);
    app_model_format_soc(buf, sizeof(buf), 101);
    assert(strcmp(buf, "[--]%") == 0);

    return 0;
}
