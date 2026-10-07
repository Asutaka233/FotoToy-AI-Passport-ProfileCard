// tests/test_app_images_layout.c —— 图像分区槽位布局(4KB 对齐/总字节)测试。
#include <assert.h>

#include "app_images_layout.h"

int main(void)
{
    // 槽位尺寸。
    assert(app_images_slot_size(0) == APP_AVATAR_BYTES);   // 96*96*2
    for (int i = 1; i <= APP_IMAGE_PAGE_COUNT; i++) {
        assert(app_images_slot_size(i) == APP_PAGE_BYTES); // 240*320*2
    }

    // 每个槽的擦除尺寸都 4KB 对齐且不小于原始尺寸。
    for (int i = 0; i <= APP_IMAGE_PAGE_COUNT; i++) {
        assert(app_images_slot_erase_size(i) >= app_images_slot_size(i));
        assert(app_images_slot_erase_size(i) % 4096 == 0);
    }

    // 起始偏移 4KB 对齐、单调递增、与前槽擦除尺寸衔接。
    assert(app_images_slot_offset(0) == 0);
    for (int i = 1; i <= APP_IMAGE_PAGE_COUNT; i++) {
        size_t off = app_images_slot_offset(i);
        assert(off % 4096 == 0);
        assert(off == app_images_slot_offset(i - 1) + app_images_slot_erase_size(i - 1));
    }

    // 分区总字节 = 末槽偏移 + 末槽擦除尺寸,且 4KB 对齐。
    size_t total = app_images_partition_bytes();
    assert(total == app_images_slot_offset(APP_IMAGE_PAGE_COUNT)
                    + app_images_slot_erase_size(APP_IMAGE_PAGE_COUNT));
    assert(total % 4096 == 0);
    assert(total == 1110016U);  // 0x10F000,与 partitions.csv 的 0x110000 分区留有余量

    // 槽位合法性。
    assert(app_images_slot_valid(0));
    assert(app_images_slot_valid(APP_IMAGE_PAGE_COUNT));
    assert(!app_images_slot_valid(-1));
    assert(!app_images_slot_valid(APP_IMAGE_PAGE_COUNT + 1));

    return 0;
}
