#ifndef ALTRAGRAPHIX_SPRITE_H
#define ALTRAGRAPHIX_SPRITE_H

#include "py/obj.h"

typedef struct _altra_sprite_obj_t {
    mp_obj_base_t base;
    mp_int_t id;
    mp_obj_t pos;
    mp_int_t z_coord;
    mp_float_t rotation;
    mp_float_t scale;
    mp_obj_t collis_box;
    mp_int_t collis_group;
    bool auto_anim;
    bool has_changed;
    size_t fx_idx;
    mp_obj_t fxset;
} altra_sprite_obj_t;

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_sprite;

mp_obj_t altra_sprite_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args);

#define ALTRA_SPRITE_METHOD(name) mp_obj_t altra_sprite_##name(mp_obj_t self_in)
#define ALTRA_SPRITE_SETTER(name) mp_obj_t altra_sprite_set_##name(mp_obj_t self_in, mp_obj_t value_in)

ALTRA_SPRITE_METHOD(get_id);
ALTRA_SPRITE_SETTER(id);
ALTRA_SPRITE_METHOD(get_pos);
ALTRA_SPRITE_SETTER(pos);
ALTRA_SPRITE_METHOD(get_z_coord);
ALTRA_SPRITE_SETTER(z_coord);
ALTRA_SPRITE_METHOD(get_rotation);
ALTRA_SPRITE_SETTER(rotation);
ALTRA_SPRITE_METHOD(get_scale);
ALTRA_SPRITE_SETTER(scale);
ALTRA_SPRITE_METHOD(get_collis_box);
ALTRA_SPRITE_SETTER(collis_box);
ALTRA_SPRITE_METHOD(get_collis_group);
ALTRA_SPRITE_SETTER(collis_group);
ALTRA_SPRITE_METHOD(get_auto_anim);
ALTRA_SPRITE_SETTER(auto_anim);
ALTRA_SPRITE_METHOD(get_has_changed);
ALTRA_SPRITE_SETTER(has_changed);
ALTRA_SPRITE_METHOD(get_fx_idx);
ALTRA_SPRITE_SETTER(fx_idx);
ALTRA_SPRITE_METHOD(get_fxset);
ALTRA_SPRITE_SETTER(fxset);
ALTRA_SPRITE_METHOD(inc_fx_idx);
ALTRA_SPRITE_METHOD(dec_fx_idx);

#undef ALTRA_SPRITE_METHOD
#undef ALTRA_SPRITE_SETTER

#if defined(__cplusplus)
}
#endif

#endif
