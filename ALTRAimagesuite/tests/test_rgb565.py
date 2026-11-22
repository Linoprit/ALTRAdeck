"""Tests for RGB565 conversion module."""

import pytest
from PIL import Image
from app_parts.img_conversion.rgb565 import (
    image_to_rgb565,
    rgb565_to_image,
    rgb888_to_rgb565_pixel,
    rgb565_pixel_to_rgb888,
    swap_image_endianness,
)


def test_pixel_conversion():
    # Pure red
    val = rgb888_to_rgb565_pixel(255, 0, 0)
    assert val == 0xF800
    r, g, b = rgb565_pixel_to_rgb888(val)
    assert r == 255 and g == 0 and b == 0

    # Pure green
    val = rgb888_to_rgb565_pixel(0, 255, 0)
    assert val == 0x07E0
    r, g, b = rgb565_pixel_to_rgb888(val)
    assert r == 0 and g == 255 and b == 0

    # Pure blue
    val = rgb888_to_rgb565_pixel(0, 0, 255)
    assert val == 0x001F
    r, g, b = rgb565_pixel_to_rgb888(val)
    assert r == 0 and g == 0 and b == 255

    # White
    val = rgb888_to_rgb565_pixel(255, 255, 255)
    assert val == 0xFFFF
    r, g, b = rgb565_pixel_to_rgb888(val)
    assert r == 255 and g == 255 and b == 255

    # Black
    val = rgb888_to_rgb565_pixel(0, 0, 0)
    assert val == 0x0000
    r, g, b = rgb565_pixel_to_rgb888(val)
    assert r == 0 and g == 0 and b == 0


@pytest.mark.parametrize("byteorder", ["little", "big"])
def test_image_roundtrip(byteorder):
    width, height = 32, 16
    img = Image.new("RGB", (width, height), color=(128, 64, 200))
    # Add some specific colors
    img.putpixel((0, 0), (255, 0, 0))
    img.putpixel((1, 0), (0, 255, 0))
    img.putpixel((2, 0), (0, 0, 255))

    data = image_to_rgb565(img, byteorder=byteorder)
    assert len(data) == width * height * 2

    restored = rgb565_to_image(data, width, height, byteorder=byteorder)
    assert restored.size == (width, height)
    assert restored.mode == "RGB"

    # Red pixel roundtrip check
    p_red = restored.getpixel((0, 0))
    assert p_red[0] == 255
    assert p_red[1] < 10
    assert p_red[2] < 10


def test_invalid_length():
    with pytest.raises(ValueError, match="Data length mismatch"):
        rgb565_to_image(b"\x00" * 10, width=10, height=10)


def test_swap_image_endianness():
    w, h = 20, 10
    img = Image.new("RGB", (w, h), color=(255, 0, 0))  # Red

    # Swapping from little to big
    swapped, new_bo = swap_image_endianness(img, from_byteorder="little")
    assert new_bo == "big"
    assert swapped.size == (w, h)
    # The swapped pixel colors will differ from original pure red
    assert swapped.getpixel((0, 0)) != img.getpixel((0, 0))

    # Swapping back from big to little should restore the original RGB565 representation
    restored, final_bo = swap_image_endianness(swapped, from_byteorder="big")
    assert final_bo == "little"
    # Round-trip color matches
    r, g, b = restored.getpixel((0, 0))
    assert r == 255 and g == 0 and b == 0
