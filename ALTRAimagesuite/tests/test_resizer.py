"""Tests for image resizer and cropper."""

from PIL import Image
import pytest
from app_parts.img_conversion.resizer import (
    calculate_aspect_ratio_size,
    resize_image,
    crop_image,
    offset_collis_box_points,
)


def test_aspect_ratio():
    # 400x200 is 2:1
    w, h = calculate_aspect_ratio_size(400, 200, target_w=200)
    assert w == 200
    assert h == 100

    w, h = calculate_aspect_ratio_size(400, 200, target_h=50)
    assert w == 100
    assert h == 50

    w, h = calculate_aspect_ratio_size(400, 200, target_w=300, target_h=150)
    assert w == 300
    assert h == 150


def test_resize_image():
    img = Image.new("RGB", (100, 100), color=(255, 0, 0))
    resized = resize_image(img, 50, 25)
    assert resized.size == (50, 25)

    with pytest.raises(ValueError):
        resize_image(img, 0, 50)


def test_crop_image():
    # Create 100x100 test image with 4 distinct colored quadrants
    img = Image.new("RGB", (100, 100), color=(0, 0, 0))
    # Red quadrant at top-left: (0,0) to (50,50)
    for x in range(50):
        for y in range(50):
            img.putpixel((x, y), (255, 0, 0))

    cropped = crop_image(img, 10, 10, 30, 20)
    assert cropped.size == (30, 20)
    assert cropped.getpixel((0, 0)) == (255, 0, 0)
    assert cropped.getpixel((29, 19)) == (255, 0, 0)

    with pytest.raises(ValueError):
        crop_image(img, 10, 10, 0, 20)

    with pytest.raises(ValueError):
        crop_image(img, 10, 10, 20, -5)


def test_offset_collis_box_points():
    points = [(10, 10), (30, 15), (50, 60), (5, 80)]
    # Crop offset = (10, 10), new image size = 40x40
    offset_pts = offset_collis_box_points(points, offset_x=10, offset_y=10, max_w=40, max_h=40)

    # (10,10) - (10,10) = (0,0)
    assert offset_pts[0] == (0, 0)
    # (30,15) - (10,10) = (20,5)
    assert offset_pts[1] == (20, 5)
    # (50,60) - (10,10) = (40,50) -> clamped to (39, 39)
    assert offset_pts[2] == (39, 39)
    # (5,80) - (10,10) = (-5, 70) -> clamped to (0, 39)
    assert offset_pts[3] == (0, 39)
