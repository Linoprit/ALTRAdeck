"""Image resize helpers."""

from typing import Optional, Tuple
from PIL import Image


def calculate_aspect_ratio_size(
    orig_w: int,
    orig_h: int,
    target_w: Optional[int] = None,
    target_h: Optional[int] = None
) -> Tuple[int, int]:
    """
    Calculate target dimensions preserving original aspect ratio.

    If target_w is provided, target_h is computed.
    If target_h is provided, target_w is computed.
    """
    if orig_w <= 0 or orig_h <= 0:
        return target_w or 1, target_h or 1

    ratio = orig_w / orig_h

    if target_w is not None and target_h is None:
        target_h = max(1, round(target_w / ratio))
        return target_w, target_h

    if target_h is not None and target_w is None:
        target_w = max(1, round(target_h * ratio))
        return target_w, target_h

    if target_w is not None and target_h is not None:
        return target_w, target_h

    return orig_w, orig_h


def resize_image(
    image: Image.Image,
    width: int,
    height: int,
    resample: int = Image.Resampling.LANCZOS
) -> Image.Image:
    """
    Resize PIL image to target width and height using specified resample filter.
    """
    if width <= 0 or height <= 0:
        raise ValueError(f"Invalid dimensions: {width}x{height}")
    return image.resize((width, height), resample=resample)


def crop_image(image: Image.Image, x: int, y: int, width: int, height: int) -> Image.Image:
    """
    Crop PIL image to specified bounding box [x, y, x + width, y + height].
    """
    if width <= 0 or height <= 0:
        raise ValueError(f"Invalid crop dimensions: {width}x{height}")
    box = (x, y, x + width, y + height)
    return image.crop(box)


def offset_collis_box_points(
    points: list,
    offset_x: int,
    offset_y: int,
    max_w: int,
    max_h: int
) -> list:
    """
    Offset collis-box points when an image is cropped, clamping to new image bounds.
    """
    new_points = []
    for px, py in points:
        nx = max(0, min(max_w - 1, px - offset_x))
        ny = max(0, min(max_h - 1, py - offset_y))
        new_points.append((nx, ny))
    return new_points
