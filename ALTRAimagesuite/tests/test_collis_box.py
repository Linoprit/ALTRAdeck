"""Tests for collis-box geometry and model."""

from app_parts.collis_box.polygon import (
    signed_area,
    is_counter_clockwise,
    is_convex,
    make_counter_clockwise,
    find_closest_point,
    compute_convex_hull,
    generate_bounding_box_hull,
)
from app_parts.collis_box.collis_box_model import CollisBoxModel, BumpBoxModel


def test_specification_example_is_ccw():
    # Example directly from Specification.md:
    spec_points = [(3, 5), (3, 10), (6, 10), (6, 5)]
    # In screen coordinates (Y down), this polygon is counter-clockwise
    assert is_counter_clockwise(spec_points, screen_coords=True)
    assert is_convex(spec_points)

    # Reversed order should NOT be CCW
    cw_points = list(reversed(spec_points))
    assert not is_counter_clockwise(cw_points, screen_coords=True)

    # make_counter_clockwise should fix it
    fixed = make_counter_clockwise(cw_points, screen_coords=True)
    assert is_counter_clockwise(fixed, screen_coords=True)


def test_convexity():
    # Regular convex triangle
    triangle = [(0, 0), (10, 0), (5, 10)]
    assert is_convex(triangle)

    # Concave polygon (arrowhead pointing in)
    concave = [(0, 0), (10, 0), (5, 5), (10, 10), (0, 10)]
    assert not is_convex(concave)

    # Too few points
    assert not is_convex([(0, 0), (10, 0)])


def test_generate_bounding_box_hull():
    # An image of 48x48 has pixels indexed from 0 to 47
    box = generate_bounding_box_hull(48, 48, margin=0)
    assert box == [(0, 0), (0, 47), (47, 47), (47, 0)]
    assert is_counter_clockwise(box, screen_coords=True)
    assert is_convex(box)


def test_closest_point():
    points = [(10, 10), (50, 50), (100, 100)]
    assert find_closest_point(points, (11, 12), threshold=5.0) == 0
    assert find_closest_point(points, (48, 52), threshold=5.0) == 1
    assert find_closest_point(points, (200, 200), threshold=10.0) == -1


def test_convex_hull():
    pts = [(0, 0), (10, 0), (10, 10), (0, 10), (5, 5)]
    hull = compute_convex_hull(pts, screen_coords=True)
    assert len(hull) == 4
    assert is_convex(hull)
    assert is_counter_clockwise(hull, screen_coords=True)


def test_collis_box_model():
    model = CollisBoxModel([(10, 10), (20, 20)])
    assert len(model.points) == 2

    # Add point
    model.add_point(30, 30)
    assert len(model.points) == 3
    assert model.points[-1] == (30, 30)

    # Reorder
    new_idx = model.move_point_up(2)
    assert new_idx == 1
    assert model.points == [(10, 10), (30, 30), (20, 20)]

    new_idx = model.move_point_down(0)
    assert new_idx == 1
    assert model.points == [(30, 30), (10, 10), (20, 20)]

    # Undo / Redo
    assert model.can_undo()
    model.undo()
    assert model.points == [(10, 10), (30, 30), (20, 20)]
    model.redo()
    assert model.points == [(30, 30), (10, 10), (20, 20)]

    # Remove point
    assert model.remove_point(1)
    assert model.points == [(30, 30), (20, 20)]

    # Clear
    model.clear()
    assert len(model.points) == 0
    assert model.can_undo()
    model.undo()
    assert len(model.points) == 2


def test_collis_box_drag_and_update():
    model = CollisBoxModel([(10, 10), (20, 20)])
    # Simulate drag with manual undo snapshot
    model.save_undo_snapshot()
    model.update_point_position(0, 15, 18, record_history=False)
    assert model.points[0] == (15, 18)

    # Undo should restore pre-drag state
    assert model.can_undo()
    model.undo()
    assert model.points[0] == (10, 10)
    assert model.points[1] == (20, 20)
