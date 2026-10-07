// main/app_ui.c —— 名片应用界面:深色名片页 + 7 张全屏图像页 + 上传模式屏。
// 约定:所有函数在 LVGL 任务内(或已持有 bsp_lvgl_lock())调用。
#include "app_ui.h"
#include "app_images.h"
#include "app_fonts.h"
#include <stdio.h>

#include "lvgl.h"

// 深色赛博主题(参考设计稿:深海军蓝底 + 暖金点缀 + 冷白文字)。
#define UI_BG      0x0F1A2D   // 屏幕底色(深海军蓝)
#define UI_BG_DEEP 0x040A15   // 更深的顶部/悬浮底色
#define UI_PANEL   0x18263E   // 卡片面板
#define UI_ACCENT  0xF0B040   // 强调色(暖金:头像描边/身份/分隔线)
#define UI_TEXT    0xE0F0F0   // 主文字(冷白)
#define UI_MUTED   0x8FA0B8   // 次要文字(蓝灰)
#define UI_CHIP    0x040A15   // 悬浮小标签底色(压图)

static app_model_t *s_model;

static lv_obj_t *s_screens[APP_PAGE_COUNT];
static lv_obj_t *s_battery[APP_PAGE_COUNT];

// 名片页控件。
static lv_obj_t *s_card_name;
static lv_obj_t *s_card_unit;
static lv_obj_t *s_card_position;
static lv_obj_t *s_card_signature;
static lv_obj_t *s_avatar_img;   // 头像图像(有图时显示)
static lv_obj_t *s_avatar_ph;    // 头像占位(无图时显示)

// 图像页控件(索引 p-1,对应槽位 p=1..APP_IMAGE_PAGE_COUNT)。
static lv_obj_t *s_page_img[APP_IMAGE_PAGE_COUNT];
static lv_obj_t *s_page_ph[APP_IMAGE_PAGE_COUNT];

// 上传模式屏。
static lv_obj_t *s_upload_scr;
static lv_obj_t *s_upload_status;

// 图像描述符(指向 mmap 只读分区,不复制到 RAM)。索引=槽位(0=头像,1..N=图像页)。
static lv_image_dsc_t s_img_dsc[APP_PAGE_COUNT];

// —— 小工具 ---------------------------------------------------------------

static lv_obj_t *new_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    return scr;
}

static lv_obj_t *new_label(lv_obj_t *parent, const char *text,
                           const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}

static lv_obj_t *new_battery(lv_obj_t *scr)
{
    lv_obj_t *chip = lv_obj_create(scr);
    lv_obj_remove_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(chip, lv_color_hex(UI_CHIP), 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_80, 0);
    lv_obj_set_style_radius(chip, 8, 0);
    lv_obj_set_style_border_width(chip, 0, 0);
    lv_obj_set_style_pad_all(chip, 3, 0);
    lv_obj_set_size(chip, 84, 24);
    lv_obj_set_pos(chip, 240 - 84 - 8, 6);

    lv_obj_t *l = lv_label_create(chip);
    lv_label_set_text(l, "BAT [--]%");
    lv_obj_set_style_text_font(l, app_font_en(), 0);
    // 电量文字颜色与左上角 "CYBER ID" 一致(UI_MUTED)。
    lv_obj_set_style_text_color(l, lv_color_hex(UI_MUTED), 0);
    lv_obj_center(l);
    return l;
}

// 一根细金线(赛博分隔线)。
static lv_obj_t *new_rule(lv_obj_t *scr, int y)
{
    lv_obj_t *r = lv_obj_create(scr);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(r, 220, 1);
    lv_obj_set_pos(r, 10, y);
    lv_obj_set_style_bg_color(r, lv_color_hex(UI_ACCENT), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_40, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    return r;
}

// 依据 model 刷新某个槽位的图像/占位可见性。
static void sync_slot_image(int slot, lv_obj_t *img, lv_obj_t *ph)
{
    bool present = app_model_has_image(s_model, slot) && s_img_dsc[slot].data != NULL;
    if (present) {
        lv_image_set_src(img, &s_img_dsc[slot]);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ph, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(img);
    } else {
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ph, LV_OBJ_FLAG_HIDDEN);
    }
}

static void init_img_dsc(void)
{
    for (int slot = 0; slot <= APP_IMAGE_PAGE_COUNT; slot++) {
        const uint8_t *p = app_images_slot_ptr(slot);
        s_img_dsc[slot].data = NULL;
        if (!p) continue;

        uint16_t w = (slot == 0) ? APP_AVATAR_W : APP_PAGE_W;
        uint16_t h = (slot == 0) ? APP_AVATAR_H : APP_PAGE_H;
        s_img_dsc[slot].header.magic = LV_IMAGE_HEADER_MAGIC;
        s_img_dsc[slot].header.cf = LV_COLOR_FORMAT_RGB565;
        s_img_dsc[slot].header.w = w;
        s_img_dsc[slot].header.h = h;
        s_img_dsc[slot].header.stride = w * 2;
        s_img_dsc[slot].data_size = (uint32_t)w * h * 2;
        s_img_dsc[slot].data = p;
    }
}

// —— 名片页 ---------------------------------------------------------------

static void build_card_screen(void)
{
    lv_obj_t *scr = s_screens[0] = new_screen();

    // 顶部:品牌标 + 电量。
    lv_obj_t *brand = new_label(scr, "CYBER ID // 01", app_font_en(), lv_color_hex(UI_MUTED));
    lv_obj_set_pos(brand, 10, 8);
    s_battery[0] = new_battery(scr);
    new_rule(scr, 34);

    // 头像:96x96 图像 + 占位,同一矩形,二选一可见。靠左,信息放右侧。
    const int ax = 8;
    const int ay = 44;

    s_avatar_img = lv_image_create(scr);
    lv_obj_set_size(s_avatar_img, APP_AVATAR_W, APP_AVATAR_H);
    lv_obj_set_pos(s_avatar_img, ax, ay);
    lv_obj_set_style_border_width(s_avatar_img, 2, 0);
    lv_obj_set_style_border_color(s_avatar_img, lv_color_hex(UI_ACCENT), 0);
    lv_obj_set_style_radius(s_avatar_img, 12, 0);
    lv_obj_add_flag(s_avatar_img, LV_OBJ_FLAG_HIDDEN);

    s_avatar_ph = new_label(scr, "头像", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_size(s_avatar_ph, APP_AVATAR_W, APP_AVATAR_H);
    lv_obj_set_pos(s_avatar_ph, ax, ay);
    lv_obj_set_style_text_align(s_avatar_ph, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_border_width(s_avatar_ph, 1, 0);
    lv_obj_set_style_border_color(s_avatar_ph, lv_color_hex(UI_MUTED), 0);
    lv_obj_set_style_radius(s_avatar_ph, 12, 0);
    lv_obj_set_style_bg_color(s_avatar_ph, lv_color_hex(UI_PANEL), 0);

    // 信息区(头像右侧,靠左三行):姓名 / 单位 / 职务。
    const int tx = ax + APP_AVATAR_W + 16;
    const int tw = 240 - tx - 8;

    s_card_name = new_label(scr, "未设置姓名", app_font_big(), lv_color_hex(UI_TEXT));
    lv_obj_set_width(s_card_name, tw);
    lv_label_set_long_mode(s_card_name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_card_name, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(s_card_name, tx, 56);

    s_card_unit = new_label(scr, "未设置单位", app_font_zh(), lv_color_hex(UI_ACCENT));
    lv_obj_set_width(s_card_unit, tw);
    lv_label_set_long_mode(s_card_unit, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_card_unit, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(s_card_unit, tx, 86);

    s_card_position = new_label(scr, "未设置职务", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_width(s_card_position, tw);
    lv_label_set_long_mode(s_card_position, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_card_position, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(s_card_position, tx, 112);

    // 个性签名(头像下方,居中,可换行,「」括起)。
    s_card_signature = new_label(scr, "「未设置个性签名」", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_width(s_card_signature, 220);
    lv_label_set_long_mode(s_card_signature, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_card_signature, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_card_signature, 10, 170);

    // 底部状态栏。
    new_rule(scr, 286);
    lv_obj_t *hint = new_label(scr, "UP/DOWN 翻页 · 长按 OK 上传", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_width(hint, 240);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(hint, 0, 292);
}

// —— 图像页 ---------------------------------------------------------------

static void build_image_screen(int page)
{
    lv_obj_t *scr = s_screens[page] = new_screen();
    int idx = page - 1;

    // 全屏图像 + 占位:纯全屏,不叠加页码、电量等覆盖层。
    s_page_img[idx] = lv_image_create(scr);
    lv_obj_set_size(s_page_img[idx], APP_PAGE_W, APP_PAGE_H);
    lv_obj_set_pos(s_page_img[idx], 0, 0);
    lv_obj_add_flag(s_page_img[idx], LV_OBJ_FLAG_HIDDEN);

    s_page_ph[idx] = new_label(scr, "未上传图像", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_size(s_page_ph[idx], APP_PAGE_W, APP_PAGE_H);
    lv_obj_set_style_text_align(s_page_ph[idx], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_page_ph[idx], 0, 0);
}

// —— 上传模式屏 -----------------------------------------------------------

static void build_upload_screen(void)
{
    s_upload_scr = new_screen();

    lv_obj_t *title = new_label(s_upload_scr, "上传模式", app_font_big(), lv_color_hex(UI_TEXT));
    lv_obj_set_width(title, 240);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(title, 0, 24);

    s_upload_status = new_label(s_upload_scr, "正在开启热点...", app_font_zh(), lv_color_hex(UI_MUTED));
    lv_obj_set_width(s_upload_status, 220);
    lv_label_set_long_mode(s_upload_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_upload_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_upload_status, 10, 90);

    lv_obj_t *hint = new_label(s_upload_scr, "长按 OK 退出", app_font_zh(), lv_color_hex(UI_ACCENT));
    lv_obj_set_width(hint, 240);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(hint, 0, 320 - 30);
}

// —— 对外接口 -------------------------------------------------------------

void app_ui_init(app_model_t *model)
{
    s_model = model;
    init_img_dsc();

    build_card_screen();
    for (int p = 1; p <= APP_IMAGE_PAGE_COUNT; p++) {
        build_image_screen(p);
    }
    build_upload_screen();

    app_ui_sync();
    lv_screen_load(s_screens[0]);
}

void app_ui_show_page(int page)
{
    if (page < 0 || page >= APP_PAGE_COUNT) return;
    lv_screen_load(s_screens[page]);
}

void app_ui_sync(void)
{
    if (!s_model) return;

    char soc[16], bat[24];
    app_model_format_soc(soc, sizeof(soc), s_model->battery_soc);
    snprintf(bat, sizeof(bat), "BAT %s", soc);
    for (int i = 0; i < APP_PAGE_COUNT; i++) {
        if (s_battery[i]) lv_label_set_text(s_battery[i], bat);
    }

    lv_label_set_text(s_card_name, s_model->card.name[0] ? s_model->card.name : "未设置姓名");
    lv_label_set_text(s_card_unit, s_model->card.unit[0] ? s_model->card.unit : "未设置单位");
    lv_label_set_text(s_card_position, s_model->card.position[0] ? s_model->card.position : "未设置职务");
    // 个性签名用「」括起,呼应参考设计稿。两个「」各 3 字节 + NUL,故 +7。
    char sig[APP_CARD_SIGNATURE_MAX + 7];
    if (s_model->card.signature[0]) {
        snprintf(sig, sizeof(sig), "「%s」", s_model->card.signature);
    } else {
        snprintf(sig, sizeof(sig), "「未设置个性签名」");
    }
    lv_label_set_text(s_card_signature, sig);

    sync_slot_image(0, s_avatar_img, s_avatar_ph);
    for (int p = 1; p <= APP_IMAGE_PAGE_COUNT; p++) {
        sync_slot_image(p, s_page_img[p - 1], s_page_ph[p - 1]);
    }
}

void app_ui_enter_upload(void)
{
    lv_screen_load(s_upload_scr);
}

void app_ui_leave_upload(void)
{
    lv_screen_load(s_screens[s_model ? s_model->current_page : 0]);
}

void app_ui_upload_status(const char *text)
{
    if (s_upload_status) lv_label_set_text(s_upload_status, text ? text : "");
}
