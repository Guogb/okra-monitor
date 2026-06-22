/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.4.0
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH 16

/*Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
#define LV_MEM_SIZE (48U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/

/*Default display refresh period. LVG will redraw changed areas with this period time*/
#define LV_DISP_DEF_REFR_PERIOD 30      /*[ms]*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD 10     /*[ms] - 更频繁地读取输入*/

/*Use a custom tick source that tells the elapsed time in milliseconds.*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/*Default Dot Per Inch.*/
#define LV_DPI_DEF 130

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*1: Enable the Animations */
#define LV_USE_ANIMATION 1

/*1: Enable shadow drawing on rectangles*/
#define LV_USE_SHADOW 0

/*1: Enable outline drawing on rectangles*/
#define LV_USE_OUTLINE 0

/*1: Enable pattern drawing on rectangles*/
#define LV_USE_PATTERN 0

/*1: Enable value strings drawing on rectangles*/
#define LV_USE_VALUE_STR 0

/*1: Enable drawing gradient background*/
#define LV_USE_GRADIENT 0

/*================
 *  FONT USAGE
 *===============*/

#define LV_FONT_MONTSERRAT_8 0
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_48 0

/*Enable it if you have an external font file*/
#define LV_USE_USER_DATA 1

/*Enable large font support for fonts with large glyphs*/
#define LV_FONT_FMT_TXT_LARGE 1

/*===================
 *  MODULES USAGE
 *==================*/

/*QR Code library*/
#define LV_USE_QRCODE 1

/*==================
 *  EXTRA LIBS
 *=================*/

/*Enable extra libraries*/
#define LV_USE_EXTRA 1

/*==================
* EXAMPLES
*=================*/

/*Enable the examples to be built with the library*/
#define LV_BUILD_EXAMPLES 0

#endif /*LV_CONF_H*/
