"""Collis-box package initialization."""

from .polygon import (
    Point,
    IntPoint,
    signed_area,
    is_counter_clockwise,
    is_convex,
    make_counter_clockwise,
    find_closest_point,
    compute_convex_hull,
    generate_bounding_box_hull,
)
from .collis_box_model import CollisBoxModel, BumpBoxModel

__all__ = [
    "Point",
    "IntPoint",
    "signed_area",
    "is_counter_clockwise",
    "is_convex",
    "make_counter_clockwise",
    "find_closest_point",
    "compute_convex_hull",
    "generate_bounding_box_hull",
    "CollisBoxModel",
    "BumpBoxModel",
]
