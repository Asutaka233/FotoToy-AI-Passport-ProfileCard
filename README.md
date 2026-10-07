# FotoToy-AI-Passport-ProfileCard

基于 [FoloToy AI Passport](https://ai-passport.folotoy.cn/) 开源可穿戴硬件（ESP32-C3）打造的一款**电子名片**固件。

开机即显示一张深色赛博风个人名片（头像、姓名、单位、职务、个性签名、电量），上下键可翻阅 7 张全屏展示图，可以放置社交平台二维码；长按 OK 键开启 Wi-Fi 热点，通过热点对内容进行编辑。

首页名片设计借鉴[*赛博工牌：你的数字身份卡*](https://ai-passport.folotoy.cn/plays/214/)。本项目通过Claude Code使用deepseek-v4-pro API进行开发。

---

## 功能特性

- **名片首页**：头像（96×96）+ 信息（姓名 / 单位 / 职务）+ 个性签名 + 电量指示。
- **7 张展示图**：通过上下键切换。
- **按键操作**：
  - 上 / 下 短按：切换页面（0 = 名片页，1–7 = 展示图页）。
  - OK 长按：进入 / 退出热点编辑模式。
- **网页编辑**：
  - 修改姓名、单位、职务、个性签名；
  - 上传头像（96×96）与 7 张展示图（240×320）。
- **掉电不丢**：文字存入 NVS，图像存入独立 `images` Flash 分区。
- **中文支持**：内置 GB2312 全集子集字体（16px / 20px），可显示任意常用中文。

## 快速上手

### 方式一：烧录预编译固件

项目已提供可直接烧录的完整合并镜像（含 bootloader + 分区表 + 应用）。将设备通过 USB 连接电脑后执行：

```bash
esptool.py --chip esp32c3 -p /dev/cu.usbmodem1101 -b 460800 \
  --before default_reset --after hard_reset \
  write_flash 0x0 ai-passport-20261007-full.bin
```

> 将 `/dev/cu.usbmodem1101` 替换为你本机的串口设备。合并镜像可用 `esptool.py merge_bin` 由构建产物生成（见下文）。

### 方式二：从源码构建

**环境要求**

- [ESP-IDF v5.5.3](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32c3/get-started/)（已配置 ESP32-C3 工具链）
- Python 3.9+（随 ESP-IDF 安装环境提供）

**构建**

```bash
# 激活 ESP-IDF 环境（路径按你的安装位置调整）
. ~/esp/esp-idf-v5.5.3/export.sh

# 编译
idf.py build

# 烧录（bootloader + 分区表 + 应用）
idf.py -p /dev/cu.usbmodem1101 flash
```

**生成单文件合并镜像（可选，方便分发）**

```bash
esptool.py --chip esp32c3 merge_bin -o ai-passport-full.bin \
  --flash_mode dio --flash_size 8MB --flash_freq 80m \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/FoloToy-AI-Passport.bin
```

## 使用说明

### 编辑名片文字 / 上传图像

1. **长按 OK 键**，设备进入热点编辑模式，屏幕显示热点名、密码与访问地址。
2. 手机连接 Wi-Fi 热点：
   - SSID：`PassportCard`
   - 密码：`12345678`
   - 地址：`http://192.168.4.1/`（浏览器打开）
3. 在网页上填写姓名、单位、职务、个性签名，点击「保存文字」；选择图片上传头像与展示图。
4. 完成后再次**长按 OK 键**退出编辑模式。

> 热点 SSID、密码、最大连接数等参数集中在 `main/app_config.h`（`APP_AP_SSID` / `APP_AP_PASSWORD`）。

## 目录结构

```
├── main/                  # 应用源码（UI、HTTP、模型、持久化、字体）
│   ├── app_ui.c           # 名片页 / 图像页 / 上传模式界面
│   ├── app_http.c         # 编辑页与上传接口（GET /、POST /api/*）
│   ├── app_model.c        # 纯数据模型与页面导航
│   ├── app_store*.c       # NVS 持久化（带 CRC 校验）
│   ├── app_images*.c      # 图像分区读写
│   └── edit_page.html     # 内置网页编辑页
├── components/bsp/        # 硬件抽象（显示、按键、电量、I2C）
├── assets/fonts/          # 中文字体（GB2312 子集，lv_font_conv 生成）
├── tests/                 # 主机单元测试（模型 / 持久化）
├── partitions.csv         # 分区表（含 images 图像分区）
└── sdkconfig.defaults     # 默认 SDK 配置（LVGL 等）
```

## 构建要点

- **LVGL 9.5.0**（`esp_lvgl_port`）：单缓冲 40 行，全局圆角 30。
- **中文字体**：由 `lv_font_conv` 从 Noto Sans CJK SC 生成 GB2312-80 全集子集（16/20px，bpp 4），源文件与生成命令见 `assets/fonts/`。
- **LVGL 内存池**：`CONFIG_LV_MEM_SIZE_KILOBYTES=48`，过小会在整屏绘制时触发 `LV_ASSERT_MALLOC` 并卡死看门狗。
- 本玩法仅使用 Wi-Fi 热点与 HTTP 服务，已关闭蓝牙以节省 RAM / Flash。

