#include "py/objlist.h"
#include "py/runtime.h"

typedef struct _altra_point_obj_t {
    mp_obj_base_t base;
    mp_int_t x;
    mp_int_t y;
} altra_point_obj_t;

typedef struct _altra_box_obj_t {
    mp_obj_base_t base;
    mp_obj_t points[4];
} altra_box_obj_t;

typedef struct _altra_poly_obj_t {
    mp_obj_base_t base;
    mp_obj_t points;
} altra_poly_obj_t;

extern const mp_obj_type_t mp_type_point;
extern const mp_obj_type_t mp_type_box;
extern const mp_obj_type_t mp_type_poly;

static mp_obj_t altra_point_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    altra_point_obj_t *self = mp_obj_malloc(altra_point_obj_t, type);
    self->x = mp_obj_get_int(args[0]);
    self->y = mp_obj_get_int(args[1]);
    return MP_OBJ_FROM_PTR(self);
}

static void altra_point_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_point_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_x) {
            dest[0] = mp_obj_new_int(self->x);
        } else if (attr == MP_QSTR_y) {
            dest[0] = mp_obj_new_int(self->y);
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        if (attr == MP_QSTR_x) {
            self->x = mp_obj_get_int(dest[1]);
            dest[0] = MP_OBJ_NULL;
        } else if (attr == MP_QSTR_y) {
            self->y = mp_obj_get_int(dest[1]);
            dest[0] = MP_OBJ_NULL;
        }
    }
}

static mp_obj_t altra_box_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 4, 4, false);
    for (size_t i = 0; i < 4; ++i) {
        if (!mp_obj_is_type(args[i], &mp_type_point)) {
            mp_raise_TypeError(MP_ERROR_TEXT("box points must be point objects"));
        }
    }

    altra_box_obj_t *self = mp_obj_malloc(altra_box_obj_t, type);
    for (size_t i = 0; i < 4; ++i) {
        self->points[i] = args[i];
    }
    return MP_OBJ_FROM_PTR(self);
}

static void altra_box_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_box_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_p1) {
            dest[0] = self->points[0];
        } else if (attr == MP_QSTR_p2) {
            dest[0] = self->points[1];
        } else if (attr == MP_QSTR_p3) {
            dest[0] = self->points[2];
        } else if (attr == MP_QSTR_p4) {
            dest[0] = self->points[3];
        } else if (attr == MP_QSTR_points) {
            dest[0] = mp_obj_new_tuple(4, self->points);
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        size_t index;
        if (attr == MP_QSTR_p1) {
            index = 0;
        } else if (attr == MP_QSTR_p2) {
            index = 1;
        } else if (attr == MP_QSTR_p3) {
            index = 2;
        } else if (attr == MP_QSTR_p4) {
            index = 3;
        } else {
            return;
        }
        if (!mp_obj_is_type(dest[1], &mp_type_point)) {
            mp_raise_TypeError(MP_ERROR_TEXT("box points must be point objects"));
        }
        self->points[index] = dest[1];
        dest[0] = MP_OBJ_NULL;
    }
}

static mp_obj_t altra_poly_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_obj_t points = mp_obj_new_list(0, NULL);
    mp_obj_t iterator = mp_getiter(args[0], NULL);
    mp_obj_t point;
    while ((point = mp_iternext(iterator)) != MP_OBJ_STOP_ITERATION) {
        if (!mp_obj_is_type(point, &mp_type_point)) {
            mp_raise_TypeError(MP_ERROR_TEXT("poly items must be point objects"));
        }
        mp_obj_list_append(points, point);
    }

    altra_poly_obj_t *self = mp_obj_malloc(altra_poly_obj_t, type);
    self->points = points;
    return MP_OBJ_FROM_PTR(self);
}

static void altra_poly_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_poly_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_points) {
            dest[0] = self->points;
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL && attr == MP_QSTR_points) {
        size_t len;
        mp_obj_t *points;
        mp_obj_get_array(dest[1], &len, &points);
        for (size_t i = 0; i < len; ++i) {
            if (!mp_obj_is_type(points[i], &mp_type_point)) {
                mp_raise_TypeError(MP_ERROR_TEXT("poly items must be point objects"));
            }
        }
        self->points = dest[1];
        dest[0] = MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_point,
    MP_QSTR_point,
    MP_TYPE_FLAG_NONE,
    make_new, altra_point_make_new,
    attr, altra_point_attr
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_box,
    MP_QSTR_box,
    MP_TYPE_FLAG_NONE,
    make_new, altra_box_make_new,
    attr, altra_box_attr
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_poly,
    MP_QSTR_poly,
    MP_TYPE_FLAG_NONE,
    make_new, altra_poly_make_new,
    attr, altra_poly_attr
    );

static const mp_rom_map_elem_t altra_types_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_altra_dot_types) },
    { MP_ROM_QSTR(MP_QSTR_point), MP_ROM_PTR(&mp_type_point) },
    { MP_ROM_QSTR(MP_QSTR_box), MP_ROM_PTR(&mp_type_box) },
    { MP_ROM_QSTR(MP_QSTR_poly), MP_ROM_PTR(&mp_type_poly) },
};
static MP_DEFINE_CONST_DICT(altra_types_globals, altra_types_globals_table);

const mp_obj_module_t altra_types_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&altra_types_globals,
};
