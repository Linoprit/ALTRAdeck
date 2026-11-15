#include "py/runtime.h"

#include "fxbuf.h"

static mp_obj_t altra_fxbuf_save_fun(size_t n_args, const mp_obj_t *args) {
    return altra_fxbuf_save_with_endian(args[0], args[1],
        n_args == 3 && mp_obj_is_true(args[2]));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbuf_save_obj, 2, 3,
    altra_fxbuf_save_fun);

static mp_obj_t altra_fxbuf_load_fun(size_t n_args, const mp_obj_t *args) {
    return altra_fxbuf_load_from_file_with_endian(args[0],
        n_args == 2 && mp_obj_is_true(args[1]));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbuf_load_obj, 1, 2,
    altra_fxbuf_load_fun);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbuf_get_trans_col_obj,
    altra_fxbuf_get_trans_col);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbuf_set_trans_col_obj,
    altra_fxbuf_set_trans_col);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbuf_get_collis_box_obj,
    altra_fxbuf_get_collis_box);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbuf_set_collis_box_obj,
    altra_fxbuf_set_collis_box);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbuf_get_raw_obj, altra_fxbuf_get_raw);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbuf_to_rgb565_obj,
    altra_fxbuf_to_rgb565);
static MP_DEFINE_CONST_FUN_OBJ_3(altra_fxbuf_to_color_obj,
    altra_fxbuf_to_color);

static mp_obj_t altra_fxbuf_fill_method(mp_obj_t self_in, mp_obj_t color_in) {
    altra_fxbuf_fill(self_in, mp_obj_get_int(color_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbuf_fill_obj, altra_fxbuf_fill_method);

static mp_obj_t altra_fxbuf_blit_method(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    altra_fxbuf_blit(args[0], args[1], mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbuf_blit_obj, 4, 4,
    altra_fxbuf_blit_method);

static const mp_rom_map_elem_t altra_fxbuf_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&altra_fxbuf_save_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_trans_col), MP_ROM_PTR(&altra_fxbuf_get_trans_col_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_trans_col), MP_ROM_PTR(&altra_fxbuf_set_trans_col_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_collis_box), MP_ROM_PTR(&altra_fxbuf_get_collis_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_collis_box), MP_ROM_PTR(&altra_fxbuf_set_collis_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_raw), MP_ROM_PTR(&altra_fxbuf_get_raw_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&altra_fxbuf_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&altra_fxbuf_blit_obj) },
};
static MP_DEFINE_CONST_DICT(altra_fxbuf_locals, altra_fxbuf_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_fxbuf,
    MP_QSTR_fxbuf,
    MP_TYPE_FLAG_NONE,
    make_new, altra_fxbuf_make_new,
    attr, altra_fxbuf_attr,
    buffer, altra_fxbuf_get_buffer,
    locals_dict, &altra_fxbuf_locals
    );

static const mp_rom_map_elem_t altra_fxbuf_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_altra_dot_fxbuf) },
    { MP_ROM_QSTR(MP_QSTR_fxbuf), MP_ROM_PTR(&mp_type_fxbuf) },
    { MP_ROM_QSTR(MP_QSTR_fxbuff), MP_ROM_PTR(&mp_type_fxbuf) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&altra_fxbuf_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_to_rgb565), MP_ROM_PTR(&altra_fxbuf_to_rgb565_obj) },
    { MP_ROM_QSTR(MP_QSTR_to_color), MP_ROM_PTR(&altra_fxbuf_to_color_obj) },
};
static MP_DEFINE_CONST_DICT(altra_fxbuf_globals, altra_fxbuf_globals_table);

const mp_obj_module_t altra_fxbuf_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&altra_fxbuf_globals,
};
