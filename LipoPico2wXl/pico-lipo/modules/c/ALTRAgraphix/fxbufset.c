#include "py/runtime.h"

#include "fxbufset.h"

static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbufset_add_fxbuf_obj,
    altra_fxbufset_add_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_3(altra_fxbufset_set_fxbuf_obj,
    altra_fxbufset_set_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbufset_get_count_obj,
    altra_fxbufset_get_count);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbufset_get_fxbuf_obj,
    altra_fxbufset_get_fxbuf);
static mp_obj_t altra_fxbufset_load_fxbuf_fun(size_t n_args, const mp_obj_t *args) {
    return altra_fxbufset_load_fxbuf(args[0], args[1], args[2],
        n_args == 4 && mp_obj_is_true(args[3]));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbufset_load_fxbuf_obj, 3, 4,
    altra_fxbufset_load_fxbuf_fun);

static mp_obj_t altra_fxbufset_save_fxbuf_fun(size_t n_args, const mp_obj_t *args) {
    return altra_fxbufset_save_fxbuf(args[0], args[1], args[2],
        n_args == 4 && mp_obj_is_true(args[3]));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbufset_save_fxbuf_obj, 3, 4,
    altra_fxbufset_save_fxbuf_fun);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbufset_load_fxbufset_fun_obj,
    2, MP_OBJ_FUN_ARGS_MAX, altra_fxbufset_load_fxbufset);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(altra_fxbufset_load_fxbufset_obj,
    MP_ROM_PTR(&altra_fxbufset_load_fxbufset_fun_obj));
static mp_obj_t altra_fxbufset_save_fxbufset_fun(size_t n_args,
    const mp_obj_t *args) {
    return altra_fxbufset_save_fxbufset(args[0], args[1],
        n_args == 3 && mp_obj_is_true(args[2]));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbufset_save_fxbufset_obj, 2, 3,
    altra_fxbufset_save_fxbufset_fun);

static const mp_rom_map_elem_t altra_fxbufset_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add_fxbuf), MP_ROM_PTR(&altra_fxbufset_add_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_fxbuf), MP_ROM_PTR(&altra_fxbufset_set_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_count), MP_ROM_PTR(&altra_fxbufset_get_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fxbuf), MP_ROM_PTR(&altra_fxbufset_get_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_fxbuf), MP_ROM_PTR(&altra_fxbufset_load_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_fxbuf), MP_ROM_PTR(&altra_fxbufset_save_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_fxbufset), MP_ROM_PTR(&altra_fxbufset_load_fxbufset_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_fxbufset), MP_ROM_PTR(&altra_fxbufset_save_fxbufset_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_fxbuset), MP_ROM_PTR(&altra_fxbufset_save_fxbufset_obj) },
};
static MP_DEFINE_CONST_DICT(altra_fxbufset_locals, altra_fxbufset_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_fxbufset,
    MP_QSTR_fxbufset,
    MP_TYPE_FLAG_NONE,
    make_new, altra_fxbufset_make_new,
    locals_dict, &altra_fxbufset_locals
    );
