#ifndef ALTRAGRAPHIX_CANVAS_H
#define ALTRAGRAPHIX_CANVAS_H

#include "py/obj.h"

typedef struct _altra_canvas_obj_t {
    mp_obj_base_t base;
    mp_int_t width;
    mp_int_t height;
    mp_int_t bkg_col;
    mp_obj_t content_obj;
    mp_obj_t *content;
    size_t count;
    mp_obj_t fxbuf;
} altra_canvas_obj_t;

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_canvas;

mp_obj_t altra_canvas_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args);
mp_obj_t altra_canvas_get_width(mp_obj_t self_in);
mp_obj_t altra_canvas_get_height(mp_obj_t self_in);
mp_obj_t altra_canvas_get_bkg_col(mp_obj_t self_in);
mp_obj_t altra_canvas_get_content(mp_obj_t self_in);
mp_obj_t altra_canvas_get_count(mp_obj_t self_in);
mp_obj_t altra_canvas_get_fxbuf(mp_obj_t self_in);
mp_obj_t altra_canvas_add_sprite(mp_obj_t self_in, mp_obj_t sprite_in);
mp_obj_t altra_canvas_cycle(mp_obj_t self_in);

#if defined(__cplusplus)
}
#endif

#endif
