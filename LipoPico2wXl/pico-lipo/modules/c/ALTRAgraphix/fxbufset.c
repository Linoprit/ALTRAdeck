#include "py/runtime.h"

#include "fxbufset.h"

static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbufset_add_fxbuf_obj,
    altra_fxbufset_add_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbufset_get_count_obj,
    altra_fxbufset_get_count);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_fxbufset_get_fxbuf_obj,
    altra_fxbufset_get_fxbuf);
static MP_DEFINE_CONST_FUN_OBJ_3(altra_fxbufset_load_fxbuf_obj,
    altra_fxbufset_load_fxbuf);

static const mp_rom_map_elem_t altra_fxbufset_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add_fxbuf), MP_ROM_PTR(&altra_fxbufset_add_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_count), MP_ROM_PTR(&altra_fxbufset_get_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fxbuf), MP_ROM_PTR(&altra_fxbufset_get_fxbuf_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_fxbuf), MP_ROM_PTR(&altra_fxbufset_load_fxbuf_obj) },
};
static MP_DEFINE_CONST_DICT(altra_fxbufset_locals, altra_fxbufset_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_fxbufset,
    MP_QSTR_fxbufset,
    MP_TYPE_FLAG_NONE,
    make_new, altra_fxbufset_make_new,
    locals_dict, &altra_fxbufset_locals
    );
