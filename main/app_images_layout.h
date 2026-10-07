// main/app_images_layout.h —— 图像分区槽位布局(纯计算,主机可测)。
// 每个槽的起始偏移都 4KB 对齐,便于 esp_partition_erase_range 整扇区擦除。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "app_config.h"

// slot:0=头像,1..APP_IMAGE_PAGE_COUNT=全屏图像页。
size_t app_images_slot_offset(int slot);       // 槽在分区内的起始字节偏移(4KB 对齐)
size_t app_images_slot_size(int slot);         // 槽实际图像字节数(未对齐)
size_t app_images_slot_erase_size(int slot);   // 擦除所需字节数(已向上对齐到 4KB)
size_t app_images_partition_bytes(void);       // 分区所需总字节数(4KB 对齐)

// 校验 slot 是否合法(0..APP_IMAGE_PAGE_COUNT)。
bool app_images_slot_valid(int slot);
