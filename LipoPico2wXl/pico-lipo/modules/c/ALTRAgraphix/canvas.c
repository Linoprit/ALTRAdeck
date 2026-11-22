#include "py/runtime.h"

#include "canvas.h"

static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_width_obj, altra_canvas_get_width);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_height_obj, altra_canvas_get_height);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_bkg_col_obj, altra_canvas_get_bkg_col);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_content_obj, altra_canvas_get_content);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_count_obj, altra_canvas_get_count);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_get_fxbuf_obj, altra_canvas_get_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_transfer_done_obj,
    altra_canvas_transfer_done);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_is_transfered_obj,
    altra_canvas_is_transfered);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_is_new_frame_obj,
    altra_canvas_is_new_frame);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_await_new_frame_obj,
    altra_canvas_await_new_frame);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_render_obj, altra_canvas_render);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_await_transfer_obj,
    altra_canvas_await_transfer);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_do_aabb_detection_obj,
    altra_canvas_do_aabb_detection);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_do_sta_detection_obj,
    altra_canvas_do_sta_detection);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_do_ent_detection_obj,
    altra_canvas_do_ent_detection);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_do_pixelwise_detection_obj,
    altra_canvas_do_pixelwise_detection);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_add_sprite_obj, altra_canvas_add_sprite);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_canvas_reorder_objects_obj,
    altra_canvas_reorder_objects);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_canvas_cycle_obj, altra_canvas_cycle);

static const mp_rom_map_elem_t altra_canvas_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_width), MP_ROM_PTR(&altra_canvas_get_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_height), MP_ROM_PTR(&altra_canvas_get_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_bkg_col), MP_ROM_PTR(&altra_canvas_get_bkg_col_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_content), MP_ROM_PTR(&altra_canvas_get_content_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_count), MP_ROM_PTR(&altra_canvas_get_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fxbuf), MP_ROM_PTR(&altra_canvas_get_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_transfer_done), MP_ROM_PTR(&altra_canvas_transfer_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_transfered), MP_ROM_PTR(&altra_canvas_is_transfered_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_new_frame), MP_ROM_PTR(&altra_canvas_is_new_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_await_new_frame), MP_ROM_PTR(&altra_canvas_await_new_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&altra_canvas_render_obj) },
    { MP_ROM_QSTR(MP_QSTR_await_transfer), MP_ROM_PTR(&altra_canvas_await_transfer_obj) },
    { MP_ROM_QSTR(MP_QSTR_do_aabb_detection),
        MP_ROM_PTR(&altra_canvas_do_aabb_detection_obj) },
    { MP_ROM_QSTR(MP_QSTR_do_sta_detection),
        MP_ROM_PTR(&altra_canvas_do_sta_detection_obj) },
    { MP_ROM_QSTR(MP_QSTR_do_ent_detection),
        MP_ROM_PTR(&altra_canvas_do_ent_detection_obj) },
    { MP_ROM_QSTR(MP_QSTR_do_pixelwise_detection),
        MP_ROM_PTR(&altra_canvas_do_pixelwise_detection_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_sprite), MP_ROM_PTR(&altra_canvas_add_sprite_obj) },
    { MP_ROM_QSTR(MP_QSTR_reorder_objects),
        MP_ROM_PTR(&altra_canvas_reorder_objects_obj) },
    { MP_ROM_QSTR(MP_QSTR_cycle), MP_ROM_PTR(&altra_canvas_cycle_obj) },
};
static MP_DEFINE_CONST_DICT(altra_canvas_locals, altra_canvas_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_canvas,
    MP_QSTR_canvas,
    MP_TYPE_FLAG_NONE,
    make_new, altra_canvas_make_new,
    attr, altra_canvas_attr,
    locals_dict, &altra_canvas_locals
    );
