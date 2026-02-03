#!/bin/bash

SOURCE_DIR="/mnt/c/Users/KCCISTC/Desktop/STM32"
TARGET_DIR="$HOME/stm32-projects"

# 프로젝트 목록
projects=(
    "ADC"
    "DMAtest"
    "IButton"
    "InterruptUART"
    "LCD_I2C"
    "LED"
    "UART"
    "UART_COMMUNICATION"
    "STM32-Dam-Control-Clean"
    "dam_control_system"
)

for project in "${projects[@]}"; do
    echo "Processing: $project"
    
    # 프로젝트 디렉토리 생성
    mkdir -p "$TARGET_DIR/$project"
    
    # Core/Src/main.c 복사
    if [ -f "$SOURCE_DIR/$project/Core/Src/main.c" ]; then
        mkdir -p "$TARGET_DIR/$project/Core/Src"
        cp "$SOURCE_DIR/$project/Core/Src/main.c" "$TARGET_DIR/$project/Core/Src/"
        echo "  ✓ Copied Core/Src/main.c"
    fi
    
    # Core/Inc 폴더 복사 (헤더 파일)
    if [ -d "$SOURCE_DIR/$project/Core/Inc" ]; then
        mkdir -p "$TARGET_DIR/$project/Core/Inc"
        cp -r "$SOURCE_DIR/$project/Core/Inc"/*.h "$TARGET_DIR/$project/Core/Inc/" 2>/dev/null
        echo "  ✓ Copied Core/Inc"
    fi
    
    # App 폴더 복사 (있으면)
    if [ -d "$SOURCE_DIR/$project/App" ]; then
        cp -r "$SOURCE_DIR/$project/App" "$TARGET_DIR/$project/"
        echo "  ✓ Copied App folder"
    fi
    
    # .ioc 파일 복사 (STM32CubeMX 설정)
    find "$SOURCE_DIR/$project" -maxdepth 1 -name "*.ioc" -exec cp {} "$TARGET_DIR/$project/" \;
    
    echo ""
done

echo "✅ Extraction complete!"
