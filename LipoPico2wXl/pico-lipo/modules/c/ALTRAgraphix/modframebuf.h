#ifndef MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H
#define MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "py/obj.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_framebuf;

bool mp_framebuf_get_rgb565_info(mp_obj_t obj, size_t *width, size_t *height,
    size_t *stride);

#if defined(__cplusplus)
}
#endif

#endif // MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H
