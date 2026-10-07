// main/app_images_layout.c —— 图像槽位布局计算(纯逻辑)。
#include "app_images_layout.h"

#define FLASH_SECTOR 4096U

static size_t align_up(size_t n)
{
    return (n + (FLASH_SECTOR - 1)) & ~(size_t)(FLASH_SECTOR - 1);
}

size_t app_images_slot_size(int slot)
{
    return slot == 0 ? APP_AVATAR_BYTES : APP_PAGE_BYTES;
}

size_t app_images_slot_erase_size(int slot)
{
    return align_up(app_images_slot_size(slot));
}

size_t app_images_slot_offset(int slot)
{
    if (!app_images_slot_valid(slot)) return 0;
    size_t offset = 0;
    for (int i = 0; i < slot; i++) {
        offset += app_images_slot_erase_size(i);
    }
    return offset;
}

size_t app_images_partition_bytes(void)
{
    return app_images_slot_offset(APP_IMAGE_PAGE_COUNT)
           + app_images_slot_erase_size(APP_IMAGE_PAGE_COUNT);
}

bool app_images_slot_valid(int slot)
{
    return slot >= 0 && slot <= APP_IMAGE_PAGE_COUNT;
}
