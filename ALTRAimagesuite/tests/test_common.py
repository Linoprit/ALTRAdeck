"""Tests for common utilities."""

from common.cli_parser import parse_args
from common.file_io import get_fxbuf_paths
from common.logger import setup_logger
from common.settings import SettingsManager


def test_cli_parser():
    args = parse_args(["my_image.png", "-b", "big", "-r", "320x240", "--debug"])
    assert args.file == "my_image.png"
    assert args.byte_order == "big"
    assert args.resize == "320x240"
    assert args.debug is True


def test_fxbuf_paths():
    d, j = get_fxbuf_paths("foo.data")
    assert d.endswith("foo.data")
    assert j.endswith("foo.json")

    d, j = get_fxbuf_paths("foo.json")
    assert d.endswith("foo.data")
    assert j.endswith("foo.json")

    d, j = get_fxbuf_paths("foo")
    assert d.endswith("foo.data")
    assert j.endswith("foo.json")


def test_settings_manager(tmp_path):
    cfg_file = str(tmp_path / "settings.json")
    mgr = SettingsManager(cfg_file)
    assert mgr.get("byte_order") == "little"

    mgr.set("byte_order", "big")
    assert mgr.get("byte_order") == "big"

    # Reload from disk
    mgr2 = SettingsManager(cfg_file)
    assert mgr2.get("byte_order") == "big"


def test_logger():
    log = setup_logger("test_suite")
    assert log is not None


def test_fxbuf_overwrite_detection(tmp_path):
    base = str(tmp_path / "sprite")
    data_p, json_p = get_fxbuf_paths(base)

    # Initially neither exists
    import os
    assert not os.path.exists(data_p)
    assert not os.path.exists(json_p)

    # Create dummy data file
    with open(data_p, "w") as f:
        f.write("test")

    assert os.path.exists(data_p)
    assert not os.path.exists(json_p)

