#include "py/runtime.h"
#include "modframebuf.h"


static mp_obj_t altra_fxbuf_framebuffer1(size_t n_args, const mp_obj_t *args) {
    mp_obj_t framebuf_args[] = {
        args[0],
        args[1],
        args[2],
        MP_OBJ_NEW_SMALL_INT(0),
        n_args == 4 ? args[3] : args[1],
    };
    return mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_framebuf), 5, 0, framebuf_args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(altra_fxbuf_framebuffer1_obj, 3, 4, altra_fxbuf_framebuffer1);

static mp_obj_t altra_fxbuf_get_rgb565_info(mp_obj_t framebuffer_obj) {
    size_t width;
    size_t height;
    size_t stride;
    if (!mp_framebuf_get_rgb565_info(framebuffer_obj, &width, &height, &stride)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected an RGB565 FrameBuffer"));
    }
    mp_obj_t info[] = {
        mp_obj_new_int_from_uint(width),
        mp_obj_new_int_from_uint(height),
        mp_obj_new_int_from_uint(stride),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(info), info);
}
static MP_DEFINE_CONST_FUN_OBJ_1(altra_fxbuf_get_rgb565_info_obj, altra_fxbuf_get_rgb565_info);

static const mp_rom_map_elem_t altra_fxbuf_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_altra_dot_fxbuf) },
    { MP_ROM_QSTR(MP_QSTR_fxbuff), MP_ROM_PTR(&mp_type_framebuf) },
    { MP_ROM_QSTR(MP_QSTR_FrameBuffer), MP_ROM_PTR(&mp_type_framebuf) },
    { MP_ROM_QSTR(MP_QSTR_FrameBuffer1), MP_ROM_PTR(&altra_fxbuf_framebuffer1_obj) },
    { MP_ROM_QSTR(MP_QSTR_MVLSB), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_MONO_VLSB), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_RGB565), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_GS2_HMSB), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_GS4_HMSB), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_GS8), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_MONO_HLSB), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_MONO_HMSB), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_get_rgb565_info), MP_ROM_PTR(&altra_fxbuf_get_rgb565_info_obj) },
};
static MP_DEFINE_CONST_DICT(altra_fxbuf_globals, altra_fxbuf_globals_table);

const mp_obj_module_t altra_fxbuf_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&altra_fxbuf_globals,
};
