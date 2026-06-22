#!/bin/bash
# 更新 Web 烧录固件包（单文件版本）
# 用法: ./update_firmware.sh [版本号]

VERSION=${1:-"1.1"}
BUILD_DIR=".pio/build/esp32dev"
FIRMWARE_DIR="firmware"

echo "=== CYD Monitor 固件打包工具 ==="
echo "版本: v${VERSION}"
echo ""

# 检查是否已编译
if [ ! -f "$BUILD_DIR/firmware.bin" ]; then
    echo "❌ 错误: 未找到编译后的固件，请先运行 'pio run' 编译项目"
    exit 1
fi

# 创建固件目录
mkdir -p "$FIRMWARE_DIR"

# 生成合并固件 (ESP32 标准偏移量)
echo "📦 生成合并固件..."
echo "   bootloader: 0x1000"
echo "   partitions: 0x8000"
echo "   otadata:    0xe000"
echo "   app:        0x10000"

# 查找 boot_app0.bin
BOOT_APP0=$(find ~/.platformio/packages -name "boot_app0.bin" 2>/dev/null | head -1)

if [ ! -f "$BOOT_APP0" ]; then
    echo "❌ 错误: 未找到 boot_app0.bin"
    exit 1
fi

esptool --chip esp32 merge-bin \
    --flash-mode dio \
    --flash-size 4MB \
    --flash-freq 40m \
    --output "$FIRMWARE_DIR/firmware.bin" \
    0x1000 "$BUILD_DIR/bootloader.bin" \
    0x8000 "$BUILD_DIR/partitions.bin" \
    0xe000 "$BOOT_APP0" \
    0x10000 "$BUILD_DIR/firmware.bin"

# 更新 manifest.json（单文件版本）
echo "📝 更新 manifest.json..."
cat > "$FIRMWARE_DIR/manifest.json" << EOF
{
  "name": "CYD Monitor",
  "version": "${VERSION}",
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "firmware.bin",
          "offset": 0
        }
      ]
    }
  ]
}
EOF

# 显示文件大小
echo ""
echo "✅ 固件打包完成！"
echo ""
ls -lh "$FIRMWARE_DIR"
echo ""
echo "🌐 部署方式:"
echo "   1. 本地测试: python3 -m http.server 8080"
echo "      然后访问: http://localhost:8080/install.html"
echo ""
echo "   2. 命令行烧录:"
echo "      esptool.py -p /dev/cu.usbserial-2110 write-flash 0x0 firmware/firmware.bin"