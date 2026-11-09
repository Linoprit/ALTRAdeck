extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"

#include <stdint.h>

#include "canvas.h"
#include "fxbuf.h"
#include "fxbufset.h"
#include "sprite.h"

extern const mp_obj_type_t mp_type_point;
extern const mp_obj_type_t mp_type_sprite;

static altra_canvas_obj_t *altra_canvas_from_obj(mp_obj_t self_in) {
    return static_cast<altra_canvas_obj_t *>(MP_OBJ_TO_PTR(self_in));
}

static void altra_canvas_sync(altra_canvas_obj_t *self) {
    mp_obj_list_get(self->content_obj, &self->count, &self->content);
}

mp_obj_t altra_canvas_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 4, 4, false);

    mp_int_t width = mp_obj_get_int(args[0]);
    mp_int_t height = mp_obj_get_int(args[1]);
    mp_int_t bkg_col = mp_obj_get_int(args[3]);
    if (width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("canvas dimensions must be in range 1..65535"));
    }

    mp_obj_t framebuffer_args[] = {
        args[0],
        args[1],
        args[2],
    };
    mp_obj_t fxbuf = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_fxbuf),
        MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);

    altra_canvas_obj_t *self = mp_obj_malloc(altra_canvas_obj_t, type);
    self->width = width;
    self->height = height;
    self->bkg_col = bkg_col;
    self->content_obj = mp_obj_new_list(0, NULL);
    self->content = NULL;
    self->count = 0;
    self->fxbuf = fxbuf;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t altra_canvas_get_width(mp_obj_t self_in) {
    return mp_obj_new_int(altra_canvas_from_obj(self_in)->width);
}

mp_obj_t altra_canvas_get_height(mp_obj_t self_in) {
    return mp_obj_new_int(altra_canvas_from_obj(self_in)->height);
}

mp_obj_t altra_canvas_get_bkg_col(mp_obj_t self_in) {
    return mp_obj_new_int(altra_canvas_from_obj(self_in)->bkg_col);
}

mp_obj_t altra_canvas_get_content(mp_obj_t self_in) {
    return altra_canvas_from_obj(self_in)->content_obj;
}

mp_obj_t altra_canvas_get_count(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_sync(self);
    return mp_obj_new_int_from_uint(self->count);
}

mp_obj_t altra_canvas_get_fxbuf(mp_obj_t self_in) {
    return altra_canvas_from_obj(self_in)->fxbuf;
}

mp_obj_t altra_canvas_add_sprite(mp_obj_t self_in, mp_obj_t sprite_in) {
    if (!mp_obj_is_type(sprite_in, &mp_type_sprite)) {
        mp_raise_TypeError(MP_ERROR_TEXT("add_sprite expects a sprite"));
    }

    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    const auto *sprite = static_cast<const altra_sprite_obj_t *>(MP_OBJ_TO_PTR(sprite_in));
    mp_obj_t insert_method[4];
    mp_load_method(self->content_obj, MP_QSTR_insert, insert_method);

    size_t index = 0;
    while (index < self->count) {
        const auto *existing = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(self->content[index]));
        if (existing->z_coord >= sprite->z_coord) {
            break;
        }
        ++index;
    }
    insert_method[2] = mp_obj_new_int_from_uint(index);
    insert_method[3] = sprite_in;
    mp_call_method_n_kw(2, 0, insert_method);
    altra_canvas_sync(self);
    return mp_const_none;
}

mp_obj_t altra_canvas_cycle(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_sync(self);

    mp_obj_t method[6];
    mp_load_method(self->fxbuf, MP_QSTR_fill, method);
    method[2] = mp_obj_new_int(self->bkg_col);
    mp_call_method_n_kw(1, 0, method);

    mp_obj_t destination = altra_fxbuf_get_native(self->fxbuf);
    for (size_t i = 0; i < self->count; ++i) {
        const auto *sprite = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(self->content[i]));
        const auto *fxset = static_cast<const altra_fxbufset_obj_t *>(
            MP_OBJ_TO_PTR(sprite->fxset));
        if (sprite->fx_idx >= fxset->count) {
            mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("sprite fx_idx out of range"));
        }

        mp_obj_t source = altra_fxbuf_get_native(fxset->fbufs[sprite->fx_idx]);
        mp_int_t x = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_x));
        mp_int_t y = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_y));
        mp_load_method(destination, MP_QSTR_blit, method);
        method[2] = source;
        method[3] = mp_obj_new_int(x);
        method[4] = mp_obj_new_int(y);
        method[5] = mp_obj_new_int_from_uint(
            altra_fxbuf_get_trans_key(fxset->fbufs[sprite->fx_idx]));
        mp_call_method_n_kw(4, 0, method);
    }

    return self->fxbuf;
}
}
