#include "py/runtime.h"

#include "canvas.h"

static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_width_obj, altra_canvas_get_width);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_height_obj, altra_canvas_get_height);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_bkg_col_obj, altra_canvas_get_bkg_col);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_content_obj, altra_canvas_get_content);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_count_obj, altra_canvas_get_count);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_fxbuf_obj, altra_canvas_get_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_add_sprite_obj, altra_canvas_add_sprite);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_cycle_obj, altra_canvas_cycle);

static const mp_rom_map_elem_t altra_canvas_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_width), MP_ROM_PTR(&altra_canvas_get_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_height), MP_ROM_PTR(&altra_canvas_get_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_bkg_col), MP_ROM_PTR(&altra_canvas_get_bkg_col_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_content), MP_ROM_PTR(&altra_canvas_get_content_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_count), MP_ROM_PTR(&altra_canvas_get_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fxbuf), MP_ROM_PTR(&altra_canvas_get_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_sprite), MP_ROM_PTR(&altra_canvas_add_sprite_obj) },
    { MP_ROM_QSTR(MP_QSTR_cycle), MP_ROM_PTR(&altra_canvas_cycle_obj) },
};
static MP_DEFINE_CONST_DICT(altra_canvas_locals, altra_canvas_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_canvas,
    MP_QSTR_canvas,
    MP_TYPE_FLAG_NONE,
    make_new, altra_canvas_make_new,
    locals_dict, &altra_canvas_locals
    );
