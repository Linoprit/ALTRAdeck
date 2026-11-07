#ifndef ALTRAGRAPHIX_FXBUFSET_H
#define ALTRAGRAPHIX_FXBUFSET_H

#include <stddef.h>
#include <stdint.h>

#include "py/obj.h"

typedef struct _altra_fxbufset_obj_t {
    mp_obj_base_t base;
    mp_obj_t bufset_obj;
    mp_obj_t *fbufs;
    size_t width;
    size_t height;
    size_t count;
    uint8_t format;
} altra_fxbufset_obj_t;

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_fxbufset;

mp_obj_t altra_fxbufset_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args);
mp_obj_t altra_fxbufset_add_fxbuf(mp_obj_t self_in, mp_obj_t fbuf_in);
mp_obj_t altra_fxbufset_get_count(mp_obj_t self_in);
mp_obj_t altra_fxbufset_get_fxbuf(mp_obj_t self_in, mp_obj_t index_in);
mp_obj_t altra_fxbufset_load_fxbuf(mp_obj_t self_in, mp_obj_t filename_in,
    mp_obj_t index_in);

#if defined(__cplusplus)
}
#endif

#endif
