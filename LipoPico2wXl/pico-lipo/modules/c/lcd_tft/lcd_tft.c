#include "lcd_tft.h"

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lcd_tft_color565_obj, 3, 3, lcd_tft_color565);

static const mp_map_elem_t lcd_tft_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lcd_tft) },
    { MP_ROM_QSTR(MP_QSTR_color565), MP_ROM_PTR(&lcd_tft_color565_obj) },
    { MP_ROM_QSTR(MP_QSTR_Display), MP_ROM_PTR(&lcd_tft_display_type) },
};
static MP_DEFINE_CONST_DICT(mp_module_lcd_tft_globals, lcd_tft_globals_table);

const mp_obj_module_t lcd_tft_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lcd_tft_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd_tft, lcd_tft_user_cmodule);
