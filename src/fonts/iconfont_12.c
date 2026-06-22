/*******************************************************************************
 * Size: 12 px
 * Bpp: 4
 * Opts: --bpp 4 --size 12 --no-compress --use-color-info --stride 1 --align 1 --font iconfont.ttf --range 58904-58907 --format lvgl -o iconfont_12.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef ICONFONT_12
#define ICONFONT_12 1
#endif

#if ICONFONT_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+E618 "" */
    0x56, 0x66, 0x66, 0x66, 0x65, 0x91, 0x84, 0x87,
    0x58, 0x19, 0x90, 0x76, 0x77, 0x75, 0x28, 0x81,
    0x84, 0x87, 0x59, 0x18, 0x4c, 0xbc, 0x8b, 0xbb,
    0xb5, 0x18, 0x99, 0xa9, 0x77, 0x91, 0xa3, 0xa6,
    0xa9, 0x7b, 0x4a, 0x80, 0x76, 0x77, 0x75, 0x28,
    0x80, 0x84, 0x87, 0x58, 0x7, 0x6c, 0xbc, 0x9b,
    0xbb, 0xb6, 0x6, 0x77, 0x77, 0x54, 0x70,

    /* U+E619 "" */
    0x6, 0x72, 0x0, 0x0, 0x0, 0x0, 0x7, 0xb,
    0x66, 0x66, 0x66, 0x40, 0x8, 0x8, 0x9a, 0x45,
    0x62, 0x64, 0x7, 0xb, 0x85, 0x77, 0xc8, 0x16,
    0x7, 0x7, 0x18, 0xab, 0xb8, 0x66, 0x7, 0x7,
    0x19, 0xab, 0xa9, 0x76, 0x7, 0xb, 0x75, 0x97,
    0xc9, 0x16, 0x7, 0x7, 0x78, 0x25, 0x61, 0x6,
    0x7, 0xc, 0xab, 0xa9, 0xbb, 0x72, 0x8, 0x7,
    0x77, 0x77, 0x76, 0x0, 0x2, 0x72, 0x0, 0x0,
    0x0, 0x0,

    /* U+E61A "" */
    0x0, 0x48, 0x57, 0x83, 0x98, 0x0, 0x2, 0xa8,
    0xaa, 0x8a, 0xaa, 0x60, 0xb, 0x70, 0x0, 0x0,
    0x0, 0xb8, 0xa, 0x77, 0x88, 0x88, 0xa0, 0xb7,
    0x7, 0x78, 0x6, 0x82, 0x70, 0xc3, 0x8, 0x78,
    0x0, 0x0, 0x70, 0x88, 0x7, 0x78, 0x6, 0x82,
    0x70, 0xc3, 0xa, 0x77, 0x77, 0x77, 0xa0, 0xc7,
    0xb, 0x70, 0x0, 0x0, 0x0, 0xc7, 0x0, 0xb9,
    0xdd, 0xac, 0xcc, 0x30, 0x0, 0x48, 0x57, 0x83,
    0x88, 0x0,

    /* U+E61B "" */
    0x0, 0x73, 0x0, 0x0, 0x73, 0x0, 0x7, 0x80,
    0x0, 0x8, 0x70, 0x0, 0x78, 0x58, 0x85, 0x87,
    0x0, 0x7, 0x80, 0x87, 0x18, 0x70, 0x0, 0x78,
    0x1, 0x0, 0x87, 0x0, 0x89, 0x97, 0x77, 0x79,
    0x95, 0x59, 0x77, 0x77, 0x77, 0x77, 0xa7, 0x22,
    0x10, 0x0, 0x57, 0x97, 0x70, 0x0, 0x0, 0x3,
    0x77, 0x72, 0xb7, 0x77, 0x77, 0x77, 0x7a, 0x6,
    0x77, 0x77, 0x77, 0x77, 0x30
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 192, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 55, .adv_w = 192, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 121, .adv_w = 192, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 187, .adv_w = 192, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 58904, .range_length = 4, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t iconfont_12 = {
#else
lv_font_t iconfont_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if ICONFONT_12*/
