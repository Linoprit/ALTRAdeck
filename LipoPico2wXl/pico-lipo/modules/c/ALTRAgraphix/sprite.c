#include "py/runtime.h"

#include "sprite.h"

static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_id_obj, altra_sprite_get_id);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_id_obj, altra_sprite_set_id);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_pos_obj, altra_sprite_get_pos);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_pos_obj, altra_sprite_set_pos);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_z_coord_obj, altra_sprite_get_z_coord);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_z_coord_obj, altra_sprite_set_z_coord);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_rotation_obj, altra_sprite_get_rotation);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_rotation_obj, altra_sprite_set_rotation);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_scale_obj, altra_sprite_get_scale);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_scale_obj, altra_sprite_set_scale);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_collis_box_obj, altra_sprite_get_collis_box);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_collis_box_obj, altra_sprite_set_collis_box);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_collis_group_obj, altra_sprite_get_collis_group);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_collis_group_obj, altra_sprite_set_collis_group);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_auto_anim_obj, altra_sprite_get_auto_anim);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_auto_anim_obj, altra_sprite_set_auto_anim);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_has_changed_obj, altra_sprite_get_has_changed);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_has_changed_obj, altra_sprite_set_has_changed);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_fx_idx_obj, altra_sprite_get_fx_idx);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_fx_idx_obj, altra_sprite_set_fx_idx);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_get_fxset_obj, altra_sprite_get_fxset);
static MP_DEFINE_CONST_FUN_OBJ_2(altra_sprite_set_fxset_obj, altra_sprite_set_fxset);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_inc_fx_idx_obj, altra_sprite_inc_fx_idx);
static MP_DEFINE_CONST_FUN_OBJ_1(altra_sprite_dec_fx_idx_obj, altra_sprite_dec_fx_idx);

static const mp_rom_map_elem_t altra_sprite_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_UP_LEFT), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_UP_RIGHT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_CENTER), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_LO_LEFT), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_LO_RIGHT), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_get_id), MP_ROM_PTR(&altra_sprite_get_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_id), MP_ROM_PTR(&altra_sprite_set_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pos), MP_ROM_PTR(&altra_sprite_get_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pos), MP_ROM_PTR(&altra_sprite_set_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_z_coord), MP_ROM_PTR(&altra_sprite_get_z_coord_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_z_coord), MP_ROM_PTR(&altra_sprite_set_z_coord_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_rotation), MP_ROM_PTR(&altra_sprite_get_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rotation), MP_ROM_PTR(&altra_sprite_set_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_scale), MP_ROM_PTR(&altra_sprite_get_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_scale), MP_ROM_PTR(&altra_sprite_set_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_collis_box), MP_ROM_PTR(&altra_sprite_get_collis_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_collis_box), MP_ROM_PTR(&altra_sprite_set_collis_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_collis_group), MP_ROM_PTR(&altra_sprite_get_collis_group_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_collis_group), MP_ROM_PTR(&altra_sprite_set_collis_group_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_auto_anim), MP_ROM_PTR(&altra_sprite_get_auto_anim_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_auto_anim), MP_ROM_PTR(&altra_sprite_set_auto_anim_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_has_changed), MP_ROM_PTR(&altra_sprite_get_has_changed_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_has_changed), MP_ROM_PTR(&altra_sprite_set_has_changed_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fx_idx), MP_ROM_PTR(&altra_sprite_get_fx_idx_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_fx_idx), MP_ROM_PTR(&altra_sprite_set_fx_idx_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_fxset), MP_ROM_PTR(&altra_sprite_get_fxset_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_fxset), MP_ROM_PTR(&altra_sprite_set_fxset_obj) },
    { MP_ROM_QSTR(MP_QSTR_inc_fx_idx), MP_ROM_PTR(&altra_sprite_inc_fx_idx_obj) },
    { MP_ROM_QSTR(MP_QSTR_dec_fx_idx), MP_ROM_PTR(&altra_sprite_dec_fx_idx_obj) },
};
static MP_DEFINE_CONST_DICT(altra_sprite_locals, altra_sprite_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_sprite,
    MP_QSTR_sprite,
    MP_TYPE_FLAG_NONE,
    make_new, altra_sprite_make_new,
    locals_dict, &altra_sprite_locals
    );
