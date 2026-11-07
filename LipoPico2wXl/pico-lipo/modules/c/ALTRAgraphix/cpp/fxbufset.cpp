extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"

#include "fxbufset.h"
#include "fxbuf.h"

mp_obj_t altra_fxbufset_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    altra_fxbufset_obj_t *self = mp_obj_malloc(altra_fxbufset_obj_t, type);
    self->bufset_obj = mp_obj_new_list(0, NULL);
    self->fbufs = NULL;
    self->width = 0;
    self->height = 0;
    self->count = 0;
    self->format = 0;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t altra_fxbufset_add_fxbuf(mp_obj_t self_in, mp_obj_t fbuf_in) {
    auto *self = static_cast<altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    size_t width;
    size_t height;
    size_t stride;
    if (!mp_framebuf_get_rgb565_info(altra_fxbuf_get_native(fbuf_in),
        &width, &height, &stride)) {
        return mp_const_false;
    }
    const uint8_t format = 1;

    if (self->count != 0
        && (width != self->width || height != self->height || format != self->format)) {
        return mp_const_false;
    }

    mp_obj_list_append(self->bufset_obj, fbuf_in);
    size_t count;
    mp_obj_list_get(self->bufset_obj, &count, &self->fbufs);
    self->count = count;
    if (count == 1) {
        self->width = width;
        self->height = height;
        self->format = format;
    }
    return mp_const_true;
}

mp_obj_t altra_fxbufset_get_count(mp_obj_t self_in) {
    const auto *self = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    return mp_obj_new_int_from_uint(self->count);
}

mp_obj_t altra_fxbufset_get_fxbuf(mp_obj_t self_in, mp_obj_t index_in) {
    const auto *self = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    size_t index = mp_get_index(self->base.type, self->count, index_in, false);
    return self->fbufs[index];
}

mp_obj_t altra_fxbufset_load_fxbuf(mp_obj_t self_in, mp_obj_t filename_in,
    mp_obj_t index_in) {
    (void)self_in;
    (void)filename_in;
    (void)index_in;
    mp_raise_NotImplementedError(MP_ERROR_TEXT("load_fxbuf is not implemented"));
}
}
