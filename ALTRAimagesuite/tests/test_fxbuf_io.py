"""Tests for fxbuf I/O operations."""

import os
from PIL import Image
import pytest

from app_parts.img_conversion.fxbuf_io import load_fxbuf, save_fxbuf, load_any_image, FxbufImage


def test_save_and_load_fxbuf(tmp_path):
    w, h = 40, 30
    img = Image.new("RGB", (w, h), color=(100, 150, 200))
    collis_box = [(3, 5), (3, 10), (6, 10), (6, 5)]

    base_out = str(tmp_path / "test_sprite")
    data_path, json_path = save_fxbuf(
        base_out,
        img,
        collis_box=collis_box,
        trans_col=0xFF00FF,
        byteorder="little"
    )

    assert os.path.exists(data_path)
    assert os.path.exists(json_path)

    loaded = load_fxbuf(data_path, byteorder="little")
    assert loaded.width == w
    assert loaded.height == h
    assert loaded.trans_col == 0xFF00FF
    assert loaded.collis_box == collis_box
    assert loaded.bump_box == collis_box  # alias check
    assert loaded.image.size == (w, h)

    # Test loading from .json path as well
    loaded_json = load_fxbuf(json_path, byteorder="little")
    assert loaded_json.width == w
    assert loaded_json.height == h
    assert loaded_json.trans_col == 0xFF00FF
    assert loaded_json.collis_box == collis_box

    # Test loading using base path without extension
    loaded_no_ext = load_any_image(base_out, byteorder="little")
    assert loaded_no_ext.width == w
    assert loaded_no_ext.height == h
    assert loaded_no_ext.trans_col == 0xFF00FF
    assert loaded_no_ext.collis_box == collis_box


def test_load_with_relaxed_json(tmp_path):
    w, h = 20, 20
    img = Image.new("RGB", (w, h), color=(50, 50, 50))
    base_out = str(tmp_path / "relaxed_test")
    data_path, json_path = save_fxbuf(base_out, img, collis_box=[(1, 2), (3, 4)], trans_col=0, byteorder="big")

    # Manually write relaxed python tuple syntax into JSON (legacy bump_box key support)
    with open(json_path, "w", encoding="utf-8") as f:
        f.write('{\n  "width": 20,\n  "height": 20,\n  "trans_col": 0,\n  "bump_box": [\n    (1, 2),\n    (3, 4)\n  ]\n}')

    loaded = load_fxbuf(data_path, byteorder="big")
    assert loaded.width == 20
    assert loaded.height == 20
    assert loaded.trans_col == 0
    assert loaded.collis_box == [(1, 2), (3, 4)]


def test_load_any_image_standard(tmp_path):
    w, h = 50, 25
    img = Image.new("RGB", (w, h), color=(80, 120, 160))
    png_path = str(tmp_path / "test.png")
    img.save(png_path)

    loaded = load_any_image(png_path)
    assert loaded.width == w
    assert loaded.height == h
    assert loaded.trans_col == 0
    assert loaded.collis_box == []
