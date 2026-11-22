extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"

#include <string.h>

#include "fxbuf.h"
#include "fxbufset.h"
#include "sprite.h"

extern const mp_obj_type_t mp_type_point;

static mp_uint_t altra_sprite_next_id;

static altra_sprite_obj_t *altra_sprite_from_obj(mp_obj_t self_in) {
    return static_cast<altra_sprite_obj_t *>(MP_OBJ_TO_PTR(self_in));
}

static size_t altra_sprite_fxset_count(mp_obj_t fxset) {
    const auto *set = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(fxset));
    return set->count;
}

static mp_obj_t altra_sprite_current_fxbuf(altra_sprite_obj_t *self) {
    const auto *set = static_cast<const altra_fxbufset_obj_t *>(
        MP_OBJ_TO_PTR(self->fxset));
    if (self->fx_idx >= set->count) {
        mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("sprite fx_idx out of range"));
    }
    return set->fbufs[self->fx_idx];
}

static size_t altra_sprite_name_length(const char *data, size_t length) {
    size_t copied = 0;
    while (copied < length) {
        unsigned char first = (unsigned char)data[copied];
        size_t character_length = first < 0x80 ? 1
            : (first & 0xe0) == 0xc0 ? 2
            : (first & 0xf0) == 0xe0 ? 3
            : 4;
        if (copied + character_length > ALTRA_SPRITE_NAME_SIZE) {
            break;
        }
        copied += character_length;
    }
    return copied;
}

mp_obj_t altra_sprite_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    if (!mp_obj_is_type(args[0], &mp_type_fxbufset)) {
        mp_raise_TypeError(MP_ERROR_TEXT("sprite expects an fxbufset"));
    }

    altra_sprite_obj_t *self = mp_obj_malloc(altra_sprite_obj_t, type);
    char initial_name[ALTRA_SPRITE_NAME_SIZE] = {0};
    self->id = altra_sprite_next_id++;
    self->name = mp_obj_new_bytearray(sizeof(initial_name), initial_name);
    self->name_len = 0;
    self->pos = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_type_point),
        MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0));
    self->z_coord = 0;
    self->rotation = 0;
    self->scale = 1;
    self->collis_group = 0;
    self->auto_anim = false;
    self->has_changed = false;
    self->fx_idx = 0;
    self->fxset = args[0];
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t altra_sprite_get_id(mp_obj_t self_in) {
    return mp_obj_new_int(altra_sprite_from_obj(self_in)->id);
}

mp_obj_t altra_sprite_set_id(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_from_obj(self_in)->id = mp_obj_get_int(value_in);
    return mp_const_none;
}

mp_obj_t altra_sprite_get_name(mp_obj_t self_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    mp_buffer_info_t buffer;
    mp_get_buffer_raise(self->name, &buffer, MP_BUFFER_READ);
    return mp_obj_new_str((const char *)buffer.buf, self->name_len);
}

mp_obj_t altra_sprite_set_name(mp_obj_t self_in, mp_obj_t value_in) {
    if (!mp_obj_is_str(value_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("name must be a string"));
    }

    size_t input_len;
    const char *input = mp_obj_str_get_data(value_in, &input_len);
    size_t copied = altra_sprite_name_length(input, input_len);
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    mp_buffer_info_t buffer;
    mp_get_buffer_raise(self->name, &buffer, MP_BUFFER_WRITE);
    memset(buffer.buf, 0, buffer.len);
    memcpy(buffer.buf, input, copied);
    self->name_len = copied;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_pos(mp_obj_t self_in) {
    return altra_sprite_from_obj(self_in)->pos;
}

mp_obj_t altra_sprite_set_pos(mp_obj_t self_in, mp_obj_t value_in) {
    if (!mp_obj_is_type(value_in, &mp_type_point)) {
        mp_raise_TypeError(MP_ERROR_TEXT("pos must be a point"));
    }
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->pos = value_in;
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_move(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    mp_int_t dx = mp_obj_get_int(x_in);
    mp_int_t dy = mp_obj_get_int(y_in);
    mp_obj_t pos = altra_sprite_get_pos(self_in);
    mp_obj_t x = mp_binary_op(MP_BINARY_OP_ADD, mp_load_attr(pos, MP_QSTR_x),
        mp_obj_new_int(dx));
    mp_obj_t y = mp_binary_op(MP_BINARY_OP_ADD, mp_load_attr(pos, MP_QSTR_y),
        mp_obj_new_int(dy));
    mp_obj_t new_pos = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_type_point), x, y);
    return altra_sprite_set_pos(self_in, new_pos);
}

mp_obj_t altra_sprite_get_z_coord(mp_obj_t self_in) {
    return mp_obj_new_int(altra_sprite_from_obj(self_in)->z_coord);
}

mp_obj_t altra_sprite_set_z_coord(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->z_coord = mp_obj_get_int(value_in);
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_rotation(mp_obj_t self_in) {
    return mp_obj_new_float(altra_sprite_from_obj(self_in)->rotation);
}

mp_obj_t altra_sprite_set_rotation(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->rotation = mp_obj_get_float(value_in);
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_scale(mp_obj_t self_in) {
    return mp_obj_new_float(altra_sprite_from_obj(self_in)->scale);
}

mp_obj_t altra_sprite_set_scale(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->scale = mp_obj_get_float(value_in);
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_collis_box(mp_obj_t self_in) {
    return altra_fxbuf_get_collis_box(
        altra_sprite_current_fxbuf(altra_sprite_from_obj(self_in)));
}

mp_obj_t altra_sprite_set_collis_box(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    altra_fxbuf_set_collis_box(altra_sprite_current_fxbuf(self), value_in);
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_collis_group(mp_obj_t self_in) {
    return mp_obj_new_int(altra_sprite_from_obj(self_in)->collis_group);
}

mp_obj_t altra_sprite_set_collis_group(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_from_obj(self_in)->collis_group = mp_obj_get_int(value_in);
    return mp_const_none;
}

mp_obj_t altra_sprite_get_auto_anim(mp_obj_t self_in) {
    return mp_obj_new_bool(altra_sprite_from_obj(self_in)->auto_anim);
}

mp_obj_t altra_sprite_set_auto_anim(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_from_obj(self_in)->auto_anim = mp_obj_is_true(value_in);
    return mp_const_none;
}

mp_obj_t altra_sprite_get_has_changed(mp_obj_t self_in) {
    return mp_obj_new_bool(altra_sprite_from_obj(self_in)->has_changed);
}

mp_obj_t altra_sprite_set_has_changed(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_from_obj(self_in)->has_changed = mp_obj_is_true(value_in);
    return mp_const_none;
}

mp_obj_t altra_sprite_get_fx_idx(mp_obj_t self_in) {
    return mp_obj_new_int_from_uint(altra_sprite_from_obj(self_in)->fx_idx);
}

mp_obj_t altra_sprite_set_fx_idx(mp_obj_t self_in, mp_obj_t value_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    mp_int_t index = mp_obj_get_int(value_in);
    size_t count = altra_sprite_fxset_count(self->fxset);
    if (index < 0 || (mp_uint_t)index >= count) {
        mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("fx_idx out of range"));
    }
    self->fx_idx = index;
    self->has_changed = true;
    return mp_const_none;
}

mp_obj_t altra_sprite_get_fxset(mp_obj_t self_in) {
    return altra_sprite_from_obj(self_in)->fxset;
}

mp_obj_t altra_sprite_set_fxset(mp_obj_t self_in, mp_obj_t value_in) {
    if (!mp_obj_is_type(value_in, &mp_type_fxbufset)) {
        mp_raise_TypeError(MP_ERROR_TEXT("fxset must be an fxbufset"));
    }
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->fxset = value_in;
    if (self->fx_idx >= altra_sprite_fxset_count(value_in)) {
        self->fx_idx = 0;
    }
    return mp_const_none;
}

mp_obj_t altra_sprite_inc_fx_idx(mp_obj_t self_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    size_t count = altra_sprite_fxset_count(self->fxset);
    if (count == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot advance an empty fxset"));
    }
    self->fx_idx = (self->fx_idx + 1) % count;
    self->has_changed = true;
    return mp_obj_new_int_from_uint(self->fx_idx);
}

mp_obj_t altra_sprite_dec_fx_idx(mp_obj_t self_in) {
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    size_t count = altra_sprite_fxset_count(self->fxset);
    if (count == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot reverse an empty fxset"));
    }
    self->fx_idx = (self->fx_idx + count - 1) % count;
    self->has_changed = true;
    return mp_obj_new_int_from_uint(self->fx_idx);
}
}
