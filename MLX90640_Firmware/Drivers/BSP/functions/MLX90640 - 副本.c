/**
 * @file    thermal_display.c
 * @brief   基于 MLX90640 的热成像图像显示（优化版）
 *          将原始 400+ 行代码重构为模块化函数，提升可读性和可维护性。
 *          使用定点整数运算替代浮点，提高嵌入式平台执行效率。
 */

#include "MLX90640.h"
#include "lcd.h"
#include <stdint.h>
#include <stddef.h>

// ========================== 外部变量声明 ==========================
extern float mlx90640To[768];       // 温度数据（已放大10倍并平移）
extern float emissivity;            // 辐射系数（0.xx）
extern float Ta;                    // 外壳温度

// ========================== 常量定义 ==========================
// 色条颜色表（256色，RGB565格式）
static const uint16_t camColors[] = {
    0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,
    0x3810,0x3010,0x3010,0x3010,0x2810,0x2810,0x2810,0x2810,
    0x2010,0x2010,0x2010,0x1810,0x1810,0x1811,0x1811,0x1011,
    0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,0x0011,
    0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,
    0x00B2,0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,
    0x0152,0x0172,0x0192,0x0192,0x01B2,0x01D2,0x01F3,0x01F3,
    0x0213,0x0233,0x0253,0x0253,0x0273,0x0293,0x02B3,0x02D3,
    0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,0x0394,
    0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,
    0x0474,0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,
    0x0554,0x0554,0x0574,0x0574,0x0573,0x0573,0x0573,0x0572,
    0x0572,0x0572,0x0571,0x0591,0x0591,0x0590,0x0590,0x058F,
    0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,0x05AD,
    0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,
    0x05C9,0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,
    0x05E6,0x05E6,0x05E5,0x05E5,0x0604,0x0604,0x0604,0x0603,
    0x0603,0x0602,0x0602,0x0601,0x0621,0x0621,0x0620,0x0620,
    0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,0x1E40,
    0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,
    0x3E60,0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,
    0x5E80,0x5E80,0x6680,0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,
    0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,0x8EC0,0x96C0,0x96C0,
    0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,0xBEE0,
    0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,
    0xDEE0,0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,
    0xE600,0xE5E0,0xE5C0,0xE5A0,0xE580,0xE560,0xE540,0xE520,
    0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,0xE460,0xEC40,0xEC20,
    0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,0xEB20,
    0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,
    0xF200,0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,
    0xF0E0,0xF0C0,0xF0A0,0xF080,0xF060,0xF040,0xF020,
    0x0000,0xFFFF
};

// 传感器像素坐标映射表（32x24 到屏幕坐标）
static const uint8_t Pos_x[] = {
    159,155,150,145,140,134,129,124,119,114,109,103,
    98 ,93 ,88 ,83 ,78 ,72 ,67 ,62 ,57 ,52 ,47 ,41,
    36 ,31 ,26 ,21 ,16 ,10 ,5  ,0
};
static const uint8_t Pos_y[] = {
    0  ,5  ,10 ,16 ,21 ,26 ,31 ,37 ,42 ,47 ,52 ,57 ,
    63 ,68 ,73 ,78 ,84 ,89 ,94 ,99 ,104,110,115,119
};

// ========================== 图像与屏幕参数 ==========================
#define SRC_W       32      // 传感器原始宽度
#define SRC_H       24      // 传感器原始高度
#define DST_W       160     // 目标显示宽度（放大5倍）
#define DST_H       120     // 目标显示高度（放大5倍）
#define SCALE_X     198     // 水平缩放因子（0.19375 * 1024）
#define SCALE_Y     196     // 垂直缩放因子（0.19166 * 1024）

#define SCREEN_W    320
#define SCREEN_H    240

// ========================== 辅助函数 ==========================

/**
 * @brief 安全读取温度数组，防止越界
 */
static float SafeGetMLXData(int index)
{
    if (index < 0 || index >= 768) return 0.0f;
    return mlx90640To[index];
}

/**
 * @brief 将逻辑Y坐标转换为物理屏幕Y坐标（如果LCD原点在右下角可在此翻转）
 */
static inline uint16_t ConvertToPhysicalY(uint16_t logical_y)
{
    // 若屏幕从上往下刷新，直接返回
    return logical_y;
    // 若屏幕从下往上刷新，使用：return SCREEN_H - 1 - logical_y;
}

/**
 * @brief 绘制一个2x2像素块（同一颜色）
 */
static void Draw2x2Block(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t y0 = ConvertToPhysicalY(y);
    uint16_t y1 = ConvertToPhysicalY(y + 1);
    lcd_draw_point(x,     y0, color);
    lcd_draw_point(x + 1, y0, color);
    lcd_draw_point(x,     y1, color);
    lcd_draw_point(x + 1, y1, color);
}

/**
 * @brief 根据双线性插值计算当前像素颜色（纯整数定点运算）
 * @param fx  水平方向定点坐标（0~1023表示一个源像素）
 * @param fy  垂直方向定点坐标
 * @param min 当前帧温度最小值（整数，放大10倍且平移后的值）
 * @param scale 缩放因子 = 2530 / (max - min)
 * @return  RGB565颜色值
 */
static uint16_t GetThermalColor(uint32_t fx, uint32_t fy, uint16_t min, uint16_t scale)
{
    uint8_t sx = fx >> 10;          // 源X整数部分（0-31）
    uint8_t sy = fy >> 10;          // 源Y整数部分（0-23）
    uint16_t u = fx & 0x3FF;        // 水平插值权重（0-1023）
    uint16_t v = fy & 0x3FF;        // 垂直插值权重
    uint16_t u0 = 1024 - u;
    uint16_t v0 = 1024 - v;

    int color_idx = sy * SRC_W + (SRC_W - 1 - sx);  // 传感器数据索引（0~767）

    // 边界安全：确保四个邻域点都存在
    if (color_idx >= 1 && (color_idx + SRC_W) < 768 && (color_idx + SRC_W - 1) < 768) {
        uint32_t sum = (uint32_t)SafeGetMLXData(color_idx)          * u0 * v0
                     + (uint32_t)SafeGetMLXData(color_idx + SRC_W)  * u0 * v
                     + (uint32_t)SafeGetMLXData(color_idx - 1)      * u  * v0
                     + (uint32_t)SafeGetMLXData(color_idx + SRC_W - 1) * u  * v;
        uint16_t dst = sum >> 20;                // 除以 1048576 (1024*1024)
        uint16_t index = ((dst - min) * scale) / 10;
        if (index > 255) index = 255;
        return camColors[index];
    } else {
        return camColors[0];  // 边界使用色条最低色
    }
}

/**
 * @brief 绘制一整行红外图像（放大2倍高度，每行对应2个屏幕像素）
 * @param fy        当前行对应的源Y定点坐标
 * @param screen_y  屏幕起始Y坐标（像素级）
 * @param min       当前帧温度最小值
 * @param scale     缩放因子
 */
static void DrawThermalRow(uint32_t fy, uint16_t screen_y, uint16_t min, uint16_t scale)
{
    uint32_t fx = 0;
    for (uint16_t x = 0; x < DST_W; x++) {
        uint16_t color = GetThermalColor(fx, fy, min, scale);
        Draw2x2Block(x * 2, screen_y, color);
        fx += SCALE_X;
    }
}

/**
 * @brief 绘制前5行（特殊布局：左侧图像 + 色条 + 右侧图像）
 * @param pfy       指向当前Y定点坐标的指针（会被更新）
 * @param row_count 行数（固定5）
 * @param min       温度最小值
 * @param scale     缩放因子
 */
static void DrawTopRows(uint32_t *pfy, uint16_t row_count, uint16_t min, uint16_t scale)
{
    for (uint16_t row = 0; row < row_count; row++) {
        uint16_t screen_y_base = row * 2;
        uint32_t fy = *pfy;

        // 左侧图像（源列 0~15，放大2倍）
        uint32_t fx = 0;
        for (uint16_t x = 0; x < 16; x++) {
            uint16_t color = GetThermalColor(fx, fy, min, scale);
            Draw2x2Block(x * 2, screen_y_base, color);
            fx += SCALE_X;
        }

        // 中间色条（256级，每级1像素宽）
        for (uint16_t x = 0; x < 256; x++) {
            uint16_t bar_color = camColors[x];
            lcd_draw_point(32 + x, ConvertToPhysicalY(screen_y_base),     bar_color);
            lcd_draw_point(32 + x, ConvertToPhysicalY(screen_y_base + 1), bar_color);
        }

        // 右侧图像（源列 143~159，放大2倍）
        fx = 143U * SCALE_X;
        for (uint16_t x = 143; x < DST_W; x++) {
            uint16_t color = GetThermalColor(fx, fy, min, scale);
            Draw2x2Block(286 + (x - 143) * 2, screen_y_base, color);
            fx += SCALE_X;
        }

        *pfy += SCALE_Y;
    }
}

/**
 * @brief 连续绘制多行完整红外图像（标准布局）
 * @param pfy           指向Y定点坐标的指针（会被更新）
 * @param screen_y_start 屏幕起始Y坐标
 * @param row_count     绘制的行数
 * @param min           温度最小值
 * @param scale         缩放因子
 */
static void DrawFullThermalRegion(uint32_t *pfy, uint16_t screen_y_start, uint16_t row_count,
                                  uint16_t min, uint16_t scale)
{
    for (uint16_t row = 0; row < row_count; row++) {
        DrawThermalRow(*pfy, screen_y_start + row * 2, min, scale);
        *pfy += SCALE_Y;
    }
}

/**
 * @brief 绘制十字光标
 * @param center_x 中心X坐标
 * @param center_y 中心Y坐标
 * @param radius   半径（像素）
 * @param color    颜色值
 */
static void DrawCrosshair(uint16_t center_x, uint16_t center_y, uint16_t radius, uint16_t color)
{
    for (uint16_t x = center_x - radius; x <= center_x + radius; x++) {
        lcd_draw_point(x, center_y, color);
    }
    for (uint16_t y = center_y - radius; y <= center_y + radius; y++) {
        lcd_draw_point(center_x, y, color);
    }
}

/**
 * @brief 在温度极值点处绘制标记（十字线）
 * @param pos   传感器数组索引（0~767）
 * @param color 标记颜色
 */
static void DrawValueMarker(uint16_t pos, uint16_t color)
{
    if (pos <= 31) return;   // 忽略边缘点（一般不会在边界）
    uint16_t mark_x = Pos_x[pos % SRC_W] * 2;
    uint16_t mark_y = ConvertToPhysicalY(Pos_y[pos / SRC_W] * 2);
    for (uint16_t x = mark_x - 7; x <= mark_x + 7; x++) {
        lcd_draw_point(x, mark_y, color);
    }
    for (uint16_t y = mark_y - 7; y <= mark_y + 7; y++) {
        lcd_draw_point(mark_x, y, color);
    }
}

// ========================== 主显示函数 ==========================
/**
 * @brief 显示热成像图像及所有OSD信息
 * @note  基于全局数组 mlx90640To 的温度数据，自动计算最大/最小值并映射颜色
 */
void Disp_TempPic(void)
{
    uint16_t max = 0;
    uint16_t min = 3400;   // 温度范围：-40~300℃ 放大10倍后平移400 -> 0~3400
    uint16_t scale;
    uint16_t Pos_max = 0, Pos_min = 0;
    uint32_t fy = 0;

    // 1. 计算当前帧温度的最大/最小值及对应位置
    for (uint16_t i = 0; i < 768; i++) {
        float value = mlx90640To[i];
        if (value > max) {
            max = (uint16_t)value;
            Pos_max = i;
        }
        if (value < min) {
            min = (uint16_t)value;
            Pos_min = i;
        }
    }
    if (max == min) {
        max = min + 1;   // 避免除零
    }
    scale = 2530 / (max - min);   // 缩放到 0~2530，对应256色阶

    // 2. 设置LCD显示窗口（全屏）
    lcd_set_window(0, 0, SCREEN_W - 1, SCREEN_H - 1);

    // 3. 逐区域绘制图像
    DrawTopRows(&fy, 5, min, scale);
    DrawFullThermalRegion(&fy, 10, 5, min, scale);
    DrawFullThermalRegion(&fy, 20, 5, min, scale);
    DrawFullThermalRegion(&fy, 30, 43, min, scale);
    DrawFullThermalRegion(&fy, 116, 5, min, scale);
    DrawCrosshair(160, ConvertToPhysicalY(120), 5, BUF_WHITE);
    DrawFullThermalRegion(&fy, 126, 47, min, scale);
    DrawFullThermalRegion(&fy, 220, 5, min, scale);
    DrawFullThermalRegion(&fy, 230, 5, min, scale);

    // 4. 显示温度极值文字（Min / Max）
    uint16_t bg_color = camColors[0];
    // 上半部分（背景色）
    lcd_fill(4, 10, 4, 8, bg_color);
    lcd_fill(230, 10, 4, 8, bg_color);
    lcd_fill(36, 10, 6, 8, bg_color);
    lcd_fill(262, 10, 6, 8, bg_color);
    Buf_ShowString(4, 10, "Min:", BUF_BLACK, 0);
    Buf_ShowString(230, 10, "Max:", BUF_BLACK, 0);
    Buf_SmallFloatNum(36, 10, min * 10, BUF_BLACK, 0);
    Buf_SmallFloatNum(262, 10, max * 10, BUF_BLACK, 0);

    // 下半部分（使用另一种字体大小）
    lcd_fill(4, 18, 4, 8, bg_color);
    lcd_fill(230, 18, 4, 8, bg_color);
    lcd_fill(36, 18, 6, 8, bg_color);
    lcd_fill(262, 18, 6, 8, bg_color);
    Buf_ShowString(4, 18, "Min:", BUF_BLACK, 1);
    Buf_ShowString(230, 18, "Max:", BUF_BLACK, 1);
    Buf_SmallFloatNum(36, 18, min * 10, BUF_BLACK, 1);
    Buf_SmallFloatNum(262, 18, max * 10, BUF_BLACK, 1);

    // 5. 显示辐射系数和外壳温度
    lcd_fill(4, 220, 4, 8, bg_color);
    lcd_fill(140, 220, 6, 8, bg_color);
    lcd_fill(240, 220, 3, 8, bg_color);
    lcd_fill(264, 220, 6, 8, bg_color);
    Buf_ShowString(4, 220, "e=0.", BUF_BLACK, 0);
    Buf_ShowNum(36, 220, (uint16_t)(emissivity * 100), BUF_BLACK, 0);
    Buf_SmallFloatNum(140, 220, (uint32_t)(mlx90640To[368] * 10), BUF_BLACK, 0);
    Buf_ShowString(240, 220, "Ta:", BUF_BLACK, 0);
    Buf_SmallFloatNum(264, 220, (uint32_t)(Ta * 10), BUF_BLACK, 0);

    lcd_fill(4, 228, 4, 8, bg_color);
    lcd_fill(36, 228, 4, 8, bg_color);
    lcd_fill(140, 228, 6, 8, bg_color);
    lcd_fill(240, 228, 3, 8, bg_color);
    lcd_fill(264, 228, 6, 8, bg_color);
    Buf_ShowString(4, 228, "e=0.", BUF_BLACK, 1);
    Buf_ShowNum(36, 228, (uint16_t)(emissivity * 100), BUF_BLACK, 1);
    Buf_SmallFloatNum(140, 228, (uint32_t)(mlx90640To[368] * 10), BUF_BLACK, 1);
    Buf_ShowString(240, 228, "Ta:", BUF_BLACK, 1);
    Buf_SmallFloatNum(264, 228, (uint32_t)(Ta * 10), BUF_BLACK, 1);

    // 6. 绘制极值点标记
    DrawValueMarker(Pos_max, MAGENTA);
    DrawValueMarker(Pos_min, GREEN);
}

