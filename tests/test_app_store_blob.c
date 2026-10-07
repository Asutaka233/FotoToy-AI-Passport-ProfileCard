// tests/test_app_store_blob.c —— 持久化记录序列化/CRC 校验测试。
#include <assert.h>
#include <string.h>

#include "app_store_blob.h"

int main(void)
{
    // CRC-32 标准测试向量(zlib)。
    assert(app_store_crc32("", 0) == 0x00000000U);
    assert(app_store_crc32("123456789", 9) == 0xCBF43926U);

    // 往返:model -> blob -> model 应无损。
    app_model_t src;
    app_model_init(&src);
    strcpy(src.card.name, "张三");
    strcpy(src.card.unit, "合肥工业大学");
    strcpy(src.card.position, "嵌入式工程师");
    strcpy(src.card.signature, "Hello, world!");
    src.image_present = (uint8_t)((1u << 0) | (1u << 3) | (1u << 7));

    app_store_blob_t blob;
    app_store_blob_from_model(&src, &blob);
    assert(blob.magic == APP_STORE_MAGIC);
    assert(blob.version == APP_STORE_VERSION);
    assert(blob.size == sizeof(blob));

    app_model_t dst;
    app_model_init(&dst);
    assert(app_store_blob_to_model(&blob, &dst));
    assert(strcmp(dst.card.name, "张三") == 0);
    assert(strcmp(dst.card.unit, "合肥工业大学") == 0);
    assert(strcmp(dst.card.position, "嵌入式工程师") == 0);
    assert(strcmp(dst.card.signature, "Hello, world!") == 0);
    assert(dst.image_present == src.image_present);

    // 超长文字应被截断且仍以 '\0' 结尾。
    app_model_t longm;
    app_model_init(&longm);
    memset(longm.card.name, 'A', sizeof(longm.card.name) - 1);
    longm.card.name[sizeof(longm.card.name) - 1] = '\0';
    app_store_blob_from_model(&longm, &blob);
    assert(blob.name[sizeof(blob.name) - 1] == '\0');

    // 损坏检测:改一个字节 -> 校验失败,且不修改目标 model。
    app_model_t before = src;
    blob.name[0] ^= 0x01;
    assert(!app_store_blob_to_model(&blob, &before));
    assert(strcmp(before.card.name, src.card.name) == 0);

    // 错误 magic / version / size。
    app_store_blob_from_model(&src, &blob);
    blob.magic = 0xDEADBEEF;
    assert(!app_store_blob_to_model(&blob, &before));
    app_store_blob_from_model(&src, &blob);
    blob.version = 99;
    assert(!app_store_blob_to_model(&blob, &before));
    app_store_blob_from_model(&src, &blob);
    blob.size = 1;
    assert(!app_store_blob_to_model(&blob, &before));

    return 0;
}
