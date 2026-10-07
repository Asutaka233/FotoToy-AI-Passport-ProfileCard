<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

| File | Family / size | Coverage | Source and license |
| --- | --- | --- | --- |
| [`fonts/NotoSansSC-Regular.otf`](fonts/NotoSansSC-Regular.otf) | Noto Sans CJK SC, Regular | Full GB2312-80 + ASCII | [notofonts/noto-cjk](https://github.com/notofonts/noto-cjk), SIL Open Font License 1.1 |
| [`fonts/chars_gb2312.txt`](fonts/chars_gb2312.txt) | character inventory | GB2312-80 (6763 hanzi + 682 symbols) + ASCII 0x20–0x7E | Generated locally from the Python `gb2312` codec |
| [`fonts/app_font_cjk_16.c`](fonts/app_font_cjk_16.c) | 16 px, bpp 4 | Same inventory | Converted from `NotoSansSC-Regular.otf` via `lv_font_conv` 1.5.3 |
| [`fonts/app_font_cjk_20.c`](fonts/app_font_cjk_20.c) | 20 px, bpp 4 | Same inventory | Converted from `NotoSansSC-Regular.otf` via `lv_font_conv` 1.5.3 |

Conversion command (run from the repository root, after `npm install lv_font_conv`):

```bash
lv_font_conv --font assets/fonts/NotoSansSC-Regular.otf \
  --range "$(cat assets/fonts/chars_gb2312.ranges.txt)" \
  --size 16 --bpp 4 --format lvgl --no-compress \
  --lv-include lvgl.h --lv-font-name app_font_cjk_16 \
  --output assets/fonts/app_font_cjk_16.c
```

The generated `app_font_cjk_16.c` / `app_font_cjk_20.c` are compiled into the `main` component via `target_sources` and selected by `app_fonts.c`. The GB2312 inventory is the documented contract for arbitrary user-entered Chinese text (name, identity, signature); characters outside GB2312 render as placeholders.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
