"""Tests for pre-startup package dependency check."""

from unittest.mock import patch
import pytest

from common.dependencies import (
    check_missing_packages,
    format_missing_report,
    ensure_dependencies_or_exit,
    CORE_DEPENDENCIES,
    GUI_DEPENDENCIES,
)


def test_check_missing_packages_installed():
    # In this environment, numpy and PIL are installed
    missing = check_missing_packages(check_gui=False)
    assert missing == []


def test_check_missing_packages_simulated_missing():
    def fake_find_spec(name):
        if name == "PIL":
            return None
        return True

    with patch("importlib.util.find_spec", side_effect=fake_find_spec):
        missing = check_missing_packages(check_gui=False)
        assert ("PIL", "Pillow") in missing


def test_format_missing_report():
    missing = [("PIL", "Pillow"), ("wx", "wxPython")]
    report = format_missing_report(missing)
    assert "Pillow" in report
    assert "wxPython" in report
    assert "pip install" in report


def test_ensure_dependencies_exits_on_missing():
    with patch("common.dependencies.check_missing_packages", return_value=[("wx", "wxPython")]):
        with pytest.raises(SystemExit) as exc_info:
            ensure_dependencies_or_exit(check_gui=True)
        assert exc_info.value.code == 1


def test_ensure_dependencies_passes_when_all_present():
    with patch("common.dependencies.check_missing_packages", return_value=[]):
        # Should not raise SystemExit
        ensure_dependencies_or_exit(check_gui=True)
