idf_component_register(
    SRCS "main.c"
         "app_model.c"
         "app_store_blob.c"
         "app_images_layout.c"
         "app_store.c"
         "app_images.c"
         "app_fonts.c"
         "app_ui.c"
         "app_wifi.c"
         "app_http.c"
    INCLUDE_DIRS "."
    EMBED_FILES "edit_page.html"
    REQUIRES bsp esp_event esp_http_server esp_netif esp_partition esp_wifi json nvs_flash
)

# 内嵌中文字体(lv_font_conv 生成,见 assets/fonts/)。文件位于组件目录之外,
# 用绝对路径编入本组件,避免改 SRCS 列表(生成的 .c 自带 lvgl.h 依赖)。
target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_cjk_16.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_cjk_20.c"
)
