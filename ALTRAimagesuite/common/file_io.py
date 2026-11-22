"""File I/O utility functions."""

import json
import os
import re
from pathlib import Path
from typing import Any, Tuple


def read_bytes(path: str) -> bytes:
    """Read binary file contents."""
    with open(path, "rb") as f:
        return f.read()


def write_bytes(path: str, data: bytes) -> None:
    """Write binary data safely to file."""
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def read_relaxed_json(path: str) -> Any:
    """
    Read JSON from path. Tolerant of Python-style tuples or single quotes
    e.g. [(3, 5), (3, 10)] as described in the specification.
    """
    with open(path, "r", encoding="utf-8") as f:
        raw = f.read()

    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        # Normalize python tuples (x, y) to JSON arrays [x, y]
        normalized = re.sub(r"\(\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*\)", r"[\1, \2]", raw)
        # Normalize single quotes to double quotes if any
        normalized = normalized.replace("'", '"')
        # Remove trailing commas in lists/dicts
        normalized = re.sub(r",\s*([\]}])", r"\1", normalized)
        return json.loads(normalized)


def write_json(path: str, data: Any, indent: int = 2) -> None:
    """Write JSON data to path with indentation."""
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=indent)


def get_fxbuf_paths(path: str) -> Tuple[str, str]:
    """
    Given a file path (with .data, .json, or base name),
    returns (data_path, json_path).
    """
    p = Path(path)
    stem = p.stem
    if p.name.endswith(".data") or p.name.endswith(".json"):
        base_dir = p.parent
        data_path = str(base_dir / f"{stem}.data")
        json_path = str(base_dir / f"{stem}.json")
    else:
        # Full path is stem
        base_dir = p.parent
        data_path = str(p.with_suffix(".data"))
        json_path = str(p.with_suffix(".json"))
    return data_path, json_path
