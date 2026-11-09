extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"

#include "fxbufset.h"
#include "sprite.h"

extern const mp_obj_type_t mp_type_point;
extern const mp_obj_type_t mp_type_poly;

static mp_uint_t altra_sprite_next_id;

static altra_sprite_obj_t *altra_sprite_from_obj(mp_obj_t self_in) {
    return static_cast<altra_sprite_obj_t *>(MP_OBJ_TO_PTR(self_in));
}

static size_t altra_sprite_fxset_count(mp_obj_t fxset) {
    const auto *set = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(fxset));
    return set->count;
}

mp_obj_t altra_sprite_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    if (!mp_obj_is_type(args[0], &mp_type_fxbufset)) {
        mp_raise_TypeError(MP_ERROR_TEXT("sprite expects an fxbufset"));
    }

    altra_sprite_obj_t *self = mp_obj_malloc(altra_sprite_obj_t, type);
    self->id = altra_sprite_next_id++;
    self->pos = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_type_point),
        MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0));
    self->z_coord = 0;
    self->rotation = 0;
    self->scale = 1;
    self->collis_box = mp_call_function_1(MP_OBJ_FROM_PTR(&mp_type_poly),
        mp_obj_new_list(0, NULL));
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
    return altra_sprite_from_obj(self_in)->collis_box;
}

mp_obj_t altra_sprite_set_collis_box(mp_obj_t self_in, mp_obj_t value_in) {
    if (!mp_obj_is_type(value_in, &mp_type_poly)) {
        mp_raise_TypeError(MP_ERROR_TEXT("collis_box must be a poly"));
    }
    altra_sprite_obj_t *self = altra_sprite_from_obj(self_in);
    self->collis_box = value_in;
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
