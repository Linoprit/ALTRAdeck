"""Tests for CLI headless conversion."""

from pathlib import Path
from PIL import Image
import pytest

from common.cli_parser import parse_args
from main import run_headless_conversion
from app_parts.img_conversion.fxbuf_io import load_fxbuf


def test_headless_conversion(tmp_path):
    # Create test input image
    in_img = Image.new("RGB", (120, 90), color=(200, 100, 50))
    in_path = str(tmp_path / "input.png")
    in_img.save(in_path)

    out_base = str(tmp_path / "output_sprite")

    # Run conversion with resizing to 60x45, big-endian, and trans_col
    args = parse_args([in_path, "-c", out_base, "-b", "big", "-r", "60x45", "-t", "0x00FF00"])
    ret = run_headless_conversion(args)
    assert ret == 0

    data_file = tmp_path / "output_sprite.data"
    json_file = tmp_path / "output_sprite.json"

    assert data_file.is_file()
    assert json_file.is_file()

    # Verify data file size: 60 * 45 * 2 = 5400 bytes
    assert data_file.stat().st_size == 60 * 45 * 2

    # Verify loaded fxbuf
    loaded = load_fxbuf(str(data_file), byteorder="big")
    assert loaded.width == 60
    assert loaded.height == 45
    assert loaded.byteorder == "big"
    assert loaded.trans_col == 0x00FF00

    # Also test headless conversion using input filename without extension
    out_base2 = str(tmp_path / "output_sprite2")
    args2 = parse_args([str(out_base), "-c", out_base2, "-b", "little"])
    ret2 = run_headless_conversion(args2)
    assert ret2 == 0
    assert (tmp_path / "output_sprite2.data").is_file()
    assert (tmp_path / "output_sprite2.json").is_file()


def test_headless_missing_file():
    args = parse_args(["-c", "out"])
    ret = run_headless_conversion(args)
    assert ret != 0
