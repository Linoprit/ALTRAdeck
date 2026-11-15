extern "C" {
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/misc.h"
#include "py/runtime.h"

#include <string.h>

#include "fxbufset.h"
#include "fxbuf.h"

mp_obj_t altra_fxbufset_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    altra_fxbufset_obj_t *self = mp_obj_malloc(altra_fxbufset_obj_t, type);
    self->bufset_obj = mp_obj_new_list(0, NULL);
    self->fbufs = NULL;
    self->count = 0;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t altra_fxbufset_add_fxbuf(mp_obj_t self_in, mp_obj_t fbuf_in) {
    auto *self = static_cast<altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    if (!mp_obj_is_type(fbuf_in, &mp_type_fxbuf)) {
        return mp_const_false;
    }

    mp_obj_list_append(self->bufset_obj, fbuf_in);
    size_t count;
    mp_obj_list_get(self->bufset_obj, &count, &self->fbufs);
    self->count = count;
    return mp_const_true;
}

mp_obj_t altra_fxbufset_set_fxbuf(mp_obj_t self_in, mp_obj_t fbuf_in,
    mp_obj_t index_in) {
    auto *self = static_cast<altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    if (!mp_obj_is_type(fbuf_in, &mp_type_fxbuf)) {
        return mp_const_false;
    }

    size_t index = mp_get_index(self->base.type, self->count, index_in, false);
    mp_obj_list_store(self->bufset_obj, mp_obj_new_int_from_uint(index), fbuf_in);
    mp_obj_list_get(self->bufset_obj, &self->count, &self->fbufs);
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

static void altra_fxbufset_sync(altra_fxbufset_obj_t *self) {
    mp_obj_list_get(self->bufset_obj, &self->count, &self->fbufs);
}

mp_obj_t altra_fxbufset_load_fxbuf(mp_obj_t self_in, mp_obj_t filename_in,
    mp_obj_t index_in, bool swap_endian) {
    auto *self = static_cast<altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    mp_int_t index = mp_obj_get_int(index_in);
    if (index < 0 || (mp_uint_t)index > self->count) {
        mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("fxbufset index out of range"));
    }

    mp_obj_t fxbuf = altra_fxbuf_load_from_file_with_endian(filename_in,
        swap_endian);
    if ((mp_uint_t)index == self->count) {
        mp_obj_list_append(self->bufset_obj, fxbuf);
    } else {
        mp_obj_list_store(self->bufset_obj, mp_obj_new_int(index), fxbuf);
    }
    altra_fxbufset_sync(self);
    return fxbuf;
}

mp_obj_t altra_fxbufset_save_fxbuf(mp_obj_t self_in, mp_obj_t filename_in,
    mp_obj_t index_in, bool swap_endian) {
    const auto *self = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    size_t index = mp_get_index(self->base.type, self->count, index_in, false);
    return altra_fxbuf_save_with_endian(self->fbufs[index], filename_in,
        swap_endian);
}

mp_obj_t altra_fxbufset_load_fxbufset(size_t n_args, const mp_obj_t *args) {
    mp_arg_check_num(n_args, 0, 2, MP_OBJ_FUN_ARGS_MAX, false);
    size_t filename_end = n_args;
    bool swap_endian = false;
    if (n_args > 2 && mp_obj_is_bool(args[n_args - 1])) {
        swap_endian = mp_obj_is_true(args[n_args - 1]);
        --filename_end;
    }

    auto *self = mp_obj_malloc(altra_fxbufset_obj_t, &mp_type_fxbufset);
    self->bufset_obj = mp_obj_new_list(0, NULL);
    self->fbufs = NULL;
    self->count = 0;
    for (size_t i = 1; i < filename_end; ++i) {
        mp_obj_list_append(self->bufset_obj,
            altra_fxbuf_load_from_file_with_endian(args[i], swap_endian));
    }
    altra_fxbufset_sync(self);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t altra_fxbufset_numbered_filename(mp_obj_t filename_in, size_t index) {
    if (!mp_obj_is_str(filename_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("filename must be a string"));
    }

    GET_STR_DATA_LEN(filename_in, filename_data, filename_len);
    size_t base_len = filename_len;
    if ((base_len >= 5 && memcmp(filename_data + base_len - 5, ".data", 5) == 0)
        || (base_len >= 5 && memcmp(filename_data + base_len - 5, ".json", 5) == 0)) {
        base_len -= 5;
    }

    vstr_t numbered_filename;
    vstr_init(&numbered_filename, base_len + 8);
    vstr_add_strn(&numbered_filename, (const char *)filename_data, base_len);
    vstr_printf(&numbered_filename, "_%03u", (unsigned)index);
    return mp_obj_new_str_from_vstr(&numbered_filename);
}

mp_obj_t altra_fxbufset_save_fxbufset(mp_obj_t self_in, mp_obj_t filename_in,
    bool swap_endian) {
    const auto *self = static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(self_in));
    for (size_t index = 0; index < self->count; ++index) {
        mp_obj_t numbered_filename = altra_fxbufset_numbered_filename(filename_in, index);
        altra_fxbuf_save_with_endian(self->fbufs[index], numbered_filename,
            swap_endian);
    }
    return mp_const_none;
}
}
