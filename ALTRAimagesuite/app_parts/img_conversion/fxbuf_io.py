"""ALTRAgraphix fxbuf I/O module handling .data and .json files."""

from dataclasses import dataclass, field
import os
from pathlib import Path
from typing import Any, List, Optional, Tuple, Union
from PIL import Image

from common.file_io import read_bytes, read_relaxed_json, write_bytes, write_json, get_fxbuf_paths
from common.logger import logger
from .rgb565 import image_to_rgb565, rgb565_to_image


def parse_trans_col(val: Any) -> int:
    """Parse trans_col value into an integer RGB888 representation."""
    if val is None:
        return 0
    if isinstance(val, int):
        return val
    if isinstance(val, str):
        val_str = val.strip()
        if val_str.startswith("#"):
            return int(val_str[1:], 16)
        if val_str.startswith("0x") or val_str.startswith("0X"):
            return int(val_str, 16)
        try:
            return int(val_str)
        except ValueError:
            return 0
    if isinstance(val, (list, tuple)) and len(val) >= 3:
        return (int(val[0]) << 16) | (int(val[1]) << 8) | int(val[2])
    return 0


@dataclass
class FxbufImage:
    """Holds image data, dimensions, transparent color, collis-box points and metadata."""
    image: Image.Image
    width: int
    height: int
    trans_col: int = 0
    collis_box: List[Tuple[int, int]] = field(default_factory=list)
    byteorder: str = "little"
    source_path: Optional[str] = None

    # Backwards compatibility property
    @property
    def bump_box(self) -> List[Tuple[int, int]]:
        return self.collis_box

    @bump_box.setter
    def bump_box(self, val: List[Tuple[int, int]]) -> None:
        self.collis_box = val

    @classmethod
    def from_image(
        cls,
        image: Image.Image,
        collis_box: Optional[List[Tuple[int, int]]] = None,
        trans_col: int = 0,
        byteorder: str = "little",
        source_path: Optional[str] = None
    ) -> "FxbufImage":
        """Create FxbufImage instance from a PIL Image."""
        if image.mode != "RGB":
            image = image.convert("RGB")
        w, h = image.size
        return cls(
            image=image,
            width=w,
            height=h,
            trans_col=parse_trans_col(trans_col),
            collis_box=list(collis_box or []),
            byteorder=byteorder,
            source_path=source_path
        )


def load_fxbuf(path: str, byteorder: str = "little") -> FxbufImage:
    """
    Load an fxbuf from either .data or .json file path.
    Looks for matching pair in the same directory.
    """
    data_path, json_path = get_fxbuf_paths(path)

    if not os.path.exists(data_path):
        raise FileNotFoundError(f"fxbuf data file not found: {data_path}")
    if not os.path.exists(json_path):
        raise FileNotFoundError(f"fxbuf metadata JSON not found: {json_path}")

    meta = read_relaxed_json(json_path)
    width = int(meta.get("width", 0))
    height = int(meta.get("height", 0))

    if width <= 0 or height <= 0:
        raise ValueError(f"Invalid dimensions in {json_path}: width={width}, height={height}")

    trans_col = parse_trans_col(meta.get("trans_col", meta.get("trans_color", 0)))

    raw_data = read_bytes(data_path)
    img = rgb565_to_image(raw_data, width, height, byteorder=byteorder)

    # Read collis_box (with fallback to old bump_box key)
    raw_points = meta.get("collis_box", meta.get("bump_box", []))
    collis_box: List[Tuple[int, int]] = []
    for pt in raw_points:
        if isinstance(pt, (list, tuple)) and len(pt) >= 2:
            collis_box.append((int(pt[0]), int(pt[1])))

    return FxbufImage(
        image=img,
        width=width,
        height=height,
        trans_col=trans_col,
        collis_box=collis_box,
        byteorder=byteorder,
        source_path=data_path
    )


def save_fxbuf(
    base_path: str,
    image: Image.Image,
    collis_box: Optional[List[Tuple[int, int]]] = None,
    trans_col: int = 0,
    byteorder: str = "little",
    bump_box: Optional[List[Tuple[int, int]]] = None
) -> Tuple[str, str]:
    """
    Save image and collis-box metadata as an fxbuf pair (.data + .json).

    Args:
        base_path: Base path or path ending in .data/.json.
        image: PIL Image object.
        collis_box: List of (x, y) coordinates.
        trans_col: Transparent RGB888 pixel color integer.
        byteorder: 'little' or 'big'.

    Returns:
        (data_path, json_path)
    """
    data_path, json_path = get_fxbuf_paths(base_path)

    if image.mode != "RGB":
        image = image.convert("RGB")

    width, height = image.size
    raw_data = image_to_rgb565(image, byteorder=byteorder)
    write_bytes(data_path, raw_data)

    points = collis_box if collis_box is not None else bump_box
    points_serialized = [[int(p[0]), int(p[1])] for p in (points or [])]
    meta = {
        "width": width,
        "height": height,
        "trans_col": parse_trans_col(trans_col),
        "collis_box": points_serialized
    }
    write_json(json_path, meta, indent=2)

    logger.info("Saved fxbuf: %s (%dx%d, trans_col=0x%06X, %s-endian)", data_path, width, height, parse_trans_col(trans_col), byteorder)
    return data_path, json_path


def load_any_image(path: str, byteorder: str = "little") -> FxbufImage:
    """
    Load an image from common formats (PNG, JPEG, BMP, etc.) or fxbuf (.data / .json).
    If a filename without extension is given, opens the corresponding .data and .json files.
    If a companion .json exists next to a common image format, loads collis_box and trans_col from it.
    """
    p = Path(path)
    ext = p.suffix.lower()

    data_path, json_path = get_fxbuf_paths(path)

    # If explicitly .data / .json, or filename has no extension, or fxbuf files exist for this path
    if ext in (".data", ".json") or not ext or (not os.path.exists(path) and (os.path.exists(data_path) or os.path.exists(json_path))):
        return load_fxbuf(path, byteorder=byteorder)

    # Standard image formats
    img = Image.open(path)
    if img.mode != "RGB":
        img = img.convert("RGB")

    collis_box: List[Tuple[int, int]] = []
    trans_col = 0

    # Check if a .json file with the same stem exists alongside the image
    companion_json = p.with_suffix(".json")
    if companion_json.is_file():
        try:
            meta = read_relaxed_json(str(companion_json))
            trans_col = parse_trans_col(meta.get("trans_col", meta.get("trans_color", 0)))
            for pt in meta.get("collis_box", meta.get("bump_box", [])):
                if isinstance(pt, (list, tuple)) and len(pt) >= 2:
                    collis_box.append((int(pt[0]), int(pt[1])))
        except Exception as e:
            logger.warning("Could not read companion JSON %s: %s", companion_json, e)

    return FxbufImage.from_image(img, collis_box=collis_box, trans_col=trans_col, byteorder=byteorder, source_path=path)
