#ifndef ALTRAGRAPHIX_CANVAS_H
#define ALTRAGRAPHIX_CANVAS_H

#include <stdbool.h>

#include "py/obj.h"
#include "py/mpthread.h"

typedef struct _altra_canvas_obj_t {
    mp_obj_base_t base;
    mp_int_t width;
    mp_int_t height;
    mp_int_t bkg_col;
    mp_obj_t content_obj;
    mp_obj_t *content;
    size_t count;
    mp_obj_t object_fxbuf;
    mp_obj_t display_fxbuf;
    bool transfer_complete;
    bool new_frame_available;
    bool enable_aabb;
    bool enable_sta;
    bool enable_ent;
    bool enable_pixelwise;
#if MICROPY_PY_THREAD
    mp_thread_mutex_t state_mutex;
#endif
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
mp_obj_t altra_canvas_transfer_done(mp_obj_t self_in);
mp_obj_t altra_canvas_is_transfered(mp_obj_t self_in);
mp_obj_t altra_canvas_is_new_frame(mp_obj_t self_in);
mp_obj_t altra_canvas_await_new_frame(mp_obj_t self_in, mp_obj_t timeout_in);
mp_obj_t altra_canvas_render(mp_obj_t self_in);
mp_obj_t altra_canvas_await_transfer(mp_obj_t self_in, mp_obj_t timeout_in);
mp_obj_t altra_canvas_do_aabb_detection(mp_obj_t self_in);
mp_obj_t altra_canvas_do_sta_detection(mp_obj_t self_in, mp_obj_t pairs_in);
mp_obj_t altra_canvas_do_ent_detection(mp_obj_t self_in, mp_obj_t pairs_in);
mp_obj_t altra_canvas_do_pixelwise_detection(mp_obj_t self_in, mp_obj_t pairs_in);
mp_obj_t altra_canvas_add_sprite(mp_obj_t self_in, mp_obj_t sprite_in);
mp_obj_t altra_canvas_reorder_objects(mp_obj_t self_in);
mp_obj_t altra_canvas_cycle(mp_obj_t self_in, mp_obj_t timeout_in);
void altra_canvas_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#if defined(__cplusplus)
}
#endif

#endif
