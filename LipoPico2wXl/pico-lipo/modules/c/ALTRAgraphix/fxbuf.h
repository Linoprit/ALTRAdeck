#ifndef ALTRAGRAPHIX_FXBUF_H
#define ALTRAGRAPHIX_FXBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "py/obj.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_fxbuf;

typedef struct _altra_fxbuf_obj_t {
    mp_obj_base_t base;
    mp_obj_t buffer;
    size_t width;
    size_t height;
    int32_t trans_col;
    mp_obj_t collis_box;
} altra_fxbuf_obj_t;

mp_obj_t altra_fxbuf_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args);
void altra_fxbuf_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);
mp_int_t altra_fxbuf_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo,
    mp_uint_t flags);
mp_obj_t altra_fxbuf_get_raw(mp_obj_t self_in);
mp_obj_t altra_fxbuf_save(mp_obj_t self_in, mp_obj_t filename_in);
mp_obj_t altra_fxbuf_save_with_endian(mp_obj_t self_in, mp_obj_t filename_in,
    bool swap_endian);
mp_obj_t altra_fxbuf_load_from_file(mp_obj_t filename_in);
mp_obj_t altra_fxbuf_load_from_file_with_endian(mp_obj_t filename_in,
    bool swap_endian);
mp_obj_t altra_fxbuf_get_trans_col(mp_obj_t self_in);
mp_obj_t altra_fxbuf_set_trans_col(mp_obj_t self_in, mp_obj_t value_in);
mp_obj_t altra_fxbuf_get_collis_box(mp_obj_t self_in);
mp_obj_t altra_fxbuf_set_collis_box(mp_obj_t self_in, mp_obj_t value_in);
mp_obj_t altra_fxbuf_to_rgb565(mp_obj_t color_in);
mp_obj_t altra_fxbuf_to_color(mp_obj_t red_in, mp_obj_t green_in, mp_obj_t blue_in);
void altra_fxbuf_fill(mp_obj_t self_in, mp_int_t color);
void altra_fxbuf_blit(mp_obj_t self_in, mp_obj_t source_in, mp_int_t x, mp_int_t y);
bool altra_fxbuf_get_info(mp_obj_t obj, size_t *width, size_t *height);

#if defined(__cplusplus)
}
#endif

#endif
