extern "C" {
#include "py/objarray.h"
#include "py/objtuple.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "extmod/modmachine.h"
//#include "extmod/modframebuf.h"
}

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "lcd_tft.h"
}

namespace {

struct lcd_tft_display_obj_t {
    mp_obj_base_t base;
    mp_obj_t spi;
    mp_obj_t cs;
    mp_obj_t dc;
    mp_obj_t rst;
    mp_obj_base_t *spi_base;
    void (*spi_transfer)(mp_obj_base_t *, size_t, const uint8_t *, uint8_t *);
    mp_hal_pin_obj_t cs_pin;
    mp_hal_pin_obj_t dc_pin;
    mp_hal_pin_obj_t rst_pin;
    mp_int_t width;
    mp_int_t height;
    mp_int_t x_offset;
    mp_int_t y_offset;
    uint8_t rotation;
    bool offset;
};

struct point_t {
    mp_int_t x;
    mp_int_t y;
};

static lcd_tft_display_obj_t *display_from_obj(mp_obj_t obj) {
    return static_cast<lcd_tft_display_obj_t *>(MP_OBJ_TO_PTR(obj));
}

static mp_obj_t display_call_method(mp_obj_t obj, qstr method, size_t n_args,
                                    mp_obj_t *args) {
    mp_obj_t method_args[18];
    if (n_args > 16) {
        mp_raise_ValueError(MP_ERROR_TEXT("too many method arguments"));
    }
    mp_load_method(obj, method, method_args);
    for (size_t i = 0; i < n_args; ++i) {
        method_args[i + 2] = args[i];
    }
    return mp_call_method_n_kw(n_args, 0, method_args);
}

static inline void display_call_pin(mp_hal_pin_obj_t pin, mp_int_t value) {
    mp_hal_pin_write(pin, value != 0);
}

static void display_pin_init(mp_obj_t pin, mp_int_t value) {
    mp_obj_t init_args[5];
    mp_load_method(pin, MP_QSTR_init, init_args);
    init_args[2] = mp_load_attr(pin, MP_QSTR_OUT);
    init_args[3] = MP_OBJ_NEW_QSTR(MP_QSTR_value);
    init_args[4] = mp_obj_new_int(value);
    mp_call_method_n_kw(1, 1, init_args);
}

static mp_obj_t display_bytes(const uint8_t *data, size_t length) {
    return mp_obj_new_bytes(data, length);
}

static inline void display_transfer(lcd_tft_display_obj_t *self,
                                    const uint8_t *data, size_t length) {
    if (length != 0) {
        self->spi_transfer(self->spi_base, length, data, nullptr);
    }
}

static void display_write_data(lcd_tft_display_obj_t *self, mp_obj_t data) {
    mp_buffer_info_t buf;
    mp_get_buffer_raise(data, &buf, MP_BUFFER_READ);
    display_call_pin(self->dc_pin, 1);
    display_call_pin(self->cs_pin, 0);
    display_transfer(self, static_cast<const uint8_t *>(buf.buf), buf.len);
    display_call_pin(self->cs_pin, 1);
}

static void display_write_cmd(lcd_tft_display_obj_t *self, uint8_t command,
                              const uint8_t *data = nullptr, size_t length = 0) {
    display_call_pin(self->dc_pin, 0);
    display_call_pin(self->cs_pin, 0);
    display_transfer(self, &command, sizeof(command));
    if (length != 0) {
        display_call_pin(self->dc_pin, 1);
        display_transfer(self, data, length);
    }
    display_call_pin(self->cs_pin, 1);
}

static void display_begin_block(lcd_tft_display_obj_t *self, mp_int_t x0,
                                mp_int_t y0, mp_int_t x1, mp_int_t y1) {
    if (self->offset) {
        x0 += self->x_offset;
        x1 += self->x_offset;
        y0 += self->y_offset;
        y1 += self->y_offset;
    }
    uint8_t column[] = {
        static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0),
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)
    };
    uint8_t page[] = {
        static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0),
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)
    };
    display_call_pin(self->cs_pin, 0);
    display_call_pin(self->dc_pin, 0);
    const uint8_t column_command = 0x2a;
    display_transfer(self, &column_command, sizeof(column_command));
    display_call_pin(self->dc_pin, 1);
    display_transfer(self, column, sizeof(column));
    display_call_pin(self->dc_pin, 0);
    const uint8_t page_command = 0x2b;
    display_transfer(self, &page_command, sizeof(page_command));
    display_call_pin(self->dc_pin, 1);
    display_transfer(self, page, sizeof(page));
    display_call_pin(self->dc_pin, 0);
    const uint8_t memory_write_command = 0x2c;
    display_transfer(self, &memory_write_command, sizeof(memory_write_command));
    display_call_pin(self->dc_pin, 1);
}

static void display_send_block(lcd_tft_display_obj_t *self, mp_int_t x0,
                               mp_int_t y0, mp_int_t x1, mp_int_t y1,
                               const uint8_t *data, size_t transfer_length) {
    display_begin_block(self, x0, y0, x1, y1);
    display_transfer(self, data, transfer_length);
    display_call_pin(self->cs_pin, 1);
}

static void display_block(lcd_tft_display_obj_t *self, mp_int_t x0, mp_int_t y0,
                          mp_int_t x1, mp_int_t y1, mp_obj_t data) {
    mp_buffer_info_t buf;
    mp_get_buffer_raise(data, &buf, MP_BUFFER_READ);
    if (x1 < x0 || y1 < y0) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid display block bounds"));
    }
    size_t width = static_cast<size_t>(x1 - x0 + 1);
    size_t height = static_cast<size_t>(y1 - y0 + 1);
    if (height > SIZE_MAX / 2 || width > SIZE_MAX / (height * 2)) {
        mp_raise_ValueError(MP_ERROR_TEXT("block dimensions are too large"));
    }
    size_t transfer_length = width * height * 2;
    if (buf.len < transfer_length) {
        mp_raise_ValueError(MP_ERROR_TEXT("block buffer is too small for its dimensions"));
    }
    display_send_block(self, x0, y0, x1, y1,
                       static_cast<const uint8_t *>(buf.buf), transfer_length);
}

// takes very long time to transfer, but works
static void display_block_framebuf(lcd_tft_display_obj_t *self, mp_int_t x0,
                                   mp_int_t y0, mp_obj_t framebuf_obj) {
#if MICROPY_PY_FRAMEBUF && !MICROPY_ENABLE_DYNRUNTIME
    size_t width, height, stride;
    if (!mp_framebuf_get_rgb565_info(framebuf_obj, &width, &height, &stride)) {
        mp_raise_TypeError(MP_ERROR_TEXT("block expects an RGB565 FrameBuffer"));
    }
    if (width == 0 || height == 0 || stride < width ||
        height > SIZE_MAX / 2 || stride > SIZE_MAX / (height * 2) ||
        width > SIZE_MAX / (height * 2)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid framebuffer dimensions"));
    }

    mp_buffer_info_t source;
    mp_get_buffer_raise(framebuf_obj, &source, MP_BUFFER_READ);
    size_t source_length = ((height - 1) * stride + width) * 2;
    size_t transfer_length = width * height * 2;
    if (source.len < source_length) {
        mp_raise_ValueError(MP_ERROR_TEXT("framebuffer buffer is too small"));
    }

    constexpr size_t scratch_size = 4096;
    mp_obj_t transfer_buffer = mp_obj_new_bytearray(
        transfer_length < scratch_size ? transfer_length : scratch_size, nullptr);
    mp_buffer_info_t destination;
    mp_get_buffer_raise(transfer_buffer, &destination, MP_BUFFER_RW);
    const uint8_t *source_bytes = static_cast<const uint8_t *>(source.buf);
    uint8_t *destination_bytes = static_cast<uint8_t *>(destination.buf);
    size_t pixel_count = width * height;
    size_t max_chunk_pixels = destination.len / 2;
    display_begin_block(self, x0, y0, x0 + width - 1, y0 + height - 1);
    for (size_t first_pixel = 0; first_pixel < pixel_count;) {
        size_t chunk_pixels = pixel_count - first_pixel;
        if (chunk_pixels > max_chunk_pixels) {
            chunk_pixels = max_chunk_pixels;
        }
        for (size_t i = 0; i < chunk_pixels; ++i) {
            size_t pixel = first_pixel + i;
            size_t source_index = ((pixel / width) * stride + pixel % width) * 2;
            destination_bytes[i * 2] = source_bytes[source_index + 1];
            destination_bytes[i * 2 + 1] = source_bytes[source_index];
        }
        display_transfer(self, destination_bytes, chunk_pixels * 2);
        first_pixel += chunk_pixels;
    }
    display_call_pin(self->cs_pin, 1);
#else
    (void)self;
    (void)x0;
    (void)y0;
    (void)framebuf_obj;
    mp_raise_TypeError(MP_ERROR_TEXT("FrameBuffer support is not enabled"));
#endif
}

static bool display_is_off_grid(lcd_tft_display_obj_t *self, mp_int_t xmin,
                                mp_int_t ymin, mp_int_t xmax, mp_int_t ymax) {
    return xmin < 0 || ymin < 0 || xmax >= self->width || ymax >= self->height;
}

static mp_obj_t display_color_bytes(uint16_t color, size_t pixels) {
    size_t length = pixels * 2;
    uint8_t *bytes = static_cast<uint8_t *>(m_new(uint8_t, length));
    for (size_t i = 0; i < pixels; ++i) {
        bytes[2 * i] = static_cast<uint8_t>(color >> 8);
        bytes[2 * i + 1] = static_cast<uint8_t>(color);
    }
    mp_obj_t result = display_bytes(bytes, length);
    m_del(uint8_t, bytes, length);
    return result;
}

static void display_pixel(lcd_tft_display_obj_t *self, mp_int_t x, mp_int_t y,
                          uint16_t color) {
    if (display_is_off_grid(self, x, y, x, y)) {
        return;
    }
    display_block(self, x, y, x, y, display_color_bytes(color, 1));
}

static void display_hline(lcd_tft_display_obj_t *self, mp_int_t x, mp_int_t y,
                          mp_int_t width, uint16_t color) {
    if (width <= 0 || display_is_off_grid(self, x, y, x + width - 1, y)) {
        return;
    }
    display_block(self, x, y, x + width - 1, y,
                  display_color_bytes(color, width));
}

static void display_vline(lcd_tft_display_obj_t *self, mp_int_t x, mp_int_t y,
                          mp_int_t height, uint16_t color) {
    if (height <= 0 || display_is_off_grid(self, x, y, x, y + height - 1)) {
        return;
    }
    display_block(self, x, y, x, y + height - 1,
                  display_color_bytes(color, height));
}

static void display_line(lcd_tft_display_obj_t *self, mp_int_t x0, mp_int_t y0,
                         mp_int_t x1, mp_int_t y1, uint16_t color) {
    if (y0 == y1) {
        if (x0 > x1) { mp_int_t t = x0; x0 = x1; x1 = t; }
        display_hline(self, x0, y0, x1 - x0 + 1, color);
        return;
    }
    if (x0 == x1) {
        if (y0 > y1) { mp_int_t t = y0; y0 = y1; y1 = t; }
        display_vline(self, x0, y0, y1 - y0 + 1, color);
        return;
    }
    if (display_is_off_grid(self, MIN(x0, x1), MIN(y0, y1),
                            MAX(x0, x1), MAX(y0, y1))) {
        return;
    }
    mp_int_t dx = x1 - x0;
    mp_int_t dy = y1 - y0;
    bool steep = labs(dy) > labs(dx);
    if (steep) {
        mp_int_t t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }
    if (x0 > x1) {
        mp_int_t t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    dx = x1 - x0;
    dy = y1 - y0;
    mp_int_t error = dx >> 1;
    mp_int_t ystep = y0 < y1 ? 1 : -1;
    mp_int_t y = y0;
    for (mp_int_t x = x0; x <= x1; ++x) {
        display_pixel(self, steep ? y : x, steep ? x : y, color);
        error -= labs(dy);
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

static void display_fill_hrect(lcd_tft_display_obj_t *self, mp_int_t x,
                               mp_int_t y, mp_int_t width, mp_int_t height,
                               uint16_t color) {
    if (width <= 0 || height <= 0 ||
        display_is_off_grid(self, x, y, x + width - 1, y + height - 1)) {
        return;
    }
    mp_int_t rows_per_chunk = MAX(1, 1024 / width);
    for (mp_int_t row = 0; row < height; row += rows_per_chunk) {
        mp_int_t rows = MIN(rows_per_chunk, height - row);
        display_block(self, x, y + row, x + width - 1, y + row + rows - 1,
                      display_color_bytes(color, width * rows));
    }
}

static void display_fill_vrect(lcd_tft_display_obj_t *self, mp_int_t x,
                               mp_int_t y, mp_int_t width, mp_int_t height,
                               uint16_t color) {
    if (width <= 0 || height <= 0 ||
        display_is_off_grid(self, x, y, x + width - 1, y + height - 1)) {
        return;
    }
    mp_int_t cols_per_chunk = MAX(1, 1024 / height);
    for (mp_int_t col = 0; col < width; col += cols_per_chunk) {
        mp_int_t cols = MIN(cols_per_chunk, width - col);
        display_block(self, x + col, y, x + col + cols - 1, y + height - 1,
                      display_color_bytes(color, cols * height));
    }
}

static void display_circle(lcd_tft_display_obj_t *self, mp_int_t x0,
                           mp_int_t y0, mp_int_t radius, uint16_t color,
                           bool filled) {
    if (radius < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("radius must be non-negative"));
    }
    mp_int_t f = 1 - radius;
    mp_int_t dx = 1;
    mp_int_t dy = -2 * radius;
    mp_int_t x = 0;
    mp_int_t y = radius;
    if (filled) {
        display_vline(self, x0, y0 - radius, 2 * radius + 1, color);
    } else {
        display_pixel(self, x0, y0 + radius, color);
        display_pixel(self, x0, y0 - radius, color);
        display_pixel(self, x0 + radius, y0, color);
        display_pixel(self, x0 - radius, y0, color);
    }
    while (x < y) {
        if (f >= 0) {
            --y;
            dy += 2;
            f += dy;
        }
        ++x;
        dx += 2;
        f += dx;
        if (filled) {
            display_vline(self, x0 + x, y0 - y, 2 * y + 1, color);
            display_vline(self, x0 - x, y0 - y, 2 * y + 1, color);
            display_vline(self, x0 + y, y0 - x, 2 * x + 1, color);
            display_vline(self, x0 - y, y0 - x, 2 * x + 1, color);
        } else {
            display_pixel(self, x0 + x, y0 + y, color);
            display_pixel(self, x0 - x, y0 + y, color);
            display_pixel(self, x0 + x, y0 - y, color);
            display_pixel(self, x0 - x, y0 - y, color);
            display_pixel(self, x0 + y, y0 + x, color);
            display_pixel(self, x0 - y, y0 + x, color);
            display_pixel(self, x0 + y, y0 - x, color);
            display_pixel(self, x0 - y, y0 - x, color);
        }
    }
}

static void display_ellipse(lcd_tft_display_obj_t *self, mp_int_t x0,
                            mp_int_t y0, mp_int_t a, mp_int_t b,
                            uint16_t color, bool filled) {
    if (a < 0 || b < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("ellipse radii must be non-negative"));
    }
    int64_t a2 = static_cast<int64_t>(a) * a;
    int64_t b2 = static_cast<int64_t>(b) * b;
    int64_t twoa2 = 2 * a2;
    int64_t twob2 = 2 * b2;
    mp_int_t x = 0;
    mp_int_t y = b;
    int64_t px = 0;
    int64_t py = twoa2 * y;
    auto plot = [&](mp_int_t xx, mp_int_t yy) {
        if (filled) {
            display_vline(self, x0 + xx, y0 - yy, 2 * yy + 1, color);
            if (xx != 0) display_vline(self, x0 - xx, y0 - yy, 2 * yy + 1, color);
        } else {
            display_pixel(self, x0 + xx, y0 + yy, color);
            display_pixel(self, x0 - xx, y0 + yy, color);
            display_pixel(self, x0 + xx, y0 - yy, color);
            display_pixel(self, x0 - xx, y0 - yy, color);
        }
    };
    plot(0, y);
    int64_t p = llround(static_cast<double>(b2 - a2 * b) + 0.25 * a2);
    while (px < py) {
        ++x;
        px += twob2;
        if (p < 0) p += b2 + px;
        else {
            --y;
            py -= twoa2;
            p += b2 + px - py;
        }
        plot(x, y);
    }
    p = llround(b2 * (x + 0.5) * (x + 0.5) +
                a2 * (y - 1) * (y - 1) - a2 * b2);
    while (y > 0) {
        --y;
        py -= twoa2;
        if (p > 0) p += a2 - py;
        else {
            ++x;
            px += twob2;
            p += a2 - py + px;
        }
        plot(x, y);
    }
}

static mp_obj_t display_open_file(mp_obj_t path) {
    mp_obj_t open_fn = mp_load_global(MP_QSTR_open);
    mp_obj_t mode = mp_obj_new_str("rb", 2);
    mp_obj_t args[] = {path, mode};
    return mp_call_function_n_kw(open_fn, 2, 0, args);
}

static mp_obj_t display_file_read(mp_obj_t file, size_t count) {
    mp_obj_t arg = mp_obj_new_int_from_uint(count);
    return display_call_method(file, MP_QSTR_read, 1, &arg);
}

static void display_file_close(mp_obj_t file) {
    display_call_method(file, MP_QSTR_close, 0, nullptr);
}

static mp_obj_t display_make_new(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *args) {
    enum {
        ARG_spi, ARG_cs, ARG_dc, ARG_rst, ARG_width, ARG_height, ARG_rotation,
        ARG_mirror, ARG_bgr, ARG_gamma, ARG_x_offset, ARG_y_offset
    };
    const mp_arg_t allowed_args[] = {
        {static_cast<uint16_t>(qstr_from_str("spi")), MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {static_cast<uint16_t>(qstr_from_str("cs")), MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_dc, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_rst, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_width, MP_ARG_INT, {.u_int = 240}},
        {MP_QSTR_height, MP_ARG_INT, {.u_int = 320}},
        {MP_QSTR_rotation, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_mirror, MP_ARG_BOOL, {.u_bool = false}},
        {MP_QSTR_bgr, MP_ARG_BOOL, {.u_bool = true}},
        {MP_QSTR_gamma, MP_ARG_BOOL, {.u_bool = true}},
        {MP_QSTR_x_offset, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_y_offset, MP_ARG_INT, {.u_int = 0}},
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args),
                              allowed_args, values);
    mp_int_t rotation_deg = values[ARG_rotation].u_int;
    if (rotation_deg != 0 && rotation_deg != 90 &&
        rotation_deg != 180 && rotation_deg != 270) {
        mp_raise_ValueError(MP_ERROR_TEXT("rotation must be 0, 90, 180 or 270"));
    }
    static const uint8_t rotation_map[2][4] = {
        {0x80, 0xe0, 0x40, 0x20},
        {0xc0, 0x60, 0x00, 0xa0},
    };
    mp_int_t rotation_index = rotation_deg / 90;
    uint8_t rotation = rotation_map[values[ARG_mirror].u_bool][rotation_index];
    if (values[ARG_bgr].u_bool) rotation |= 0x08;

    lcd_tft_display_obj_t *self = mp_obj_malloc(lcd_tft_display_obj_t, type);
    self->spi = values[ARG_spi].u_obj;
    self->cs = values[ARG_cs].u_obj;
    self->dc = values[ARG_dc].u_obj;
    self->rst = values[ARG_rst].u_obj;
    self->cs_pin = mp_hal_get_pin_obj(self->cs);
    self->dc_pin = mp_hal_get_pin_obj(self->dc);
    self->rst_pin = mp_hal_get_pin_obj(self->rst);
    self->spi_base = static_cast<mp_obj_base_t *>(MP_OBJ_TO_PTR(self->spi));
    bool is_spi = mp_obj_is_type(self->spi, &machine_spi_type);
#if MICROPY_PY_MACHINE_SOFTSPI
    is_spi = is_spi || mp_obj_is_type(self->spi, &mp_machine_soft_spi_type);
#endif
    if (!is_spi) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected an SPI object"));
    }
    const mp_machine_spi_p_t *spi_protocol =
        static_cast<const mp_machine_spi_p_t *>(
            MP_OBJ_TYPE_GET_SLOT(self->spi_base->type, protocol));
    if (spi_protocol == nullptr || spi_protocol->transfer == nullptr) {
        mp_raise_TypeError(MP_ERROR_TEXT("SPI object does not support transfers"));
    }
    self->spi_transfer = spi_protocol->transfer;
    self->width = values[ARG_width].u_int;
    self->height = values[ARG_height].u_int;
    self->x_offset = values[ARG_x_offset].u_int;
    self->y_offset = values[ARG_y_offset].u_int;
    self->offset = self->x_offset != 0 || self->y_offset != 0;
    self->rotation = rotation;

    if (self->width <= 0 || self->height <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("display dimensions must be positive"));
    }
    display_pin_init(self->cs, 1);
    display_pin_init(self->dc, 0);
    display_pin_init(self->rst, 1);
    display_call_pin(self->rst_pin, 0);
    mp_hal_delay_ms(50);
    display_call_pin(self->rst_pin, 1);
    mp_hal_delay_ms(50);

    display_write_cmd(self, 0x01);
    mp_hal_delay_ms(100);
    const uint8_t pwctrb[] = {0x00, 0xc1, 0x30};
    const uint8_t posc[] = {0x64, 0x03, 0x12, 0x81};
    const uint8_t dtca[] = {0x85, 0x00, 0x78};
    const uint8_t pwctra[] = {0x39, 0x2c, 0x00, 0x34, 0x02};
    const uint8_t dtcb[] = {0x00, 0x00};
    const uint8_t vmctr1[] = {0x3e, 0x28};
    const uint8_t frmctr1[] = {0x00, 0x18};
    const uint8_t dfunctr[] = {0x08, 0x82, 0x27};
    display_write_cmd(self, 0xcf, pwctrb, sizeof(pwctrb));
    display_write_cmd(self, 0xed, posc, sizeof(posc));
    display_write_cmd(self, 0xe8, dtca, sizeof(dtca));
    display_write_cmd(self, 0xcb, pwctra, sizeof(pwctra));
    const uint8_t pumprc[] = {0x20};
    display_write_cmd(self, 0xf7, pumprc, sizeof(pumprc));
    display_write_cmd(self, 0xea, dtcb, sizeof(dtcb));
    const uint8_t pwctr1[] = {0x23};
    const uint8_t pwctr2[] = {0x10};
    display_write_cmd(self, 0xc0, pwctr1, sizeof(pwctr1));
    display_write_cmd(self, 0xc1, pwctr2, sizeof(pwctr2));
    display_write_cmd(self, 0xc5, vmctr1, sizeof(vmctr1));
    const uint8_t vmctr2[] = {0x86};
    display_write_cmd(self, 0xc7, vmctr2, sizeof(vmctr2));
    display_write_cmd(self, 0x36, &self->rotation, 1);
    const uint8_t zero[] = {0x00};
    const uint8_t pixfmt[] = {0x55};
    display_write_cmd(self, 0x37, zero, sizeof(zero));
    display_write_cmd(self, 0x3a, pixfmt, sizeof(pixfmt));
    display_write_cmd(self, 0xb1, frmctr1, sizeof(frmctr1));
    display_write_cmd(self, 0xb6, dfunctr, sizeof(dfunctr));
    display_write_cmd(self, 0xf2, zero, sizeof(zero));
    const uint8_t gamma_set[] = {0x01};
    display_write_cmd(self, 0x26, gamma_set, sizeof(gamma_set));
    if (values[ARG_gamma].u_bool) {
        const uint8_t positive[] = {
            0x0f, 0x31, 0x2b, 0x0c, 0x0e, 0x08, 0x4e, 0xf1,
            0x37, 0x07, 0x10, 0x03, 0x0e, 0x09, 0x00
        };
        const uint8_t negative[] = {
            0x00, 0x0e, 0x14, 0x03, 0x11, 0x07, 0x31, 0xc1,
            0x48, 0x08, 0x0f, 0x0c, 0x31, 0x36, 0x0f
        };
        display_write_cmd(self, 0xe0, positive, sizeof(positive));
        display_write_cmd(self, 0xe1, negative, sizeof(negative));
    }
    display_write_cmd(self, 0x11);
    mp_hal_delay_ms(100);
    display_write_cmd(self, 0x29);
    mp_hal_delay_ms(100);
    display_fill_hrect(self, 0, 0, self->width, self->height, 0);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t display_block_method(size_t n_args, const mp_obj_t *args) {
    if (n_args == 4) {
        display_block_framebuf(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                               mp_obj_get_int(args[2]), args[3]);
    } else if (n_args == 6) {
        display_block(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                      mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                      mp_obj_get_int(args[4]), args[5]);
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT(
            "block expects (x0, y0, x1, y1, data) or (x0, y0, framebuffer)"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_block_obj, 4, 6, display_block_method);

static mp_obj_t display_clear_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 1 || n_args > 3) mp_raise_TypeError(MP_ERROR_TEXT("clear expects 0 to 2 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    uint16_t color = n_args > 1 ? mp_obj_get_int(args[1]) : 0;
    mp_int_t hlines = n_args > 2 ? mp_obj_get_int(args[2]) : 8;
    if (hlines <= 0 || self->height % hlines != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("hlines must be a non-zero factor of height"));
    }
    mp_obj_t line = display_color_bytes(color, self->width * hlines);
    for (mp_int_t y = 0; y < self->height; y += hlines) {
        display_block(self, 0, y, self->width - 1, y + hlines - 1, line);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_clear_obj, 1, 3, display_clear_method);

static mp_obj_t display_one_command(mp_obj_t self_in, uint8_t command) {
    display_write_cmd(display_from_obj(self_in), command);
    return mp_const_none;
}
#define DISPLAY_CMD_METHOD(fn, cmd) \
    static mp_obj_t fn(mp_obj_t self) { return display_one_command(self, cmd); } \
    static MP_DEFINE_CONST_FUN_OBJ_1(fn##_obj, fn)
DISPLAY_CMD_METHOD(display_on_method, 0x29);
DISPLAY_CMD_METHOD(display_off_method, 0x28);

static mp_obj_t display_cleanup_method(mp_obj_t self_in) {
    lcd_tft_display_obj_t *self = display_from_obj(self_in);
    display_fill_hrect(self, 0, 0, self->width, self->height, 0);
    display_write_cmd(self, 0x28);
    display_call_method(self->spi, MP_QSTR_deinit, 0, nullptr);
    mp_printf(&mp_plat_print, "display off\n");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(display_cleanup_obj, display_cleanup_method);

static mp_obj_t display_draw_pixel_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 4) mp_raise_TypeError(MP_ERROR_TEXT("draw_pixel expects 3 arguments"));
    display_pixel(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                  mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_pixel_obj, 4, 4, display_draw_pixel_method);

static mp_obj_t display_draw_hline_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 5) mp_raise_TypeError(MP_ERROR_TEXT("draw_hline expects 4 arguments"));
    display_hline(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                  mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                  mp_obj_get_int(args[4]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_hline_obj, 5, 5, display_draw_hline_method);

static mp_obj_t display_draw_vline_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 5) mp_raise_TypeError(MP_ERROR_TEXT("draw_vline expects 4 arguments"));
    display_vline(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                  mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                  mp_obj_get_int(args[4]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_vline_obj, 5, 5, display_draw_vline_method);

static mp_obj_t display_draw_line_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("draw_line expects 5 arguments"));
    display_line(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                 mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                 mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_line_obj, 6, 6, display_draw_line_method);

static mp_obj_t display_draw_rectangle_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("draw_rectangle expects 5 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]), y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]), h = mp_obj_get_int(args[4]);
    uint16_t color = mp_obj_get_int(args[5]);
    display_hline(self, x, y, w, color);
    display_hline(self, x, y + h - 1, w, color);
    display_vline(self, x, y, h, color);
    display_vline(self, x + w - 1, y, h, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_rectangle_obj, 6, 6, display_draw_rectangle_method);

static mp_obj_t display_fill_rectangle_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("fill_rectangle expects 5 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]), y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]), h = mp_obj_get_int(args[4]);
    uint16_t color = mp_obj_get_int(args[5]);
    if (w > h) display_fill_hrect(self, x, y, w, h, color);
    else display_fill_vrect(self, x, y, w, h, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_rectangle_obj, 6, 6, display_fill_rectangle_method);

static mp_obj_t display_fill_hrect_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("fill_hrect expects 5 arguments"));
    display_fill_hrect(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                       mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                       mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_hrect_obj, 6, 6, display_fill_hrect_method);

static mp_obj_t display_fill_vrect_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("fill_vrect expects 5 arguments"));
    display_fill_vrect(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                       mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                       mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_vrect_obj, 6, 6, display_fill_vrect_method);

static mp_obj_t display_draw_circle_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 5) mp_raise_TypeError(MP_ERROR_TEXT("draw_circle expects 4 arguments"));
    display_circle(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                   mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                   mp_obj_get_int(args[4]), false);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_circle_obj, 5, 5, display_draw_circle_method);

static mp_obj_t display_fill_circle_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 5) mp_raise_TypeError(MP_ERROR_TEXT("fill_circle expects 4 arguments"));
    display_circle(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                   mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                   mp_obj_get_int(args[4]), true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_circle_obj, 5, 5, display_fill_circle_method);

static mp_obj_t display_draw_ellipse_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("draw_ellipse expects 5 arguments"));
    display_ellipse(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                    mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                    mp_obj_get_int(args[4]), mp_obj_get_int(args[5]), false);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_ellipse_obj, 6, 6, display_draw_ellipse_method);

static mp_obj_t display_fill_ellipse_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("fill_ellipse expects 5 arguments"));
    display_ellipse(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                    mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                    mp_obj_get_int(args[4]), mp_obj_get_int(args[5]), true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_ellipse_obj, 6, 6, display_fill_ellipse_method);

static void display_polygon(lcd_tft_display_obj_t *self, mp_int_t sides,
                            mp_int_t x0, mp_int_t y0, mp_int_t radius,
                            uint16_t color, mp_float_t rotate, bool filled) {
    if (sides < 3 || radius < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("polygon requires at least 3 sides and non-negative radius"));
    }
    if (sides > 2048) {
        mp_raise_ValueError(MP_ERROR_TEXT("too many polygon sides"));
    }
    size_t count = static_cast<size_t>(sides) + 1;
    point_t *points = static_cast<point_t *>(m_new(point_t, count));
    const double pi = 3.14159265358979323846;
    double theta = static_cast<double>(rotate) * (pi / 180.0);
    for (mp_int_t i = 0; i <= sides; ++i) {
        double angle = 2.0 * pi * i / sides + theta;
        points[i].x = static_cast<mp_int_t>(radius * cos(angle) + x0);
        points[i].y = static_cast<mp_int_t>(radius * sin(angle) + y0);
    }
    if (!filled) {
        for (mp_int_t i = 0; i < sides; ++i) {
            display_line(self, points[i].x, points[i].y,
                         points[i + 1].x, points[i + 1].y, color);
        }
    } else {
        mp_int_t min_y = points[0].y, max_y = points[0].y;
        for (mp_int_t i = 1; i < sides; ++i) {
            min_y = MIN(min_y, points[i].y);
            max_y = MAX(max_y, points[i].y);
        }
        min_y = MAX(min_y, 0);
        max_y = MIN(max_y, self->height - 1);
        if (min_y <= max_y) {
            mp_int_t *intersections = static_cast<mp_int_t *>(m_new(mp_int_t, sides));
            for (mp_int_t y = min_y; y <= max_y; ++y) {
                mp_int_t count_intersections = 0;
                for (mp_int_t i = 0; i < sides; ++i) {
                    point_t a = points[i], b = points[i + 1];
                    if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
                        intersections[count_intersections++] =
                            a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
                    }
                }
                for (mp_int_t i = 1; i < count_intersections; ++i) {
                    mp_int_t value = intersections[i], j = i;
                    while (j > 0 && intersections[j - 1] > value) {
                        intersections[j] = intersections[j - 1];
                        --j;
                    }
                    intersections[j] = value;
                }
                for (mp_int_t i = 0; i + 1 < count_intersections; i += 2) {
                    display_hline(self, intersections[i], y,
                                  intersections[i + 1] - intersections[i] + 1, color);
                }
            }
            m_del(mp_int_t, intersections, sides);
        }
    }
    m_del(point_t, points, count);
}

static mp_obj_t display_draw_polygon_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 6 || n_args > 7) mp_raise_TypeError(MP_ERROR_TEXT("draw_polygon expects 5 or 6 arguments"));
    display_polygon(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                    mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                    mp_obj_get_int(args[4]), mp_obj_get_int(args[5]),
                    n_args == 7 ? mp_obj_get_float(args[6]) : 0, false);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_polygon_obj, 6, 7, display_draw_polygon_method);

static mp_obj_t display_fill_polygon_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 6 || n_args > 7) mp_raise_TypeError(MP_ERROR_TEXT("fill_polygon expects 5 or 6 arguments"));
    display_polygon(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                    mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                    mp_obj_get_int(args[4]), mp_obj_get_int(args[5]),
                    n_args == 7 ? mp_obj_get_float(args[6]) : 0, true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_fill_polygon_obj, 6, 7, display_fill_polygon_method);

static mp_obj_t display_draw_lines_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 3) mp_raise_TypeError(MP_ERROR_TEXT("draw_lines expects 2 arguments"));
    size_t count;
    mp_obj_t *coords;
    mp_obj_get_array(args[1], &count, &coords);
    if (count < 2) return mp_const_none;
    mp_obj_t *first;
    size_t first_len;
    mp_obj_get_array(coords[0], &first_len, &first);
    if (first_len != 2) mp_raise_ValueError(MP_ERROR_TEXT("each coordinate must contain x and y"));
    mp_int_t x0 = mp_obj_get_int(first[0]), y0 = mp_obj_get_int(first[1]);
    uint16_t color = mp_obj_get_int(args[2]);
    for (size_t i = 1; i < count; ++i) {
        mp_obj_t *point;
        size_t point_len;
        mp_obj_get_array(coords[i], &point_len, &point);
        if (point_len != 2) mp_raise_ValueError(MP_ERROR_TEXT("each coordinate must contain x and y"));
        mp_int_t x1 = mp_obj_get_int(point[0]), y1 = mp_obj_get_int(point[1]);
        display_line(display_from_obj(args[0]), x0, y0, x1, y1, color);
        x0 = x1;
        y0 = y1;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_lines_obj, 3, 3, display_draw_lines_method);

static mp_obj_t display_draw_letter_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 6 || n_args > 9) mp_raise_TypeError(MP_ERROR_TEXT("draw_letter expects 5 to 8 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]), y = mp_obj_get_int(args[2]);
    uint16_t color = mp_obj_get_int(args[5]);
    uint16_t background = n_args > 6 ? mp_obj_get_int(args[6]) : 0;
    bool landscape = n_args > 7 && mp_obj_is_true(args[7]);
    bool rotate = n_args > 8 && mp_obj_is_true(args[8]);
    mp_obj_t font_args[] = {args[3], mp_obj_new_int(color),
                           mp_obj_new_int(background), mp_obj_new_bool(landscape)};
    mp_obj_t result = display_call_method(args[4], MP_QSTR_get_letter, 4, font_args);
    mp_obj_t *tuple;
    mp_obj_get_array_fixed_n(result, 3, &tuple);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(tuple[0], &buf, MP_BUFFER_READ);
    mp_int_t width = mp_obj_get_int(tuple[1]), height = mp_obj_get_int(tuple[2]);
    if (width == 0) {
        mp_obj_t dims[] = {mp_obj_new_int(width), mp_obj_new_int(height)};
        return mp_obj_new_tuple(2, dims);
    }
    mp_obj_t data = tuple[0];
    if (rotate) {
        uint8_t *reversed = static_cast<uint8_t *>(m_new(uint8_t, buf.len));
        for (size_t i = 0; i + 1 < buf.len; i += 2) {
            reversed[buf.len - i - 2] = static_cast<const uint8_t *>(buf.buf)[i];
            reversed[buf.len - i - 1] = static_cast<const uint8_t *>(buf.buf)[i + 1];
        }
        data = display_bytes(reversed, buf.len);
        m_del(uint8_t, reversed, buf.len);
    }
    if (landscape) {
        y -= width;
        if (!display_is_off_grid(self, x, y, x + height - 1, y + width - 1)) {
            display_block(self, x, y, x + height - 1, y + width - 1, data);
        } else {
            width = 0;
            height = 0;
        }
    } else if (!display_is_off_grid(self, x, y, x + width - 1, y + height - 1)) {
        display_block(self, x, y, x + width - 1, y + height - 1, data);
    } else {
        width = 0;
        height = 0;
    }
    mp_obj_t dims[] = {mp_obj_new_int(width), mp_obj_new_int(height)};
    return mp_obj_new_tuple(2, dims);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_letter_obj, 6, 9, display_draw_letter_method);

static mp_obj_t display_draw_text_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 6 || n_args > 10) mp_raise_TypeError(MP_ERROR_TEXT("draw_text expects 5 to 9 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]), y = mp_obj_get_int(args[2]);
    size_t text_len;
    const char *text = mp_obj_str_get_data(args[3], &text_len);
    uint16_t color = mp_obj_get_int(args[5]);
    uint16_t background = n_args > 6 ? mp_obj_get_int(args[6]) : 0;
    bool landscape = n_args > 7 && mp_obj_is_true(args[7]);
    bool rotate = n_args > 8 && mp_obj_is_true(args[8]);
    mp_int_t spacing = n_args > 9 ? mp_obj_get_int(args[9]) : 1;
    for (size_t i = 0; i < text_len; ++i) {
        size_t letter_index = rotate ? text_len - i - 1 : i;
        mp_obj_t letter = mp_obj_new_bytes(reinterpret_cast<const uint8_t *>(text + letter_index), 1);
        mp_obj_t call_args[] = {args[0], mp_obj_new_int(x), mp_obj_new_int(y),
            letter, args[4], mp_obj_new_int(color), mp_obj_new_int(background),
            mp_obj_new_bool(landscape), mp_obj_new_bool(rotate)};
        mp_obj_t dims = display_draw_letter_method(9, call_args);
        mp_obj_t *wh;
        size_t wh_len;
        mp_obj_get_array(dims, &wh_len, &wh);
        mp_int_t char_w = mp_obj_get_int(wh[0]), char_h = mp_obj_get_int(wh[1]);
        if (char_w == 0 || char_h == 0) return mp_const_none;
        if (landscape) {
            if (spacing) display_fill_hrect(self, x, y - char_w - spacing,
                                             char_h, spacing, background);
            y -= char_w + spacing;
        } else {
            if (spacing) display_fill_hrect(self, x + char_w, y, spacing,
                                             char_h, background);
            x += char_w + spacing;
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_text_obj, 6, 10, display_draw_text_method);

static mp_obj_t display_write_data_mpy_method(mp_obj_t self_in, mp_obj_t data) {
    display_write_data(display_from_obj(self_in), data);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(display_write_data_mpy_obj, display_write_data_mpy_method);

static mp_obj_t display_write_cmd_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 2 || n_args > 17) {
        mp_raise_TypeError(MP_ERROR_TEXT("write_cmd_mpy expects a command and at most 15 data bytes"));
    }
    uint8_t data[15];
    for (size_t i = 2; i < n_args; ++i) {
        data[i - 2] = mp_obj_get_int(args[i]);
    }
    display_write_cmd(display_from_obj(args[0]), mp_obj_get_int(args[1]),
                      data, n_args - 2);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_write_cmd_obj, 2, 17, display_write_cmd_method);

static mp_obj_t display_draw_text8x8_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 5 || n_args > 7) {
        mp_raise_TypeError(MP_ERROR_TEXT("draw_text8x8 expects 4 to 6 arguments"));
    }
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]), y = mp_obj_get_int(args[2]);
    size_t text_len;
    mp_obj_str_get_data(args[3], &text_len);
    mp_int_t color = mp_obj_get_int(args[4]);
    mp_int_t background = n_args > 5 ? mp_obj_get_int(args[5]) : 0;
    mp_int_t rotate = n_args > 6 ? mp_obj_get_int(args[6]) : 0;
    if (rotate != 0 && rotate != 90 && rotate != 180 && rotate != 270) {
        mp_raise_ValueError(MP_ERROR_TEXT("rotate must be 0, 90, 180 or 270"));
    }
    mp_int_t width = text_len * 8;
    if (width == 0 || display_is_off_grid(self, x, y, x + 7, y + 7)) {
        return mp_const_none;
    }
    size_t size = static_cast<size_t>(width) * 8 * 2;
    mp_obj_t buffer = mp_obj_new_bytearray(size, nullptr);
    mp_buffer_info_t buffer_info;
    mp_get_buffer_raise(buffer, &buffer_info, MP_BUFFER_RW);
    memset(buffer_info.buf, 0, buffer_info.len);

    mp_obj_t framebuf = mp_import_name(MP_QSTR_framebuf, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_obj_t framebuf_type = mp_load_attr(framebuf, MP_QSTR_FrameBuffer);
    mp_obj_t pixel_format = mp_load_attr(framebuf, MP_QSTR_RGB565);
    mp_obj_t ctor_args[] = {buffer, mp_obj_new_int(width), mp_obj_new_int(8), pixel_format};
    mp_obj_t fb = mp_call_function_n_kw(framebuf_type, 4, 0, ctor_args);
    auto call_fb = [](mp_obj_t object, qstr method, size_t n, mp_obj_t *method_args) {
        return display_call_method(object, method, n, method_args);
    };
    auto swapped_color = [](mp_int_t value) {
        return mp_obj_new_int(((value & 0xff) << 8) | ((value & 0xff00) >> 8));
    };
    if (background != 0) {
        mp_obj_t fill_args[] = {swapped_color(background)};
        call_fb(fb, MP_QSTR_fill, 1, fill_args);
    }
    mp_obj_t text_args[] = {args[3], mp_obj_new_int(0), mp_obj_new_int(0), swapped_color(color)};
    call_fb(fb, MP_QSTR_text, 4, text_args);

    if (rotate == 0) {
        display_block(self, x, y, x + width - 1, y + 7, buffer);
        return mp_const_none;
    }
    mp_int_t out_width = (rotate == 90 || rotate == 270) ? 8 : width;
    mp_int_t out_height = (rotate == 90 || rotate == 270) ? width : 8;
    mp_obj_t out_buffer = mp_obj_new_bytearray(size, nullptr);
    mp_buffer_info_t out_info;
    mp_get_buffer_raise(out_buffer, &out_info, MP_BUFFER_RW);
    memset(out_info.buf, 0, out_info.len);
    mp_obj_t out_ctor_args[] = {out_buffer, mp_obj_new_int(out_width),
                                mp_obj_new_int(out_height), pixel_format};
    mp_obj_t out_fb = mp_call_function_n_kw(framebuf_type, 4, 0, out_ctor_args);
    for (mp_int_t py = 0; py < 8; ++py) {
        for (mp_int_t px = 0; px < width; ++px) {
            mp_obj_t get_args[] = {mp_obj_new_int(px), mp_obj_new_int(py)};
            mp_obj_t pixel = call_fb(fb, MP_QSTR_pixel, 2, get_args);
            mp_int_t dx, dy;
            if (rotate == 90) { dx = py; dy = width - 1 - px; }
            else if (rotate == 180) { dx = width - 1 - px; dy = 7 - py; }
            else { dx = 7 - py; dy = px; }
            mp_obj_t set_args[] = {mp_obj_new_int(dx), mp_obj_new_int(dy), pixel};
            call_fb(out_fb, MP_QSTR_pixel, 3, set_args);
        }
    }
    display_block(self, x, y, x + out_width - 1, y + out_height - 1, out_buffer);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_text8x8_obj, 5, 7, display_draw_text8x8_method);

static void display_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    lcd_tft_display_obj_t *self = display_from_obj(self_in);
    const char *name = qstr_str(attr);
    if (dest[0] == MP_OBJ_NULL) {
        if (strcmp(name, "spi") == 0) dest[0] = self->spi;
        else if (strcmp(name, "cs") == 0) dest[0] = self->cs;
        else if (attr == MP_QSTR_dc) dest[0] = self->dc;
        else if (attr == MP_QSTR_rst) dest[0] = self->rst;
        else if (strcmp(name, "block") == 0) {
            dest[0] = MP_OBJ_FROM_PTR(&display_block_obj);
            dest[1] = self_in;
        }
        else if (attr == MP_QSTR_width) dest[0] = mp_obj_new_int(self->width);
        else if (attr == MP_QSTR_height) dest[0] = mp_obj_new_int(self->height);
        else if (attr == MP_QSTR_rotation) dest[0] = mp_obj_new_int(self->rotation);
        else if (attr == MP_QSTR_x_offset) dest[0] = mp_obj_new_int(self->x_offset);
        else if (attr == MP_QSTR_y_offset) dest[0] = mp_obj_new_int(self->y_offset);
        if (dest[0] == MP_OBJ_NULL) {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        if (attr == MP_QSTR_width) self->width = mp_obj_get_int(dest[1]);
        else if (attr == MP_QSTR_height) self->height = mp_obj_get_int(dest[1]);
        else if (attr == MP_QSTR_x_offset) self->x_offset = mp_obj_get_int(dest[1]);
        else if (attr == MP_QSTR_y_offset) self->y_offset = mp_obj_get_int(dest[1]);
        else return;
        if (self->width <= 0 || self->height <= 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("display dimensions must be positive"));
        }
        self->offset = self->x_offset != 0 || self->y_offset != 0;
        dest[0] = MP_OBJ_NULL;
    }
}

static mp_obj_t display_is_off_grid_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 5) mp_raise_TypeError(MP_ERROR_TEXT("is_off_grid expects 4 arguments"));
    bool outside = display_is_off_grid(display_from_obj(args[0]),
        mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_obj_new_bool(outside);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_is_off_grid_obj, 5, 5, display_is_off_grid_method);

static mp_obj_t display_draw_sprite_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 6) mp_raise_TypeError(MP_ERROR_TEXT("draw_sprite expects 5 arguments"));
    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[1], &buf, MP_BUFFER_READ);
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = mp_obj_get_int(args[2]), y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]), h = mp_obj_get_int(args[5]);
    if (w > 0 && h > 0 && !display_is_off_grid(self, x, y, x + w - 1, y + h - 1)) {
        display_block(self, x, y, x + w - 1, y + h - 1, args[1]);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_sprite_obj, 6, 6, display_draw_sprite_method);

static mp_obj_t display_draw_image_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 2 || n_args > 6) mp_raise_TypeError(MP_ERROR_TEXT("draw_image expects 1 to 5 arguments"));
    lcd_tft_display_obj_t *self = display_from_obj(args[0]);
    mp_int_t x = n_args > 2 ? mp_obj_get_int(args[2]) : 0;
    mp_int_t y = n_args > 3 ? mp_obj_get_int(args[3]) : 0;
    mp_int_t w = n_args > 4 ? mp_obj_get_int(args[4]) : 320;
    mp_int_t h = n_args > 5 ? mp_obj_get_int(args[5]) : 240;
    if (w <= 0 || h <= 0 || display_is_off_grid(self, x, y, x + w - 1, y + h - 1)) {
        return mp_const_none;
    }
    mp_obj_t file = display_open_file(args[1]);
    mp_int_t chunk_height = MAX(1, 1024 / w);
    mp_int_t chunk_y = y;
    while (chunk_y < y + h) {
        mp_int_t rows = MIN(chunk_height, y + h - chunk_y);
        mp_obj_t data = display_file_read(file, rows * w * 2);
        mp_buffer_info_t buf;
        mp_get_buffer_raise(data, &buf, MP_BUFFER_READ);
        if (buf.len != static_cast<size_t>(rows * w * 2)) {
            display_file_close(file);
            mp_raise_OSError(MP_EIO);
        }
        display_block(self, x, chunk_y, x + w - 1, chunk_y + rows - 1, data);
        chunk_y += rows;
    }
    display_file_close(file);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_draw_image_obj, 2, 6, display_draw_image_method);

static mp_obj_t display_load_sprite_method(size_t n_args, const mp_obj_t *args) {
    if (n_args != 4) mp_raise_TypeError(MP_ERROR_TEXT("load_sprite expects 3 arguments"));
    mp_int_t w = mp_obj_get_int(args[2]), h = mp_obj_get_int(args[3]);
    if (w < 0 || h < 0 || static_cast<size_t>(w) > SIZE_MAX / 2 / MAX(1, h)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid sprite dimensions"));
    }
    mp_obj_t file = display_open_file(args[1]);
    mp_obj_t data = display_file_read(file, static_cast<size_t>(w) * h * 2);
    display_file_close(file);
    return data;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_load_sprite_obj, 4, 4, display_load_sprite_method);

static mp_obj_t display_scroll_method(mp_obj_t self_in, mp_obj_t y_obj) {
    mp_int_t y = mp_obj_get_int(y_obj);
    uint8_t data[] = {static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y)};
    display_write_cmd(display_from_obj(self_in), 0x37, data, sizeof(data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(display_scroll_obj, display_scroll_method);

static mp_obj_t display_set_scroll_method(mp_obj_t self_in, mp_obj_t top_obj,
                                          mp_obj_t bottom_obj) {
    lcd_tft_display_obj_t *self = display_from_obj(self_in);
    mp_int_t top = mp_obj_get_int(top_obj), bottom = mp_obj_get_int(bottom_obj);
    if (top + bottom <= self->height) {
        mp_int_t middle = self->height - top - bottom;
        uint8_t data[] = {
            static_cast<uint8_t>(top >> 8), static_cast<uint8_t>(top),
            static_cast<uint8_t>(middle >> 8), static_cast<uint8_t>(middle),
            static_cast<uint8_t>(bottom >> 8), static_cast<uint8_t>(bottom)
        };
        display_write_cmd(self, 0x33, data, sizeof(data));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(display_set_scroll_obj, display_set_scroll_method);

static mp_obj_t display_invert_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 1 || n_args > 2) mp_raise_TypeError(MP_ERROR_TEXT("invert expects 0 or 1 argument"));
    bool enable = n_args == 1 || mp_obj_is_true(args[1]);
    display_write_cmd(display_from_obj(args[0]), enable ? 0x21 : 0x20);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_invert_obj, 1, 2, display_invert_method);

static mp_obj_t display_sleep_method(size_t n_args, const mp_obj_t *args) {
    if (n_args < 1 || n_args > 2) mp_raise_TypeError(MP_ERROR_TEXT("sleep expects 0 or 1 argument"));
    bool enable = n_args == 1 || mp_obj_is_true(args[1]);
    display_write_cmd(display_from_obj(args[0]), enable ? 0x10 : 0x11);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_sleep_obj, 1, 2, display_sleep_method);

static mp_obj_t display_reset_mpy_method(mp_obj_t self_in) {
    lcd_tft_display_obj_t *self = display_from_obj(self_in);
    display_call_pin(self->rst_pin, 0);
    mp_hal_delay_ms(50);
    display_call_pin(self->rst_pin, 1);
    mp_hal_delay_ms(50);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(display_reset_mpy_obj, display_reset_mpy_method);
static mp_obj_t display_reset_cpy_method(mp_obj_t self_in) {
    lcd_tft_display_obj_t *self = display_from_obj(self_in);
    mp_store_attr(self->rst, MP_QSTR_value, mp_const_false);
    mp_hal_delay_ms(50);
    mp_store_attr(self->rst, MP_QSTR_value, mp_const_true);
    mp_hal_delay_ms(50);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(display_reset_cpy_obj, display_reset_cpy_method);

static const mp_rom_map_elem_t display_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_NOP), MP_ROM_INT(0x00)},
    {MP_ROM_QSTR(MP_QSTR_SWRESET), MP_ROM_INT(0x01)},
    {MP_ROM_QSTR(MP_QSTR_RDDID), MP_ROM_INT(0x04)},
    {MP_ROM_QSTR(MP_QSTR_RDDST), MP_ROM_INT(0x09)},
    {MP_ROM_QSTR(MP_QSTR_SLPIN), MP_ROM_INT(0x10)},
    {MP_ROM_QSTR(MP_QSTR_SLPOUT), MP_ROM_INT(0x11)},
    {MP_ROM_QSTR(MP_QSTR_PTLON), MP_ROM_INT(0x12)},
    {MP_ROM_QSTR(MP_QSTR_NORON), MP_ROM_INT(0x13)},
    {MP_ROM_QSTR(MP_QSTR_RDMODE), MP_ROM_INT(0x0a)},
    {MP_ROM_QSTR(MP_QSTR_RDMADCTL), MP_ROM_INT(0x0b)},
    {MP_ROM_QSTR(MP_QSTR_RDPIXFMT), MP_ROM_INT(0x0c)},
    {MP_ROM_QSTR(MP_QSTR_RDIMGFMT), MP_ROM_INT(0x0d)},
    {MP_ROM_QSTR(MP_QSTR_RDSELFDIAG), MP_ROM_INT(0x0f)},
    {MP_ROM_QSTR(MP_QSTR_INVOFF), MP_ROM_INT(0x20)},
    {MP_ROM_QSTR(MP_QSTR_INVON), MP_ROM_INT(0x21)},
    {MP_ROM_QSTR(MP_QSTR_GAMMASET), MP_ROM_INT(0x26)},
    {MP_ROM_QSTR(MP_QSTR_DISPLAY_OFF), MP_ROM_INT(0x28)},
    {MP_ROM_QSTR(MP_QSTR_DISPLAY_ON), MP_ROM_INT(0x29)},
    {MP_ROM_QSTR(MP_QSTR_SET_COLUMN), MP_ROM_INT(0x2a)},
    {MP_ROM_QSTR(MP_QSTR_SET_PAGE), MP_ROM_INT(0x2b)},
    {MP_ROM_QSTR(MP_QSTR_WRITE_RAM), MP_ROM_INT(0x2c)},
    {MP_ROM_QSTR(MP_QSTR_READ_RAM), MP_ROM_INT(0x2e)},
    {MP_ROM_QSTR(MP_QSTR_PTLAR), MP_ROM_INT(0x30)},
    {MP_ROM_QSTR(MP_QSTR_VSCRDEF), MP_ROM_INT(0x33)},
    {MP_ROM_QSTR(MP_QSTR_MADCTL), MP_ROM_INT(0x36)},
    {MP_ROM_QSTR(MP_QSTR_VSCRSADD), MP_ROM_INT(0x37)},
    {MP_ROM_QSTR(MP_QSTR_PIXFMT), MP_ROM_INT(0x3a)},
    {MP_ROM_QSTR(MP_QSTR_WRITE_DISPLAY_BRIGHTNESS), MP_ROM_INT(0x51)},
    {MP_ROM_QSTR(MP_QSTR_READ_DISPLAY_BRIGHTNESS), MP_ROM_INT(0x52)},
    {MP_ROM_QSTR(MP_QSTR_WRITE_CTRL_DISPLAY), MP_ROM_INT(0x53)},
    {MP_ROM_QSTR(MP_QSTR_READ_CTRL_DISPLAY), MP_ROM_INT(0x54)},
    {MP_ROM_QSTR(MP_QSTR_WRITE_CABC), MP_ROM_INT(0x55)},
    {MP_ROM_QSTR(MP_QSTR_READ_CABC), MP_ROM_INT(0x56)},
    {MP_ROM_QSTR(MP_QSTR_WRITE_CABC_MINIMUM), MP_ROM_INT(0x5e)},
    {MP_ROM_QSTR(MP_QSTR_READ_CABC_MINIMUM), MP_ROM_INT(0x5f)},
    {MP_ROM_QSTR(MP_QSTR_FRMCTR1), MP_ROM_INT(0xb1)},
    {MP_ROM_QSTR(MP_QSTR_FRMCTR2), MP_ROM_INT(0xb2)},
    {MP_ROM_QSTR(MP_QSTR_FRMCTR3), MP_ROM_INT(0xb3)},
    {MP_ROM_QSTR(MP_QSTR_INVCTR), MP_ROM_INT(0xb4)},
    {MP_ROM_QSTR(MP_QSTR_DFUNCTR), MP_ROM_INT(0xb6)},
    {MP_ROM_QSTR(MP_QSTR_PWCTR1), MP_ROM_INT(0xc0)},
    {MP_ROM_QSTR(MP_QSTR_PWCTR2), MP_ROM_INT(0xc1)},
    {MP_ROM_QSTR(MP_QSTR_PWCTRA), MP_ROM_INT(0xcb)},
    {MP_ROM_QSTR(MP_QSTR_PWCTRB), MP_ROM_INT(0xcf)},
    {MP_ROM_QSTR(MP_QSTR_VMCTR1), MP_ROM_INT(0xc5)},
    {MP_ROM_QSTR(MP_QSTR_VMCTR2), MP_ROM_INT(0xc7)},
    {MP_ROM_QSTR(MP_QSTR_RDID1), MP_ROM_INT(0xda)},
    {MP_ROM_QSTR(MP_QSTR_RDID2), MP_ROM_INT(0xdb)},
    {MP_ROM_QSTR(MP_QSTR_RDID3), MP_ROM_INT(0xdc)},
    {MP_ROM_QSTR(MP_QSTR_RDID4), MP_ROM_INT(0xdd)},
    {MP_ROM_QSTR(MP_QSTR_GMCTRP1), MP_ROM_INT(0xe0)},
    {MP_ROM_QSTR(MP_QSTR_GMCTRN1), MP_ROM_INT(0xe1)},
    {MP_ROM_QSTR(MP_QSTR_DTCA), MP_ROM_INT(0xe8)},
    {MP_ROM_QSTR(MP_QSTR_DTCB), MP_ROM_INT(0xea)},
    {MP_ROM_QSTR(MP_QSTR_POSC), MP_ROM_INT(0xed)},
    {MP_ROM_QSTR(MP_QSTR_ENABLE3G), MP_ROM_INT(0xf2)},
    {MP_ROM_QSTR(MP_QSTR_PUMPRC), MP_ROM_INT(0xf7)},
    {MP_ROM_QSTR(MP_QSTR_cleanup), MP_ROM_PTR(&display_cleanup_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&display_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_display_off), MP_ROM_PTR(&display_off_method_obj)},
    {MP_ROM_QSTR(MP_QSTR_display_on), MP_ROM_PTR(&display_on_method_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&display_draw_circle_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_ellipse), MP_ROM_PTR(&display_draw_ellipse_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_hline), MP_ROM_PTR(&display_draw_hline_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_image), MP_ROM_PTR(&display_draw_image_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&display_draw_line_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_lines), MP_ROM_PTR(&display_draw_lines_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_letter), MP_ROM_PTR(&display_draw_letter_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&display_draw_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_polygon), MP_ROM_PTR(&display_draw_polygon_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_rectangle), MP_ROM_PTR(&display_draw_rectangle_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_sprite), MP_ROM_PTR(&display_draw_sprite_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&display_draw_text_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_text8x8), MP_ROM_PTR(&display_draw_text8x8_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_vline), MP_ROM_PTR(&display_draw_vline_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&display_fill_circle_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_ellipse), MP_ROM_PTR(&display_fill_ellipse_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_hrect), MP_ROM_PTR(&display_fill_hrect_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_polygon), MP_ROM_PTR(&display_fill_polygon_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_rectangle), MP_ROM_PTR(&display_fill_rectangle_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_vrect), MP_ROM_PTR(&display_fill_vrect_obj)},
    {MP_ROM_QSTR(MP_QSTR_invert), MP_ROM_PTR(&display_invert_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_off_grid), MP_ROM_PTR(&display_is_off_grid_obj)},
    {MP_ROM_QSTR(MP_QSTR_load_sprite), MP_ROM_PTR(&display_load_sprite_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset_cpy), MP_ROM_PTR(&display_reset_cpy_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset_mpy), MP_ROM_PTR(&display_reset_mpy_obj)},
    {MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&display_scroll_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_scroll), MP_ROM_PTR(&display_set_scroll_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&display_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_cmd_mpy), MP_ROM_PTR(&display_write_cmd_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_cmd_cpy), MP_ROM_PTR(&display_write_cmd_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_data_mpy), MP_ROM_PTR(&display_write_data_mpy_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_data_cpy), MP_ROM_PTR(&display_write_data_mpy_obj)},
};
static MP_DEFINE_CONST_DICT(display_locals_dict, display_locals_table);

} // namespace

MP_DEFINE_CONST_OBJ_TYPE(
    lcd_tft_display_type,
    MP_QSTR_Display,
    MP_TYPE_FLAG_NONE,
    make_new, reinterpret_cast<const void *>(display_make_new),
    attr, reinterpret_cast<const void *>(display_attr),
    locals_dict, &display_locals_dict
);

extern "C" mp_obj_t lcd_tft_color565(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_int_t r = mp_obj_get_int(args[0]);
    mp_int_t g = mp_obj_get_int(args[1]);
    mp_int_t b = mp_obj_get_int(args[2]);
    return mp_obj_new_int(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}
