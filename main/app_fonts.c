// main/app_fonts.c —— 中文字体集成与字形覆盖检查。
// 使用 lv_font_conv 由 Noto Sans SC(OFL)生成的 GB2312 全集子集(16/20px)显示中文,
// 回退 Montserrat 补 ASCII/数字。生成源与字符清单见 assets/fonts/。
#include "app_fonts.h"

LV_FONT_DECLARE(app_font_cjk_16);
LV_FONT_DECLARE(app_font_cjk_20);

// 生成的字体为 const 静态字体;复制成应用自有的可写描述符,再挂 Montserrat 回退,
// 让混合中文+ASCII/数字能正确渲染(中文取 CJK,缺字回退 Montserrat)。
static lv_font_t s_font_zh;   // 16px CJK + Montserrat 14 回退
static lv_font_t s_font_big;  // 20px CJK + Montserrat 20 回退

void app_fonts_init(void)
{
    s_font_zh = app_font_cjk_16;
    s_font_zh.fallback = &lv_font_montserrat_14;

    s_font_big = app_font_cjk_20;
    s_font_big.fallback = &lv_font_montserrat_20;
}

const lv_font_t *app_font_zh(void) { return &s_font_zh; }
const lv_font_t *app_font_en(void) { return &lv_font_montserrat_14; }
const lv_font_t *app_font_big(void) { return &s_font_big; }

bool app_font_has_glyph(const lv_font_t *font, uint32_t codepoint)
{
    if (font == NULL) return false;
    lv_font_glyph_dsc_t glyph = { 0 };
    return lv_font_get_glyph_dsc(font, &glyph, codepoint, 0)
           && !glyph.is_placeholder;
}
