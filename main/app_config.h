// main/app_config.h —— 名片应用的产品级常量(非硬件)。硬件事实在 components/bsp/include/bsp_pins.h。
// 页面数、文字长度、图像尺寸、分区/热点参数集中于此,改产品形态只改这一处。
#pragma once

// 页面:0 号页是名片,1..APP_IMAGE_PAGE_COUNT 号页各展示一张全屏图像。
#define APP_IMAGE_PAGE_COUNT 7
#define APP_PAGE_COUNT       (1 + APP_IMAGE_PAGE_COUNT)

// 图像尺寸与字节数(RGB565 = 每像素 2 字节,小端 16bit,与 LVGL RGB565 一致)。
#define APP_AVATAR_W     96
#define APP_AVATAR_H     96
#define APP_AVATAR_BYTES (APP_AVATAR_W * APP_AVATAR_H * 2)   // 18432

#define APP_PAGE_W       240
#define APP_PAGE_H       320
#define APP_PAGE_BYTES   (APP_PAGE_W * APP_PAGE_H * 2)       // 153600

// 名片文字字段的最大字节数(UTF-8;常见中文一个字符 3 字节)。
#define APP_CARD_NAME_MAX      48
#define APP_CARD_UNIT_MAX      48
#define APP_CARD_POSITION_MAX  48
#define APP_CARD_SIGNATURE_MAX 192

// NVS 命名空间与图像分区标签/子类型(与 partitions.csv 保持一致)。
#define APP_NVS_NAMESPACE     "card"
#define APP_IMAGES_PARTITION  "images"
#define APP_IMAGES_SUBTYPE    0x40

// 上传模式(Wi-Fi 热点)参数。SSID/密码会显示在设备屏幕上供用户连接。
#define APP_AP_SSID        "PassportCard"
#define APP_AP_PASSWORD    "12345678"
#define APP_AP_MAX_CONN    1
#define APP_AP_IP_FALLBACK "192.168.4.1"
