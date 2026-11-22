"""Application settings read/write module."""

import json
import os
from pathlib import Path
from typing import Any, Dict


DEFAULT_SETTINGS: Dict[str, Any] = {
    "byte_order": "little",
    "last_directory": "",
    "preserve_aspect_ratio": True,
    "zoom_factor": 1.0,
    "window_width": 1024,
    "window_height": 720,
    "recent_files": [],
    "grid_visible": False,
    "point_hit_radius": 8,
}


class SettingsManager:
    """Manages application settings persistence."""

    def __init__(self, config_path: str = ""):
        if not config_path:
            home = Path.home()
            config_dir = home / ".config" / "ALTRAimagesuite"
            self.config_dir = config_dir
            self.config_path = config_dir / "settings.json"
        else:
            self.config_path = Path(config_path)
            self.config_dir = self.config_path.parent

        self._settings: Dict[str, Any] = dict(DEFAULT_SETTINGS)
        self.load()

    def load(self) -> Dict[str, Any]:
        """Load settings from JSON file if it exists."""
        if self.config_path.is_file():
            try:
                with open(self.config_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    if isinstance(data, dict):
                        self._settings.update(data)
            except Exception:
                # Fall back to defaults on corrupt config
                pass
        return self._settings

    def save(self) -> bool:
        """Save settings to JSON file."""
        try:
            self.config_dir.mkdir(parents=True, exist_ok=True)
            with open(self.config_path, "w", encoding="utf-8") as f:
                json.dump(self._settings, f, indent=2)
            return True
        except Exception:
            return False

    def get(self, key: str, default: Any = None) -> Any:
        """Get a setting value."""
        return self._settings.get(key, default)

    def set(self, key: str, value: Any, auto_save: bool = True) -> None:
        """Set a setting value."""
        self._settings[key] = value
        if auto_save:
            self.save()

    def add_recent_file(self, filepath: str, max_items: int = 10) -> None:
        """Add a file path to recent files list."""
        recent = self.get("recent_files", [])
        if filepath in recent:
            recent.remove(filepath)
        recent.insert(0, filepath)
        self.set("recent_files", recent[:max_items])
