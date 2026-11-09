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

mp_obj_t altra_fxbuf_get_native(mp_obj_t obj) {
    if (mp_obj_is_type(obj, &mp_type_fxbuf)) {
        altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(obj));
        return self->framebuffer;
    }
    return obj;
}

static mp_obj_t altra_fxbuf_wrap(mp_obj_t framebuffer, size_t width, size_t height,
    size_t stride, uint8_t format) {
    altra_fxbuf_obj_t *self = mp_obj_malloc(altra_fxbuf_obj_t, &mp_type_fxbuf);
    self->framebuffer = framebuffer;
    self->width = width;
    self->height = height;
    self->stride = stride;
    self->format = format;
    self->trans_col = 0;
    self->info_valid = true;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t altra_fxbuf_get_trans_col(mp_obj_t self_in) {
    const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
        MP_OBJ_TO_PTR(self_in));
    return mp_obj_new_int(self->trans_col);
}

mp_obj_t altra_fxbuf_set_trans_col(mp_obj_t self_in, mp_obj_t value_in) {
    mp_int_t value = mp_obj_get_int(value_in);
    if (value < INT32_MIN || value > INT32_MAX) {
        mp_raise_msg(&mp_type_OverflowError,
            MP_ERROR_TEXT("trans_col must be a signed 32-bit integer"));
    }

    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    self->trans_col = value;
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

static uint8_t altra_fxbuf_rgb888_luminance(mp_obj_t color_in) {
    mp_int_t color = mp_obj_get_int(color_in);
    if (color < 0 || color > 0xffffff) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB888 color must be in range 0..0xFFFFFF"));
    }

    uint32_t red = (color >> 16) & 0xff;
    uint32_t green = (color >> 8) & 0xff;
    uint32_t blue = color & 0xff;
    return (uint8_t)((299 * red + 587 * green + 114 * blue + 500) / 1000);
}

static mp_obj_t altra_fxbuf_to_grayscale(mp_obj_t color_in, uint8_t max_value) {
    uint32_t luminance = altra_fxbuf_rgb888_luminance(color_in);
    uint32_t value = (luminance * max_value + 127) / 255;
    return mp_obj_new_int_from_uint(value);
}

mp_obj_t altra_fxbuf_to_mono(mp_obj_t color_in) {
    return mp_obj_new_int(altra_fxbuf_rgb888_luminance(color_in) >= 128);
}

mp_obj_t altra_fxbuf_to_gs2(mp_obj_t color_in) {
    return altra_fxbuf_to_grayscale(color_in, 3);
}

mp_obj_t altra_fxbuf_to_gs4(mp_obj_t color_in) {
    return altra_fxbuf_to_grayscale(color_in, 15);
}

mp_obj_t altra_fxbuf_to_gs8(mp_obj_t color_in) {
    return altra_fxbuf_to_grayscale(color_in, 255);
}

uint32_t altra_fxbuf_get_trans_key(mp_obj_t obj) {
    size_t width;
    size_t height;
    size_t stride;
    uint8_t format;
    if (!altra_fxbuf_get_info(obj, &width, &height, &stride, &format)) {
        mp_raise_TypeError(MP_ERROR_TEXT("transparency requires an fxbuf with known format"));
    }

    int32_t color = 0;
    if (mp_obj_is_type(obj, &mp_type_fxbuf)) {
        const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
            MP_OBJ_TO_PTR(obj));
        color = self->trans_col;
    }
    if (color < 0 || color > 0xffffff) {
        mp_raise_ValueError(MP_ERROR_TEXT("trans_col must be an RGB888 color"));
    }

    uint32_t red = ((uint32_t)color >> 16) & 0xff;
    uint32_t green = ((uint32_t)color >> 8) & 0xff;
    uint32_t blue = (uint32_t)color & 0xff;
    switch (format) {
        case 0:
        case 3:
        case 4:
            return (299 * red + 587 * green + 114 * blue + 127500) / 255000;
        case 1:
            return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
        case 2:
            return (((299 * red + 587 * green + 114 * blue + 500) / 1000) * 15
                + 127) / 255;
        case 5:
            return (((299 * red + 587 * green + 114 * blue + 500) / 1000) * 3
                + 127) / 255;
        case 6:
            return (299 * red + 587 * green + 114 * blue + 500) / 1000;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("unsupported fxbuf format for transparency"));
    }
}

bool altra_fxbuf_get_info(mp_obj_t obj, size_t *width, size_t *height, size_t *stride,
    uint8_t *format) {
    if (mp_obj_is_type(obj, &mp_type_fxbuf)) {
        const altra_fxbuf_obj_t *self = static_cast<const altra_fxbuf_obj_t *>(
            MP_OBJ_TO_PTR(obj));
        if (!self->info_valid) {
            return false;
        }
        *width = self->width;
        *height = self->height;
        *stride = self->stride;
        *format = self->format;
        return true;
    }

    if (mp_framebuf_get_rgb565_info(obj, width, height, stride)) {
        *format = 1;
        return true;
    }
    return false;
}

void altra_fxbuf_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    mp_load_method_maybe(self->framebuffer, attr, dest);
    if (dest[0] == MP_OBJ_NULL) {
        dest[1] = MP_OBJ_SENTINEL;
    }
}

mp_int_t altra_fxbuf_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo,
    mp_uint_t flags) {
    altra_fxbuf_obj_t *self = static_cast<altra_fxbuf_obj_t *>(MP_OBJ_TO_PTR(self_in));
    return mp_get_buffer(self->framebuffer, bufinfo, flags) ? 0 : 1;
}

static size_t altra_fxbuf_aligned_stride(size_t stride, mp_int_t format) {
    size_t alignment;
    switch (format) {
        case 0:
        case 1:
        case 6:
            return stride;
        case 2:
            alignment = 2;
            break;
        case 3:
        case 4:
            alignment = 8;
            break;
        case 5:
            alignment = 4;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid framebuffer format"));
    }

    return (stride + alignment - 1) & ~(alignment - 1);
}

static size_t altra_fxbuf_buffer_size(size_t width, size_t height, size_t stride,
    mp_int_t format) {
    uint64_t aligned_stride = altra_fxbuf_aligned_stride(stride, format);
    uint64_t aligned_width = altra_fxbuf_aligned_stride(width, format);
    uint64_t height_required = height;
    uint64_t strides_required = height - 1;
    uint64_t bpp = 1;

    switch (format) {
        case 0:
            height_required = (height + 7) & ~UINT64_C(7);
            strides_required = height_required - 8;
            break;
        case 1:
            bpp = 16;
            break;
        case 2:
            bpp = 4;
            break;
        case 3:
        case 4:
            break;
        case 5:
            bpp = 2;
            break;
        case 6:
            bpp = 8;
            break;
    }

    uint64_t bits = (strides_required * aligned_stride
        + (height_required - strides_required) * aligned_width) * bpp;
    uint64_t bytes = (bits + 7) / 8;
    if (bytes > SIZE_MAX) {
        mp_raise_msg(&mp_type_OverflowError, MP_ERROR_TEXT("fxbuf is too large"));
    }
    return (size_t)bytes;
}

mp_obj_t altra_fxbuf_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    (void)type;
    if (n_args == 3 && n_kw == 0) {
        mp_int_t width = mp_obj_get_int(args[0]);
        mp_int_t height = mp_obj_get_int(args[1]);
        mp_int_t format = mp_obj_get_int(args[2]);
        if (width < 1 || height < 1 || width > UINT16_MAX || height > UINT16_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("fxbuf dimensions must be in range 1..65535"));
        }

        size_t stride = altra_fxbuf_aligned_stride(width, format);
        size_t buffer_size = altra_fxbuf_buffer_size(width, height, width, format);
        uint8_t *blank = m_new0(uint8_t, buffer_size);
        mp_obj_t buffer = mp_obj_new_bytearray(buffer_size, blank);
        m_del(uint8_t, blank, buffer_size);
        mp_obj_t framebuffer_args[] = {
            buffer,
            args[0],
            args[1],
            args[2],
        };
        mp_obj_t framebuffer = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_framebuf),
            MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);
        return altra_fxbuf_wrap(framebuffer, width, height, stride, format);
    }

    mp_arg_check_num(n_args, n_kw, 4, 5, false);
    mp_obj_t framebuffer = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_framebuf),
        n_args, n_kw, args);
    size_t width = mp_obj_get_int(args[1]);
    size_t height = mp_obj_get_int(args[2]);
    mp_int_t format = mp_obj_get_int(args[3]);
    size_t stride = altra_fxbuf_aligned_stride(
        n_args >= 5 ? (size_t)mp_obj_get_int(args[4]) : width, format);
    return altra_fxbuf_wrap(framebuffer, width, height, stride,
        mp_obj_get_int(args[3]));
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

static uint16_t altra_fxbuf_read_le16(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t altra_fxbuf_read_le32(const uint8_t *data) {
    return (uint32_t)data[0]
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24);
}

static mp_obj_t altra_fxbuf_decode_bmp(const mp_buffer_info_t *raw, size_t *width_out,
    size_t *height_out, size_t *stride_out, uint8_t *format_out) {
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
    size_t stride = width;
    uint64_t row_size = ((uint64_t)width * 2 + 3) & ~UINT64_C(3);
    uint64_t pixel_data_size = row_size * height;
    if (pixel_data_size > raw->len - pixel_offset) {
        mp_raise_ValueError(MP_ERROR_TEXT("truncated BMP pixel data"));
    }

    size_t buffer_size = altra_fxbuf_buffer_size(width, height, stride, 1);
    uint8_t *blank = m_new0(uint8_t, buffer_size);
    mp_obj_t buffer = mp_obj_new_bytearray(buffer_size, blank);
    m_del(uint8_t, blank, buffer_size);

    mp_buffer_info_t decoded;
    mp_get_buffer_raise(buffer, &decoded, MP_BUFFER_WRITE);
    uint8_t *pixels = static_cast<uint8_t *>(decoded.buf);
    for (size_t y = 0; y < height; ++y) {
        size_t source_y = bmp_height > 0 ? height - 1 - y : y;
        const uint8_t *source = bmp + pixel_offset + source_y * row_size;
        memcpy(pixels + y * stride * 2, source, width * 2);
    }

    *width_out = width;
    *height_out = height;
    *stride_out = stride;
    *format_out = 1;
    return buffer;
}

mp_obj_t altra_fxbuf_save(mp_obj_t self_in, mp_obj_t filename_in) {
    mp_obj_t framebuffer_in = altra_fxbuf_get_native(self_in);
    size_t width;
    size_t height;
    size_t stride;
    uint8_t format;
    if (!altra_fxbuf_get_info(self_in, &width, &height, &stride, &format)) {
        mp_raise_TypeError(MP_ERROR_TEXT("save expects an fxbuf or RGB565 FrameBuffer"));
    }

    mp_buffer_info_t raw;
    mp_get_buffer_raise(framebuffer_in, &raw, MP_BUFFER_READ);

    mp_obj_t data_path = altra_fxbuf_make_path(filename_in, ".data");
    mp_obj_t json_path = altra_fxbuf_make_path(filename_in, ".json");
    altra_fxbuf_write_file(data_path, raw.buf, raw.len);

    vstr_t metadata;
    vstr_init(&metadata, 64);
    vstr_printf(&metadata, "{\"width\":%u,\"height\":%u,\"format\":%u,\"stride\":%u}\n",
        (unsigned)width, (unsigned)height, (unsigned)format, (unsigned)stride);
    altra_fxbuf_write_file(json_path, vstr_str(&metadata), vstr_len(&metadata));
    vstr_clear(&metadata);
    return mp_const_none;
}

mp_obj_t altra_fxbuf_load_from_file(mp_obj_t filename_in) {
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
        size_t stride;
        uint8_t format;
        mp_obj_t framebuffer_buffer = altra_fxbuf_decode_bmp(&raw_bmp,
            &width, &height, &stride, &format);
        mp_obj_t framebuffer_args[] = {
            framebuffer_buffer,
            mp_obj_new_int_from_uint(width),
            mp_obj_new_int_from_uint(height),
            mp_obj_new_int_from_uint(format),
            mp_obj_new_int_from_uint(stride),
        };
        mp_obj_t framebuffer = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_framebuf),
            MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);
        return altra_fxbuf_wrap(framebuffer, width, height, stride, format);
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
    size_t format;
    size_t stride;
    bool valid = altra_fxbuf_read_json_uint(json, "width", &width)
        && altra_fxbuf_read_json_uint(json, "height", &height)
        && altra_fxbuf_read_json_uint(json, "format", &format)
        && altra_fxbuf_read_json_uint(json, "stride", &stride);
    m_del(char, json, metadata.len + 1);
    if (!valid || width == 0 || height == 0 || width > UINT16_MAX
        || height > UINT16_MAX || stride < width || stride > UINT16_MAX
        || format > UINT8_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid fxbuf metadata"));
    }

    mp_obj_t framebuffer_args[] = {
        mp_obj_new_bytearray(raw.len, raw.buf),
        mp_obj_new_int_from_uint(width),
        mp_obj_new_int_from_uint(height),
        mp_obj_new_int_from_uint(format),
        mp_obj_new_int_from_uint(stride),
    };
    mp_obj_t framebuffer = mp_call_function_n_kw(MP_OBJ_FROM_PTR(&mp_type_framebuf),
        MP_ARRAY_SIZE(framebuffer_args), 0, framebuffer_args);
    return altra_fxbuf_wrap(framebuffer, width, height, stride, format);
}

}
