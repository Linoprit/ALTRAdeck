extern "C" {

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "extmod/vfs.h"
#include "py/mperrno.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/misc.h"

#include "fxbuf.h"

extern const mp_obj_type_t mp_type_point;
extern const mp_obj_type_t mp_type_poly;

static mp_obj_t altra_fxbuf_wrap(mp_obj_t buffer, size_t width, size_t height) {
    altra_fxbuf_obj_t *self = mp_obj_malloc(altra_fxbuf_obj_t, &mp_type_fxbuf);
    self->buffer = buffer;
    self->width = width;
    self->height = height;
    self->trans_col = 0;
    self->collis_box = mp_call_function_1(MP_OBJ_FROM_PTR(&mp_type_poly),
        mp_obj_new_list(0, NULL));
    return MP_OBJ_FROM_PTR(self);
}

static uint16_t altra_fxbuf_rgb888_to_rgb565(uint32_t color) {
    return (uint16_t)(((color >> 8) & 0xf800)
        | ((color >> 5) & 0x07e0)
        | ((color >> 3) & 0x001f));
}

static size_t altra_fxbuf_buffer_size(size_t width, size_t height) {
    if (width == 0 || height == 0 || width > UINT16_MAX || height > UINT16_MAX
        || width > SIZE_MAX / 2 / height) {
        mp_raise_ValueError(MP_ERROR_TEXT("fxbuf dimensions are too large"));
    }
    return width * height * 2;
}

mp_obj_t altra_fxbuf_get_trans_col(mp_obj_t self_in) {
    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    return mp_obj_new_int(self->trans_col);
}

mp_obj_t altra_fxbuf_set_trans_col(mp_obj_t self_in, mp_obj_t value_in) {
    mp_int_t value = mp_obj_get_int(value_in);
    if (value < 0 || value > 0xffffff) {
        mp_raise_ValueError(MP_ERROR_TEXT("trans_col must be an RGB888 color"));
    }

    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    self->trans_col = value;
    return mp_const_none;
}

mp_obj_t altra_fxbuf_get_collis_box(mp_obj_t self_in) {
    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    return self->collis_box;
}

mp_obj_t altra_fxbuf_set_collis_box(mp_obj_t self_in, mp_obj_t value_in) {
    if (!mp_obj_is_type(value_in, &mp_type_poly)) {
        mp_raise_TypeError(MP_ERROR_TEXT("collis_box must be a poly"));
    }

    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    self->collis_box = value_in;
    return mp_const_none;
}

mp_obj_t altra_fxbuf_to_rgb565(mp_obj_t color_in) {
    mp_int_t color = mp_obj_get_int(color_in);
    if (color < 0 || color > 0xffffff) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB888 color must be in range 0..0xFFFFFF"));
    }

    mp_obj_t channels[] = {
        mp_obj_new_int((color >> 19) & 0x1f),
        mp_obj_new_int((color >> 10) & 0x3f),
        mp_obj_new_int((color >> 3) & 0x1f),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(channels), channels);
}

mp_obj_t altra_fxbuf_to_color(mp_obj_t red_in, mp_obj_t green_in, mp_obj_t blue_in) {
    mp_int_t red = mp_obj_get_int(red_in);
    mp_int_t green = mp_obj_get_int(green_in);
    mp_int_t blue = mp_obj_get_int(blue_in);
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB888 channels must be in range 0..255"));
    }

    uint32_t color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
    return mp_obj_new_int_from_uint(color);
}

bool altra_fxbuf_get_info(mp_obj_t obj, size_t *width, size_t *height) {
    if (!mp_obj_is_type(obj, &mp_type_fxbuf)) {
        return false;
    }
    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(obj));
    *width = self->width;
    *height = self->height;
    return true;
}

void altra_fxbuf_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_width) {
            dest[0] = mp_obj_new_int_from_uint(self->width);
        } else if (attr == MP_QSTR_height) {
            dest[0] = mp_obj_new_int_from_uint(self->height);
        } else if (attr == MP_QSTR_collis_box) {
            dest[0] = self->collis_box;
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        if (attr == MP_QSTR_collis_box) {
            altra_fxbuf_set_collis_box(self_in, dest[1]);
            dest[0] = MP_OBJ_NULL;
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    }
}

mp_int_t altra_fxbuf_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo,
    mp_uint_t flags) {
    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    return mp_get_buffer(self->buffer, bufinfo, flags) ? 0 : 1;
}

mp_obj_t altra_fxbuf_get_raw(mp_obj_t self_in) {
    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    return self->buffer;
}

void altra_fxbuf_fill(mp_obj_t self_in, mp_int_t color) {
    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    mp_buffer_info_t raw;
    mp_get_buffer_raise(self->buffer, &raw, MP_BUFFER_WRITE);
    uint16_t *pixels = static_cast<uint16_t *>(raw.buf);
    size_t count = self->width * self->height;
    uint16_t rgb565 = (uint16_t)color;
    for (size_t i = 0; i < count; ++i) {
        pixels[i] = rgb565;
    }
}

void altra_fxbuf_blit(mp_obj_t self_in, mp_obj_t source_in, mp_int_t x,
    mp_int_t y) {
    if (!mp_obj_is_type(self_in, &mp_type_fxbuf)
        || !mp_obj_is_type(source_in, &mp_type_fxbuf)) {
        mp_raise_TypeError(MP_ERROR_TEXT("blit expects an fxbuf source"));
    }

    altra_fxbuf_obj_t *destination = static_cast<altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    const altra_fxbuf_obj_t *source = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(source_in));
    if (x >= (mp_int_t)destination->width || y >= (mp_int_t)destination->height
        || x <= -(mp_int_t)source->width || y <= -(mp_int_t)source->height) {
        return;
    }

    size_t source_x = x < 0 ? (size_t)-x : 0;
    size_t source_y = y < 0 ? (size_t)-y : 0;
    size_t destination_x = x > 0 ? (size_t)x : 0;
    size_t destination_y = y > 0 ? (size_t)y : 0;
    size_t copy_width = MIN(source->width - source_x,
        destination->width - destination_x);
    size_t copy_height = MIN(source->height - source_y,
        destination->height - destination_y);
    mp_buffer_info_t destination_raw;
    mp_buffer_info_t source_raw;
    mp_get_buffer_raise(destination->buffer, &destination_raw, MP_BUFFER_WRITE);
    mp_get_buffer_raise(source->buffer, &source_raw, MP_BUFFER_READ);
    uint16_t *destination_pixels = static_cast<uint16_t *>(destination_raw.buf);
    const uint16_t *source_pixels = static_cast<const uint16_t *>(source_raw.buf);
    uint16_t trans_key = altra_fxbuf_rgb888_to_rgb565(source->trans_col);

    for (size_t row = 0; row < copy_height; ++row) {
        uint16_t *destination_row = destination_pixels
            + (destination_y + row) * destination->width + destination_x;
        const uint16_t *source_row = source_pixels
            + (source_y + row) * source->width + source_x;
        for (size_t column = 0; column < copy_width; ++column) {
            uint16_t pixel = source_row[column];
            if (pixel != trans_key) {
                destination_row[column] = pixel;
            }
        }
    }
}

mp_obj_t altra_fxbuf_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    (void)type;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    mp_int_t width = mp_obj_get_int(args[0]);
    mp_int_t height = mp_obj_get_int(args[1]);
    if (width < 1 || height < 1 || width > UINT16_MAX || height > UINT16_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("fxbuf dimensions must be in range 1..65535"));
    }

    size_t buffer_size = altra_fxbuf_buffer_size(width, height);
    uint8_t *pixels = m_new0(uint8_t, buffer_size);
    mp_obj_t buffer = mp_obj_new_bytearray_by_ref(buffer_size, pixels);
    return altra_fxbuf_wrap(buffer, width, height);
}

static mp_obj_t altra_fxbuf_make_path(mp_obj_t filename_in, const char *suffix) {
    if (!mp_obj_is_str(filename_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("filename must be a string"));
    }

    GET_STR_DATA_LEN(filename_in, filename_data, filename_len);
    size_t suffix_len = strlen(suffix);
    size_t base_len = filename_len;
    if ((base_len >= 5 && memcmp(filename_data + base_len - 5, ".data", 5) == 0)
        || (base_len >= 5 && memcmp(filename_data + base_len - 5, ".json", 5) == 0)) {
        base_len -= 5;
    }

    vstr_t path;
    vstr_init(&path, base_len + suffix_len);
    vstr_add_strn(&path, (const char *)filename_data, base_len);
    vstr_add_strn(&path, suffix, suffix_len);
    return mp_obj_new_str_from_vstr(&path);
}

static mp_obj_t altra_fxbuf_open_file(mp_obj_t path, const char *mode) {
    mp_obj_t args[] = {
        path,
        mp_obj_new_str_from_cstr(mode),
    };
    return mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_vfs_open_obj),
        MP_ARRAY_SIZE(args), 0, args);
}

static void altra_fxbuf_write_file(mp_obj_t path, const void *data, size_t len) {
    mp_obj_t file = altra_fxbuf_open_file(path, "wb");
    int errcode;
    mp_uint_t written = mp_stream_rw(file, (void *)data, len, &errcode, MP_STREAM_RW_WRITE);
    if (errcode != 0) {
        mp_stream_close(file);
        mp_raise_OSError(errcode);
    }
    if (written != len) {
        mp_stream_close(file);
        mp_raise_OSError(MP_EIO);
    }
    mp_stream_close(file);
}

static mp_obj_t altra_fxbuf_read_file(mp_obj_t path) {
    mp_obj_t file = altra_fxbuf_open_file(path, "rb");
    mp_obj_t dest[2];
    mp_load_method(file, MP_QSTR_read, dest);
    mp_obj_t data = mp_call_method_n_kw(0, 0, dest);
    mp_stream_close(file);
    return data;
}

static const char *altra_fxbuf_skip_space(const char *p) {
    while (*p != '\0' && isspace((unsigned char)*p)) {
        ++p;
    }
    return p;
}

static bool altra_fxbuf_read_json_uint(const char *json, const char *key, size_t *value) {
    char key_pattern[16];
    size_t key_len = strlen(key);
    if (key_len + 3 > sizeof(key_pattern)) {
        return false;
    }
    key_pattern[0] = '"';
    memcpy(key_pattern + 1, key, key_len);
    key_pattern[key_len + 1] = '"';
    key_pattern[key_len + 2] = '\0';

    const char *p = strstr(json, key_pattern);
    if (p == NULL) {
        return false;
    }
    p = altra_fxbuf_skip_space(p + key_len + 2);
    if (*p++ != ':') {
        return false;
    }
    p = altra_fxbuf_skip_space(p);
    if (!isdigit((unsigned char)*p)) {
        return false;
    }

    errno = 0;
    char *end;
    unsigned long long parsed = strtoull(p, &end, 10);
    if (errno == ERANGE || end == p || parsed > SIZE_MAX) {
        return false;
    }
    *value = (size_t)parsed;
    return true;
}

static bool altra_fxbuf_expect_json_char(const char **cursor, char expected) {
    const char *p = altra_fxbuf_skip_space(*cursor);
    if (*p != expected) {
        return false;
    }
    *cursor = p + 1;
    return true;
}

static bool altra_fxbuf_read_json_int(const char **cursor, mp_int_t *value) {
    const char *p = altra_fxbuf_skip_space(*cursor);
    if (*p != '-' && !isdigit((unsigned char)*p)) {
        return false;
    }

    errno = 0;
    char *end;
    long long parsed = strtoll(p, &end, 10);
    if (errno == ERANGE || end == p) {
        return false;
    }
    mp_int_t converted = (mp_int_t)parsed;
    if ((long long)converted != parsed) {
        return false;
    }
    *value = converted;
    *cursor = end;
    return true;
}

static bool altra_fxbuf_read_json_collis_box(const char *json,
    mp_obj_t *collis_box) {
    const char *p = strstr(json, "\"collis_box\"");
    if (p == NULL) {
        *collis_box = MP_OBJ_NULL;
        return true;
    }

    p = altra_fxbuf_skip_space(p + strlen("\"collis_box\""));
    if (*p++ != ':') {
        return false;
    }
    if (!altra_fxbuf_expect_json_char(&p, '[')) {
        return false;
    }

    mp_obj_t points = mp_obj_new_list(0, NULL);
    p = altra_fxbuf_skip_space(p);
    while (*p != ']') {
        if (!altra_fxbuf_expect_json_char(&p, '[')) {
            return false;
        }
        mp_int_t x;
        mp_int_t y;
        if (!altra_fxbuf_read_json_int(&p, &x)
            || !altra_fxbuf_expect_json_char(&p, ',')
            || !altra_fxbuf_read_json_int(&p, &y)
            || !altra_fxbuf_expect_json_char(&p, ']')) {
            return false;
        }
        mp_obj_t point = mp_call_function_2(MP_OBJ_FROM_PTR(&mp_type_point),
            mp_obj_new_int(x), mp_obj_new_int(y));
        mp_obj_list_append(points, point);

        p = altra_fxbuf_skip_space(p);
        if (*p == ']') {
            break;
        }
        if (*p++ != ',') {
            return false;
        }
        p = altra_fxbuf_skip_space(p);
    }
    ++p;
    *collis_box = mp_call_function_1(MP_OBJ_FROM_PTR(&mp_type_poly), points);
    return true;
}

static uint16_t altra_fxbuf_read_le16(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t altra_fxbuf_read_le32(const uint8_t *data) {
    return (uint32_t)data[0]
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24);
}

static void altra_fxbuf_swap_rgb565_endian(mp_obj_t buffer, size_t width,
    size_t height) {
    mp_buffer_info_t raw;
    mp_get_buffer_raise(buffer, &raw, MP_BUFFER_RW);
    size_t required = altra_fxbuf_buffer_size(width, height);
    if (required > raw.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB565 buffer is too small for its dimensions"));
    }

    uint8_t *data = static_cast<uint8_t *>(raw.buf);
    for (size_t i = 0; i < required; i += 2) {
        uint8_t byte = data[i];
        data[i] = data[i + 1];
        data[i + 1] = byte;
    }
}

static mp_obj_t altra_fxbuf_decode_bmp(const mp_buffer_info_t *raw, size_t *width_out,
    size_t *height_out, bool swap_endian) {
    const uint8_t *bmp = static_cast<const uint8_t *>(raw->buf);
    if (raw->len < 54 || bmp[0] != 'B' || bmp[1] != 'M') {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid or truncated BMP file"));
    }

    uint32_t file_size = altra_fxbuf_read_le32(bmp + 2);
    uint32_t pixel_offset = altra_fxbuf_read_le32(bmp + 10);
    uint32_t dib_size = altra_fxbuf_read_le32(bmp + 14);
    int32_t bmp_width = (int32_t)altra_fxbuf_read_le32(bmp + 18);
    int32_t bmp_height = (int32_t)altra_fxbuf_read_le32(bmp + 22);
    uint16_t planes = altra_fxbuf_read_le16(bmp + 26);
    uint16_t bits_per_pixel = altra_fxbuf_read_le16(bmp + 28);
    uint32_t compression = altra_fxbuf_read_le32(bmp + 30);
    uint64_t abs_height = bmp_height < 0 ? -(int64_t)bmp_height : bmp_height;
    if (dib_size < 40 || (uint64_t)14 + dib_size > raw->len
        || (file_size != 0 && file_size > raw->len)
        || pixel_offset < 14 + dib_size || pixel_offset > raw->len
        || bmp_width <= 0 || abs_height == 0 || bmp_width > UINT16_MAX
        || abs_height > UINT16_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid BMP header or dimensions"));
    }

    if (planes != 1 || bits_per_pixel != 16 || compression != 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("BMP framebuffer data must use RGB565 bitfields"));
    }

    size_t masks_offset;
    if (dib_size >= 52) {
        masks_offset = 14 + 40;
    } else if (dib_size == 40) {
        masks_offset = 14 + dib_size;
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("unsupported BMP bitfield header"));
    }
    if (masks_offset > pixel_offset || pixel_offset - masks_offset < 12
        || altra_fxbuf_read_le32(bmp + masks_offset) != 0xf800
        || altra_fxbuf_read_le32(bmp + masks_offset + 4) != 0x07e0
        || altra_fxbuf_read_le32(bmp + masks_offset + 8) != 0x001f) {
        mp_raise_ValueError(MP_ERROR_TEXT("BMP color masks are not RGB565"));
    }

    size_t width = bmp_width;
    size_t height = (size_t)abs_height;
    uint64_t row_size = ((uint64_t)width * 2 + 3) & ~UINT64_C(3);
    uint64_t pixel_data_size = row_size * height;
    if (pixel_data_size > raw->len - pixel_offset) {
        mp_raise_ValueError(MP_ERROR_TEXT("truncated BMP pixel data"));
    }

    size_t buffer_size = altra_fxbuf_buffer_size(width, height);
    uint8_t *pixels = m_new0(uint8_t, buffer_size);
    mp_obj_t buffer = mp_obj_new_bytearray_by_ref(buffer_size, pixels);

    mp_buffer_info_t decoded;
    mp_get_buffer_raise(buffer, &decoded, MP_BUFFER_WRITE);
    pixels = static_cast<uint8_t *>(decoded.buf);
    for (size_t y = 0; y < height; ++y) {
        size_t source_y = bmp_height > 0 ? height - 1 - y : y;
        const uint8_t *source = bmp + pixel_offset + source_y * row_size;
        memcpy(pixels + y * width * 2, source, width * 2);
    }
    if (swap_endian) {
        altra_fxbuf_swap_rgb565_endian(buffer, width, height);
    }

    *width_out = width;
    *height_out = height;
    return buffer;
}

mp_obj_t altra_fxbuf_save_with_endian(mp_obj_t self_in, mp_obj_t filename_in,
    bool swap_endian) {
    size_t width;
    size_t height;
    if (!altra_fxbuf_get_info(self_in, &width, &height)) {
        mp_raise_TypeError(MP_ERROR_TEXT("save expects an fxbuf"));
    }

    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    mp_buffer_info_t raw;
    mp_get_buffer_raise(self->buffer, &raw, MP_BUFFER_READ);

    mp_obj_t data_path = altra_fxbuf_make_path(filename_in, ".data");
    mp_obj_t json_path = altra_fxbuf_make_path(filename_in, ".json");
    const void *data = raw.buf;
    mp_obj_t swapped_data = MP_OBJ_NULL;
    if (swap_endian) {
        swapped_data = mp_obj_new_bytearray(raw.len, raw.buf);
        altra_fxbuf_swap_rgb565_endian(swapped_data, width, height);
        mp_buffer_info_t swapped;
        mp_get_buffer_raise(swapped_data, &swapped, MP_BUFFER_READ);
        data = swapped.buf;
    }
    altra_fxbuf_write_file(data_path, data, raw.len);

    vstr_t metadata;
    vstr_init(&metadata, 64);
    vstr_printf(&metadata, "{\"width\":%u,\"height\":%u,\"collis_box\":[",
        (unsigned)width, (unsigned)height);
    mp_obj_t points_obj = mp_load_attr(self->collis_box, MP_QSTR_points);
    size_t point_count;
    mp_obj_t *points;
    mp_obj_get_array(points_obj, &point_count, &points);
    for (size_t i = 0; i < point_count; ++i) {
        mp_obj_t point = points[i];
        mp_int_t x = mp_obj_get_int(mp_load_attr(point, MP_QSTR_x));
        mp_int_t y = mp_obj_get_int(mp_load_attr(point, MP_QSTR_y));
        vstr_printf(&metadata, "%s[" INT_FMT "," INT_FMT "]",
            i == 0 ? "" : ",", x, y);
    }
    vstr_printf(&metadata, "]}\n");
    altra_fxbuf_write_file(json_path, vstr_str(&metadata), vstr_len(&metadata));
    vstr_clear(&metadata);
    return mp_const_none;
}

mp_obj_t altra_fxbuf_save(mp_obj_t self_in, mp_obj_t filename_in) {
    return altra_fxbuf_save_with_endian(self_in, filename_in, false);
}

mp_obj_t altra_fxbuf_load_from_file_with_endian(mp_obj_t filename_in,
    bool swap_endian) {
    if (!mp_obj_is_str(filename_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("filename must be a string"));
    }

    GET_STR_DATA_LEN(filename_in, filename_data, filename_len);
    if (filename_len >= 4
        && memcmp(filename_data + filename_len - 4, ".bmp", 4) == 0) {
        mp_obj_t bmp_data = altra_fxbuf_read_file(filename_in);
        mp_buffer_info_t raw_bmp;
        mp_get_buffer_raise(bmp_data, &raw_bmp, MP_BUFFER_READ);

        size_t width;
        size_t height;
        mp_obj_t buffer = altra_fxbuf_decode_bmp(&raw_bmp, &width, &height,
            swap_endian);
        return altra_fxbuf_wrap(buffer, width, height);
    }

    mp_obj_t data_path = altra_fxbuf_make_path(filename_in, ".data");
    mp_obj_t json_path = altra_fxbuf_make_path(filename_in, ".json");
    mp_obj_t raw_data = altra_fxbuf_read_file(data_path);
    mp_obj_t json_data = altra_fxbuf_read_file(json_path);

    mp_buffer_info_t raw;
    mp_buffer_info_t metadata;
    mp_get_buffer_raise(raw_data, &raw, MP_BUFFER_READ);
    mp_get_buffer_raise(json_data, &metadata, MP_BUFFER_READ);
    char *json = m_new(char, metadata.len + 1);
    memcpy(json, metadata.buf, metadata.len);
    json[metadata.len] = '\0';

    size_t width;
    size_t height;
    bool valid = altra_fxbuf_read_json_uint(json, "width", &width)
        && altra_fxbuf_read_json_uint(json, "height", &height);
    mp_obj_t collis_box = MP_OBJ_NULL;
    if (valid) {
        valid = altra_fxbuf_read_json_collis_box(json, &collis_box);
    }
    m_del(char, json, metadata.len + 1);
    if (!valid) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid fxbuf metadata"));
    }

    size_t buffer_size = altra_fxbuf_buffer_size(width, height);
    if (raw.len != buffer_size) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB565 data size does not match metadata"));
    }
    mp_obj_t buffer = mp_obj_new_bytearray(raw.len, raw.buf);
    if (swap_endian) {
        altra_fxbuf_swap_rgb565_endian(buffer, width, height);
    }
    mp_obj_t fxbuf = altra_fxbuf_wrap(buffer, width, height);
    if (collis_box != MP_OBJ_NULL) {
        altra_fxbuf_set_collis_box(fxbuf, collis_box);
    }
    return fxbuf;
}

mp_obj_t altra_fxbuf_load_from_file(mp_obj_t filename_in) {
    return altra_fxbuf_load_from_file_with_endian(filename_in, false);
}

}
