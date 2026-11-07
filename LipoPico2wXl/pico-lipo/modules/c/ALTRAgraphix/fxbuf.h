#ifndef ALTRAGRAPHIX_FXBUF_H
#define ALTRAGRAPHIX_FXBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "py/obj.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_fxbuf;

bool mp_framebuf_get_rgb565_info(mp_obj_t obj, size_t *width, size_t *height,
    size_t *stride);
mp_obj_t altra_fxbuf_get_native(mp_obj_t obj);

#if defined(__cplusplus)
}
#endif

#endif
