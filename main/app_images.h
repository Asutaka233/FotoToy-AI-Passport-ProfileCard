// main/app_images.h —— 图像分区:上电 mmap 只读窗口 + 上传时擦写槽位。
// 显示层通过 app_images_slot_ptr() 直接取只读 RGB565 指针,不复制到 RAM。
#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
#include "app_images_layout.h"

// 定位并 mmap 图像分区。成功后可用 app_images_slot_ptr() 取槽位只读指针。
esp_err_t app_images_init(void);

// 槽位在 mmap 窗口中的只读指针(未初始化/槽位无效返回 NULL)。
const uint8_t *app_images_slot_ptr(int slot);

// 把 RGB565 数据一次性写入槽位(先擦后写)。len 必须等于 app_images_slot_size(slot)。
esp_err_t app_images_write_slot(int slot, const uint8_t *data, size_t len);

// 流式写入的两个步骤:先擦除槽位,再按 4KB 对齐的偏移分块写入。
// 供上传接口边收边写,避免为整帧(150KB)在堆上开大缓冲。
esp_err_t app_images_erase_slot(int slot);
esp_err_t app_images_write_slot_part(int slot, size_t offset,
                                     const uint8_t *data, size_t len);
