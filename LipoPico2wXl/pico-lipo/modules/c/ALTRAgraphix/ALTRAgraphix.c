#include "py/runtime.h"

extern const mp_obj_module_t altra_fxbuf_module;
extern const mp_obj_module_t altra_types_module;
extern const mp_obj_type_t mp_type_fxbufset;
extern const mp_obj_type_t mp_type_sprite;
extern const mp_obj_type_t mp_type_canvas;

static const mp_rom_map_elem_t altra_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_altra) },
    { MP_ROM_QSTR(MP_QSTR_fxbuf), MP_ROM_PTR(&altra_fxbuf_module) },
    { MP_ROM_QSTR(MP_QSTR_fxbufset), MP_ROM_PTR(&mp_type_fxbufset) },
    { MP_ROM_QSTR(MP_QSTR_types), MP_ROM_PTR(&altra_types_module) },
    { MP_ROM_QSTR(MP_QSTR_sprite), MP_ROM_PTR(&mp_type_sprite) },
    { MP_ROM_QSTR(MP_QSTR_canvas), MP_ROM_PTR(&mp_type_canvas) },
};
static MP_DEFINE_CONST_DICT(altra_globals, altra_globals_table);

const mp_obj_module_t altra_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&altra_globals,
};

MP_REGISTER_MODULE(MP_QSTR_altra, altra_user_cmodule);