"""RGB565 conversion module for ALTRAgraphix fxbuf format."""

import struct
from typing import Tuple
from PIL import Image
import numpy as np


def rgb888_to_rgb565_pixel(r: int, g: int, b: int) -> int:
    """Convert a single 24-bit RGB pixel into a 16-bit RGB565 integer."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def rgb565_pixel_to_rgb888(val: int) -> Tuple[int, int, int]:
    """Convert a single 16-bit RGB565 integer into a 24-bit (R, G, B) tuple."""
    r5 = (val >> 11) & 0x1F
    g6 = (val >> 5) & 0x3F
    b5 = val & 0x1F
    r8 = (r5 * 255) // 31
    g8 = (g6 * 255) // 63
    b8 = (b5 * 255) // 31
    return r8, g8, b8


def image_to_rgb565(image: Image.Image, byteorder: str = "little") -> bytes:
    """
    Convert a PIL Image to raw RGB565 bytes.

    Args:
        image: PIL Image object.
        byteorder: 'little' (default for ARM/RP2040) or 'big'.

    Returns:
        Raw bytes of length (width * height * 2).
    """
    if image.mode != "RGB":
        image = image.convert("RGB")

    arr = np.asarray(image, dtype=np.uint16)
    r = (arr[:, :, 0] >> 3) & 0x1F
    g = (arr[:, :, 1] >> 2) & 0x3F
    b = (arr[:, :, 2] >> 3) & 0x1F

    rgb16 = (r << 11) | (g << 5) | b

    dtype = "<u2" if byteorder == "little" else ">u2"
    return rgb16.astype(dtype).tobytes()


def rgb565_to_image(data: bytes, width: int, height: int, byteorder: str = "little") -> Image.Image:
    """
    Convert raw RGB565 bytes to a PIL Image.

    Args:
        data: Raw byte array of length (width * height * 2).
        width: Image width in pixels.
        height: Image height in pixels.
        byteorder: 'little' or 'big'.

    Returns:
        PIL Image in 'RGB' mode.
    """
    expected_len = width * height * 2
    if len(data) != expected_len:
        raise ValueError(f"Data length mismatch: expected {expected_len} bytes, got {len(data)}")

    dtype = "<u2" if byteorder == "little" else ">u2"
    val = np.frombuffer(data, dtype=dtype).reshape((height, width))

    r5 = (val >> 11) & 0x1F
    g6 = (val >> 5) & 0x3F
    b5 = val & 0x1F

    # Scale 5/6 bits back to 8 bits
    r = ((r5 * 255) // 31).astype(np.uint8)
    g = ((g6 * 255) // 63).astype(np.uint8)
    b = ((b5 * 255) // 31).astype(np.uint8)

    rgb = np.stack([r, g, b], axis=-1)
    return Image.fromarray(rgb, "RGB")


def swap_image_endianness(image: Image.Image, from_byteorder: str = "little") -> Tuple[Image.Image, str]:
    """
    Swap the 16-bit RGB565 endianness of an image.
    Converts image to raw RGB565 bytes using from_byteorder, then decodes using the opposite
    byte order, effectively swapping the high and low bytes of each 16-bit pixel.

    Returns:
        (swapped_image, to_byteorder)
    """
    to_byteorder = "big" if from_byteorder == "little" else "little"
    raw_data = image_to_rgb565(image, byteorder=from_byteorder)
    w, h = image.size
    swapped_image = rgb565_to_image(raw_data, w, h, byteorder=to_byteorder)
    return swapped_image, to_byteorder
