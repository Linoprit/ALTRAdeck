"""Polygon geometry calculations for bump-boxes."""

import math
from typing import List, Sequence, Tuple


Point = Tuple[float, float]
IntPoint = Tuple[int, int]


def signed_area(points: Sequence[Point]) -> float:
    """
    Compute signed area using shoelace formula:
    0.5 * sum(x_i * y_{i+1} - x_{i+1} * y_i).
    """
    n = len(points)
    if n < 3:
        return 0.0

    area2 = 0.0
    for i in range(n):
        x1, y1 = points[i]
        x2, y2 = points[(i + 1) % n]
        area2 += (x1 * y2 - x2 * y1)

    return area2 * 0.5


def is_counter_clockwise(points: Sequence[Point], screen_coords: bool = True) -> bool:
    """
    Check if polygon vertices are ordered counter-clockwise.

    In standard Cartesian (Y up), signed area > 0 is CCW.
    In screen coordinates (Y down, as used in ALTRAgraphix where
    [(3,5), (3,10), (6,10), (6,5)] is specified as counter-clockwise),
    the signed area is negative for CCW.
    """
    area = signed_area(points)
    if screen_coords:
        return area < 0
    return area > 0


def cross_product_2d(p1: Point, p2: Point, p3: Point) -> float:
    """
    Calculate cross product of vectors (p2 - p1) and (p3 - p2).
    Positive: left turn in Cartesian / right turn in screen coords.
    """
    dx1 = p2[0] - p1[0]
    dy1 = p2[1] - p1[1]
    dx2 = p3[0] - p2[0]
    dy2 = p3[1] - p2[1]
    return dx1 * dy2 - dy1 * dx2


def is_convex(points: Sequence[Point]) -> bool:
    """
    Determine if polygon is convex (all turns have same direction).
    Must have at least 3 points.
    """
    n = len(points)
    if n < 3:
        return False

    sign = 0
    for i in range(n):
        p1 = points[i]
        p2 = points[(i + 1) % n]
        p3 = points[(i + 2) % n]
        cp = cross_product_2d(p1, p2, p3)

        if abs(cp) > 1e-7:
            current_sign = 1 if cp > 0 else -1
            if sign == 0:
                sign = current_sign
            elif current_sign != sign:
                return False

    return sign != 0


def make_counter_clockwise(points: Sequence[IntPoint], screen_coords: bool = True) -> List[IntPoint]:
    """Return points ordered counter-clockwise."""
    if len(points) < 3:
        return list(points)
    if is_counter_clockwise(points, screen_coords=screen_coords):
        return list(points)
    return list(reversed(points))


def find_closest_point(
    points: Sequence[Point],
    target: Point,
    threshold: float = 8.0
) -> int:
    """
    Find index of closest point within distance threshold.
    Returns -1 if no point is within threshold.
    """
    best_idx = -1
    best_dist = threshold

    tx, ty = target
    for i, (px, py) in enumerate(points):
        d = math.hypot(px - tx, py - ty)
        if d <= best_dist:
            best_dist = d
            best_idx = i

    return best_idx


def compute_convex_hull(points: Sequence[Point], screen_coords: bool = True) -> List[IntPoint]:
    """
    Compute convex hull of points using Monotone Chain algorithm.
    Returns points ordered counter-clockwise.
    """
    pts = sorted(set((round(p[0]), round(p[1])) for p in points))
    if len(pts) <= 2:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)

    upper = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)

    hull = lower[:-1] + upper[:-1]
    return make_counter_clockwise(hull, screen_coords=screen_coords)


def generate_bounding_box_hull(width: int, height: int, margin: int = 0) -> List[IntPoint]:
    """
    Generate default CCW bounding box for given width & height.
    Coordinates refer to discrete pixel coordinates where (0, 0) is the top-left pixel.
    """
    m = max(0, margin)
    w = max(0, width - 1 - m)
    h = max(0, height - 1 - m)
    # Order: Top-left -> Bottom-left -> Bottom-right -> Top-right (CCW in screen coords)
    return [(m, m), (m, h), (w, h), (w, m)]
