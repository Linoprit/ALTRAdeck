extern "C" {
#include "py/objlist.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include <stdint.h>

#include "canvas.h"
#include "fxbuf.h"
#include "fxbufset.h"
#include "sprite.h"

extern const mp_obj_type_t mp_type_point;
extern const mp_obj_type_t mp_type_poly;
extern const mp_obj_type_t mp_type_sprite;

static altra_canvas_obj_t *altra_canvas_from_obj(mp_obj_t self_in) {
    return static_cast<altra_canvas_obj_t *>(MP_OBJ_TO_PTR(self_in));
}

static void altra_canvas_sync(altra_canvas_obj_t *self) {
    mp_obj_list_get(self->content_obj, &self->count, &self->content);
}

static void altra_canvas_lock(altra_canvas_obj_t *self) {
#if MICROPY_PY_THREAD
    mp_thread_mutex_lock(&self->state_mutex, 1);
#else
    (void)self;
#endif
}

static void altra_canvas_unlock(altra_canvas_obj_t *self) {
#if MICROPY_PY_THREAD
    mp_thread_mutex_unlock(&self->state_mutex);
#else
    (void)self;
#endif
}

static void altra_canvas_fill(mp_obj_t fxbuf, mp_int_t color) {
    altra_fxbuf_fill(fxbuf, color);
}

static mp_int_t altra_canvas_get_timeout(mp_obj_t timeout_in) {
    mp_int_t timeout = mp_obj_get_int(timeout_in);
    if (timeout < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be non-negative"));
    }
    return timeout;
}

typedef struct {
    mp_int_t x;
    mp_int_t y;
    size_t width;
    size_t height;
} altra_sprite_bounds_t;

typedef struct {
    const mp_obj_t *points;
    size_t count;
    double x;
    double y;
    size_t width;
    size_t height;
    bool is_bounds;
} altra_collision_polygon_t;

static void altra_canvas_get_sprite_bounds(const altra_sprite_obj_t *sprite,
    altra_sprite_bounds_t *bounds) {
    const auto *fxset = static_cast<const altra_fxbufset_obj_t *>(
        MP_OBJ_TO_PTR(sprite->fxset));
    if (sprite->fx_idx >= fxset->count) {
        mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("sprite fx_idx out of range"));
    }

    mp_obj_t fxbuf = fxset->fbufs[sprite->fx_idx];
    if (!altra_fxbuf_get_info(fxbuf, &bounds->width, &bounds->height)) {
        mp_raise_TypeError(MP_ERROR_TEXT("sprite fxset contains a non-fxbuf object"));
    }
    bounds->x = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_x));
    bounds->y = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_y));
}

static int64_t altra_canvas_add_size(int64_t value, size_t size) {
    if (value > INT64_MAX - (int64_t)size) {
        return INT64_MAX;
    }
    return value + (int64_t)size;
}

static bool altra_canvas_bounds_overlap(const altra_sprite_bounds_t *a,
    const altra_sprite_bounds_t *b) {
    int64_t a_right = altra_canvas_add_size(a->x, a->width);
    int64_t b_right = altra_canvas_add_size(b->x, b->width);
    int64_t a_bottom = altra_canvas_add_size(a->y, a->height);
    int64_t b_bottom = altra_canvas_add_size(b->y, b->height);
    return a->x <= b_right && b->x <= a_right
        && a->y <= b_bottom && b->y <= a_bottom;
}

static void altra_canvas_get_collision_polygon(const altra_sprite_obj_t *sprite,
    altra_collision_polygon_t *polygon) {
    altra_sprite_bounds_t bounds;
    altra_canvas_get_sprite_bounds(sprite, &bounds);
    polygon->x = (double)bounds.x;
    polygon->y = (double)bounds.y;
    polygon->width = bounds.width;
    polygon->height = bounds.height;

    mp_obj_t collis_box = altra_fxbuf_get_collis_box(
        static_cast<const altra_fxbufset_obj_t *>(MP_OBJ_TO_PTR(sprite->fxset))
            ->fbufs[sprite->fx_idx]);
    if (!mp_obj_is_type(collis_box, &mp_type_poly)) {
        mp_raise_TypeError(MP_ERROR_TEXT("sprite collis_box must be a poly"));
    }

    mp_obj_t points_obj = mp_load_attr(collis_box, MP_QSTR_points);
    size_t count;
    mp_obj_t *points;
    mp_obj_get_array(points_obj, &count, &points);
    if (count == 0) {
        polygon->points = NULL;
        polygon->count = 4;
        polygon->is_bounds = true;
        return;
    }
    if (count < 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("collis_box must have at least three points"));
    }

    polygon->points = points;
    polygon->count = count;
    polygon->is_bounds = false;
}

static void altra_canvas_polygon_point(const altra_collision_polygon_t *polygon,
    size_t index, double *x, double *y) {
    if (polygon->is_bounds) {
        switch (index) {
            case 0:
                *x = 0;
                *y = 0;
                break;
            case 1:
                *x = polygon->width;
                *y = 0;
                break;
            case 2:
                *x = polygon->width;
                *y = polygon->height;
                break;
            default:
                *x = 0;
                *y = polygon->height;
                break;
        }
    } else {
        mp_obj_t point = polygon->points[index];
        *x = mp_obj_get_int(mp_load_attr(point, MP_QSTR_x));
        *y = mp_obj_get_int(mp_load_attr(point, MP_QSTR_y));
    }
    *x += polygon->x;
    *y += polygon->y;
}

static double altra_canvas_polygon_area2(const altra_collision_polygon_t *polygon) {
    double area = 0;
    for (size_t i = 0; i < polygon->count; ++i) {
        double x1, y1, x2, y2;
        altra_canvas_polygon_point(polygon, i, &x1, &y1);
        altra_canvas_polygon_point(polygon, (i + 1) % polygon->count, &x2, &y2);
        area += x1 * y2 - x2 * y1;
    }
    if (area == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("collis_box must have non-zero area"));
    }
    return area;
}

static void altra_canvas_add_pair(mp_obj_t result, mp_obj_t first, mp_obj_t second) {
    mp_obj_t pair[] = { first, second };
    mp_obj_list_append(result, mp_obj_new_tuple(MP_ARRAY_SIZE(pair), pair));
}

mp_obj_t altra_canvas_do_aabb_detection(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_sync(self);
    mp_obj_t result = mp_obj_new_list(0, NULL);

    for (size_t i = 0; i < self->count; ++i) {
        if (!mp_obj_is_type(self->content[i], &mp_type_sprite)) {
            mp_raise_TypeError(MP_ERROR_TEXT("canvas content must contain sprites"));
        }
        const auto *first = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(self->content[i]));
        altra_sprite_bounds_t first_bounds;
        altra_canvas_get_sprite_bounds(first, &first_bounds);

        for (size_t j = i + 1; j < self->count; ++j) {
            if (!mp_obj_is_type(self->content[j], &mp_type_sprite)) {
                mp_raise_TypeError(MP_ERROR_TEXT("canvas content must contain sprites"));
            }
            const auto *second = static_cast<const altra_sprite_obj_t *>(
                MP_OBJ_TO_PTR(self->content[j]));
            if (self->content[i] == self->content[j]
                || first->collis_group != second->collis_group) {
                continue;
            }
            altra_sprite_bounds_t second_bounds;
            altra_canvas_get_sprite_bounds(second, &second_bounds);
            if (altra_canvas_bounds_overlap(&first_bounds, &second_bounds)) {
                altra_canvas_add_pair(result, self->content[i], self->content[j]);
            }
        }
    }
    return result;
}

typedef bool (*altra_collision_test_t)(const altra_sprite_obj_t *,
    const altra_sprite_obj_t *);

static void altra_canvas_get_pair(mp_obj_t pair_obj, mp_obj_t *first_obj,
    mp_obj_t *second_obj) {
    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(pair_obj, &count, &items);
    if (count != 2) {
        mp_raise_ValueError(MP_ERROR_TEXT("each collision pair must contain two sprites"));
    }
    if (!mp_obj_is_type(items[0], &mp_type_sprite)
        || !mp_obj_is_type(items[1], &mp_type_sprite)) {
        mp_raise_TypeError(MP_ERROR_TEXT("collision pairs must contain sprites"));
    }
    *first_obj = items[0];
    *second_obj = items[1];
}

static mp_obj_t altra_canvas_filter_pairs(mp_obj_t pairs_in,
    altra_collision_test_t collision_test) {
    size_t count;
    mp_obj_t *pairs;
    mp_obj_get_array(pairs_in, &count, &pairs);
    mp_obj_t result = mp_obj_new_list(0, NULL);
    for (size_t i = 0; i < count; ++i) {
        mp_obj_t first_obj, second_obj;
        altra_canvas_get_pair(pairs[i], &first_obj, &second_obj);
        const auto *first = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(first_obj));
        const auto *second = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(second_obj));
        if (first_obj != second_obj
            && first->collis_group == second->collis_group
            && collision_test(first, second)) {
            altra_canvas_add_pair(result, first_obj, second_obj);
        }
    }
    return result;
}

static void altra_canvas_project_polygon(const altra_collision_polygon_t *polygon,
    double axis_x, double axis_y, double *minimum, double *maximum) {
    double x, y;
    altra_canvas_polygon_point(polygon, 0, &x, &y);
    *minimum = *maximum = x * axis_x + y * axis_y;
    for (size_t i = 1; i < polygon->count; ++i) {
        altra_canvas_polygon_point(polygon, i, &x, &y);
        double projection = x * axis_x + y * axis_y;
        if (projection < *minimum) {
            *minimum = projection;
        }
        if (projection > *maximum) {
            *maximum = projection;
        }
    }
}

static bool altra_canvas_sta_axes_overlap(
    const altra_collision_polygon_t *source,
    const altra_collision_polygon_t *other) {
    for (size_t i = 0; i < source->count; ++i) {
        double x1, y1, x2, y2;
        altra_canvas_polygon_point(source, i, &x1, &y1);
        altra_canvas_polygon_point(source, (i + 1) % source->count, &x2, &y2);
        double axis_x = y1 - y2;
        double axis_y = x2 - x1;
        if (axis_x == 0 && axis_y == 0) {
            continue;
        }
        double source_min, source_max, other_min, other_max;
        altra_canvas_project_polygon(source, axis_x, axis_y,
            &source_min, &source_max);
        altra_canvas_project_polygon(other, axis_x, axis_y,
            &other_min, &other_max);
        if (source_max < other_min || other_max < source_min) {
            return false;
        }
    }
    return true;
}

static bool altra_canvas_sta_collision(const altra_sprite_obj_t *first,
    const altra_sprite_obj_t *second) {
    altra_collision_polygon_t first_polygon, second_polygon;
    altra_canvas_get_collision_polygon(first, &first_polygon);
    altra_canvas_get_collision_polygon(second, &second_polygon);
    (void)altra_canvas_polygon_area2(&first_polygon);
    (void)altra_canvas_polygon_area2(&second_polygon);
    return altra_canvas_sta_axes_overlap(&first_polygon, &second_polygon)
        && altra_canvas_sta_axes_overlap(&second_polygon, &first_polygon);
}

static bool altra_canvas_ent_separated(const altra_collision_polygon_t *source,
    const altra_collision_polygon_t *other, double orientation) {
    for (size_t i = 0; i < source->count; ++i) {
        double x1, y1, x2, y2;
        altra_canvas_polygon_point(source, i, &x1, &y1);
        altra_canvas_polygon_point(source, (i + 1) % source->count, &x2, &y2);
        bool all_outside = true;
        for (size_t j = 0; j < other->count; ++j) {
            double x, y;
            altra_canvas_polygon_point(other, j, &x, &y);
            double side = ((x2 - x1) * (y - y1) - (y2 - y1) * (x - x1))
                * orientation;
            if (side >= 0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return true;
        }
    }
    return false;
}

static bool altra_canvas_ent_collision(const altra_sprite_obj_t *first,
    const altra_sprite_obj_t *second) {
    altra_collision_polygon_t first_polygon, second_polygon;
    altra_canvas_get_collision_polygon(first, &first_polygon);
    altra_canvas_get_collision_polygon(second, &second_polygon);
    double first_area = altra_canvas_polygon_area2(&first_polygon);
    double second_area = altra_canvas_polygon_area2(&second_polygon);
    return !altra_canvas_ent_separated(&first_polygon, &second_polygon,
            first_area > 0 ? 1 : -1)
        && !altra_canvas_ent_separated(&second_polygon, &first_polygon,
            second_area > 0 ? 1 : -1);
}

static uint16_t altra_canvas_transparency_key(int32_t color) {
    return (uint16_t)(((color >> 8) & 0xf800)
        | ((color >> 5) & 0x07e0)
        | ((color >> 3) & 0x001f));
}

static bool altra_canvas_pixelwise_collision(const altra_sprite_obj_t *first,
    const altra_sprite_obj_t *second) {
    altra_sprite_bounds_t first_bounds, second_bounds;
    altra_canvas_get_sprite_bounds(first, &first_bounds);
    altra_canvas_get_sprite_bounds(second, &second_bounds);

    int64_t left = first_bounds.x > second_bounds.x
        ? first_bounds.x : second_bounds.x;
    int64_t top = first_bounds.y > second_bounds.y
        ? first_bounds.y : second_bounds.y;
    int64_t right1 = altra_canvas_add_size(first_bounds.x, first_bounds.width);
    int64_t right2 = altra_canvas_add_size(second_bounds.x, second_bounds.width);
    int64_t bottom1 = altra_canvas_add_size(first_bounds.y, first_bounds.height);
    int64_t bottom2 = altra_canvas_add_size(second_bounds.y, second_bounds.height);
    int64_t right = right1 < right2 ? right1 : right2;
    int64_t bottom = bottom1 < bottom2 ? bottom1 : bottom2;
    if (right <= left || bottom <= top) {
        return false;
    }

    const auto *first_set = static_cast<const altra_fxbufset_obj_t *>(
        MP_OBJ_TO_PTR(first->fxset));
    const auto *second_set = static_cast<const altra_fxbufset_obj_t *>(
        MP_OBJ_TO_PTR(second->fxset));
    mp_obj_t first_fxbuf = first_set->fbufs[first->fx_idx];
    mp_obj_t second_fxbuf = second_set->fbufs[second->fx_idx];
    const auto *first_buffer = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(first_fxbuf));
    const auto *second_buffer = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(second_fxbuf));
    mp_buffer_info_t first_raw, second_raw;
    mp_get_buffer_raise(altra_fxbuf_get_raw(first_fxbuf), &first_raw, MP_BUFFER_READ);
    mp_get_buffer_raise(altra_fxbuf_get_raw(second_fxbuf), &second_raw, MP_BUFFER_READ);
    const uint16_t *first_pixels = static_cast<const uint16_t *>(first_raw.buf);
    const uint16_t *second_pixels = static_cast<const uint16_t *>(second_raw.buf);
    uint16_t first_key = altra_canvas_transparency_key(first_buffer->trans_col);
    uint16_t second_key = altra_canvas_transparency_key(second_buffer->trans_col);

    for (int64_t y = top; y < bottom; ++y) {
        size_t first_y = (size_t)(y - first_bounds.y);
        size_t second_y = (size_t)(y - second_bounds.y);
        for (int64_t x = left; x < right; ++x) {
            size_t first_x = (size_t)(x - first_bounds.x);
            size_t second_x = (size_t)(x - second_bounds.x);
            uint16_t first_pixel = first_pixels[first_y * first_bounds.width + first_x];
            uint16_t second_pixel = second_pixels[second_y * second_bounds.width + second_x];
            if (first_pixel != first_key && second_pixel != second_key) {
                return true;
            }
        }
    }
    return false;
}

mp_obj_t altra_canvas_do_sta_detection(mp_obj_t self_in, mp_obj_t pairs_in) {
    (void)self_in;
    return altra_canvas_filter_pairs(pairs_in, altra_canvas_sta_collision);
}

mp_obj_t altra_canvas_do_ent_detection(mp_obj_t self_in, mp_obj_t pairs_in) {
    (void)self_in;
    return altra_canvas_filter_pairs(pairs_in, altra_canvas_ent_collision);
}

mp_obj_t altra_canvas_do_pixelwise_detection(mp_obj_t self_in, mp_obj_t pairs_in) {
    (void)self_in;
    return altra_canvas_filter_pairs(pairs_in, altra_canvas_pixelwise_collision);
}

void altra_canvas_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    bool *flag = NULL;
    if (attr == MP_QSTR_enable_aabb) {
        flag = &self->enable_aabb;
    } else if (attr == MP_QSTR_enable_sta) {
        flag = &self->enable_sta;
    } else if (attr == MP_QSTR_enable_ent) {
        flag = &self->enable_ent;
    } else if (attr == MP_QSTR_enable_pixelwise) {
        flag = &self->enable_pixelwise;
    }

    if (flag == NULL) {
        if (dest[0] == MP_OBJ_NULL) {
            dest[1] = MP_OBJ_SENTINEL;
        }
        return;
    }
    if (dest[0] == MP_OBJ_NULL) {
        dest[0] = mp_obj_new_bool(*flag);
    } else if (dest[1] != MP_OBJ_NULL) {
        *flag = mp_obj_is_true(dest[1]);
        dest[0] = MP_OBJ_NULL;
    }
}

mp_obj_t altra_canvas_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, false);

    mp_int_t width = mp_obj_get_int(args[0]);
    mp_int_t height = mp_obj_get_int(args[1]);
    mp_int_t bkg_col = mp_obj_get_int(args[2]);
    if (width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("canvas dimensions must be in range 1..65535"));
    }

    altra_canvas_obj_t *self = mp_obj_malloc(altra_canvas_obj_t, type);
    self->width = width;
    self->height = height;
    self->bkg_col = bkg_col;
    self->content_obj = mp_const_none;
    self->content = NULL;
    self->count = 0;
    self->object_fxbuf = mp_const_none;
    self->display_fxbuf = mp_const_none;
    self->transfer_complete = false;
    self->new_frame_available = true;
    self->enable_aabb = false;
    self->enable_sta = false;
    self->enable_ent = false;
    self->enable_pixelwise = false;
#if MICROPY_PY_THREAD
    mp_thread_mutex_init(&self->state_mutex);
#endif

    mp_obj_t framebuffer_args[] = {
        args[0],
        args[1],
    };
    self->object_fxbuf = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_fxbuf),
        MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);
    self->display_fxbuf = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_fxbuf),
        MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);
    self->content_obj = mp_obj_new_list(0, NULL);
    self->content = NULL;
    self->count = 0;
    altra_canvas_fill(self->object_fxbuf, bkg_col);
    altra_canvas_fill(self->display_fxbuf, bkg_col);
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

static mp_obj_t altra_canvas_peek_fxbuf(altra_canvas_obj_t *self) {
    altra_canvas_lock(self);
    mp_obj_t fxbuf = self->display_fxbuf;
    altra_canvas_unlock(self);
    return fxbuf;
}

mp_obj_t altra_canvas_get_fxbuf(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_lock(self);
    self->new_frame_available = false;
    mp_obj_t fxbuf = self->display_fxbuf;
    altra_canvas_unlock(self);
    return fxbuf;
}

mp_obj_t altra_canvas_transfer_done(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_lock(self);
    self->transfer_complete = true;
    altra_canvas_unlock(self);
    return mp_const_none;
}

mp_obj_t altra_canvas_is_transfered(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_lock(self);
    bool transfer_complete = self->transfer_complete;
    altra_canvas_unlock(self);
    return mp_obj_new_bool(transfer_complete);
}

mp_obj_t altra_canvas_is_new_frame(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_lock(self);
    bool new_frame_available = self->new_frame_available;
    altra_canvas_unlock(self);
    return mp_obj_new_bool(new_frame_available);
}

mp_obj_t altra_canvas_await_new_frame(mp_obj_t self_in, mp_obj_t timeout_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    mp_int_t timeout = mp_obj_get_int(timeout_in);
    if (timeout < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be non-negative"));
    }

    mp_uint_t start = mp_hal_ticks_ms();
    for (;;) {
        altra_canvas_lock(self);
        bool new_frame_available = self->new_frame_available;
        altra_canvas_unlock(self);
        if (new_frame_available) {
            return mp_const_true;
        }
        if (timeout == 0 || mp_hal_ticks_ms() - start >= (mp_uint_t)timeout) {
            return mp_const_false;
        }
        mp_hal_delay_us(50);
    }
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

mp_obj_t altra_canvas_render(mp_obj_t self_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    altra_canvas_sync(self);

    altra_canvas_fill(self->object_fxbuf, self->bkg_col);

    mp_obj_t destination = self->object_fxbuf;
    for (size_t i = 0; i < self->count; ++i) {
        const auto *sprite = static_cast<const altra_sprite_obj_t *>(
            MP_OBJ_TO_PTR(self->content[i]));
        const auto *fxset = static_cast<const altra_fxbufset_obj_t *>(
            MP_OBJ_TO_PTR(sprite->fxset));
        if (sprite->fx_idx >= fxset->count) {
            mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("sprite fx_idx out of range"));
        }

        mp_obj_t source_fxbuf = fxset->fbufs[sprite->fx_idx];
        mp_int_t x = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_x));
        mp_int_t y = mp_obj_get_int(mp_load_attr(sprite->pos, MP_QSTR_y));
        altra_fxbuf_blit(destination, source_fxbuf, x, y);
    }

    return mp_const_none;
}

mp_obj_t altra_canvas_await_transfer(mp_obj_t self_in, mp_obj_t timeout_in) {
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    mp_int_t timeout = altra_canvas_get_timeout(timeout_in);

    mp_uint_t start = mp_hal_ticks_ms();
    for (;;) {
        altra_canvas_lock(self);
        bool transfer_complete = self->transfer_complete;
        if (transfer_complete) {
            mp_obj_t old_display_fxbuf = self->display_fxbuf;
            self->display_fxbuf = self->object_fxbuf;
            self->object_fxbuf = old_display_fxbuf;
            self->transfer_complete = false;
            self->new_frame_available = true;
            altra_canvas_unlock(self);
            return mp_const_true;
        }
        altra_canvas_unlock(self);
        if (timeout == 0 || mp_hal_ticks_ms() - start >= (mp_uint_t)timeout) {
            return mp_const_false;
        }
        mp_hal_delay_us(50);
    }
}

mp_obj_t altra_canvas_cycle(mp_obj_t self_in, mp_obj_t timeout_in) {
    altra_canvas_get_timeout(timeout_in);
    altra_canvas_obj_t *self = altra_canvas_from_obj(self_in);
    if (self->enable_aabb || self->enable_sta || self->enable_ent
        || self->enable_pixelwise) {
        mp_obj_t pairs = altra_canvas_do_aabb_detection(self_in);
        if (self->enable_sta) {
            altra_canvas_do_sta_detection(self_in, pairs);
        }
        if (self->enable_ent) {
            altra_canvas_do_ent_detection(self_in, pairs);
        }
        if (self->enable_pixelwise) {
            altra_canvas_do_pixelwise_detection(self_in, pairs);
        }
    }
    altra_canvas_render(self_in);
    if (!mp_obj_is_true(altra_canvas_await_transfer(self_in, timeout_in))) {
        return mp_const_false;
    }
    return altra_canvas_peek_fxbuf(altra_canvas_from_obj(self_in));
}
}
