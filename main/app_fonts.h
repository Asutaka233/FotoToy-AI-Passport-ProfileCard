// main/app_fonts.h —— 中文字体集成与字形覆盖检查。
// 使用 lv_font_conv 生成的 GB2312 全集子集(16/20px)显示中文,回退 Montserrat 补 ASCII。
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

// 应用主字体(中文),回退链补 ASCII/数字。
const lv_font_t *app_font_zh(void);
// 辅助英文/数字字体。
const lv_font_t *app_font_en(void);
// 大号字体(姓名/标题等),20px 中文 + ASCII。
const lv_font_t *app_font_big(void);

// 初始化字体回退描述符(须在 LVGL 初始化后、创建控件前调用一次)。
void app_fonts_init(void);

// 检查字体是否包含某 Unicode 码点(覆盖检查,非渲染验证)。
bool app_font_has_glyph(const lv_font_t *font, uint32_t codepoint);
