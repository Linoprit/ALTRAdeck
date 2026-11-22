"""Collis-box model and point management."""

from typing import Callable, List, Optional, Tuple
from .polygon import is_convex, is_counter_clockwise, make_counter_clockwise, find_closest_point


class CollisBoxModel:
    """
    Model holding collis-box polygon points, undo/redo state, and notifications.
    """

    def __init__(self, initial_points: Optional[List[Tuple[int, int]]] = None):
        self._points: List[Tuple[int, int]] = list(initial_points or [])
        self._undo_stack: List[List[Tuple[int, int]]] = []
        self._redo_stack: List[List[Tuple[int, int]]] = []
        self._listeners: List[Callable[[], None]] = []

    @property
    def points(self) -> List[Tuple[int, int]]:
        """Return list of points."""
        return list(self._points)

    def add_listener(self, callback: Callable[[], None]) -> None:
        """Register a change listener callback."""
        if callback not in self._listeners:
            self._listeners.append(callback)

    def remove_listener(self, callback: Callable[[], None]) -> None:
        """Remove registered change listener."""
        if callback in self._listeners:
            self._listeners.remove(callback)

    def _notify(self) -> None:
        """Notify all registered listeners of a change."""
        for cb in self._listeners:
            try:
                cb()
            except Exception:
                pass

    def _save_history(self) -> None:
        """Save current state to undo stack and clear redo stack."""
        self._undo_stack.append(list(self._points))
        if len(self._undo_stack) > 50:
            self._undo_stack.pop(0)
        self._redo_stack.clear()

    def save_undo_snapshot(self) -> None:
        """Explicitly save current points to undo stack."""
        self._save_history()

    def update_point_position(self, index: int, x: int, y: int, record_history: bool = False) -> bool:
        """Update coordinates of point at index."""
        if 0 <= index < len(self._points):
            if record_history:
                self._save_history()
            self._points[index] = (int(round(x)), int(round(y)))
            self._notify()
            return True
        return False

    def set_points(self, points: List[Tuple[int, int]], record_history: bool = True) -> None:
        """Set all points in the model."""
        if record_history:
            self._save_history()
        self._points = [(int(p[0]), int(p[1])) for p in points]
        self._notify()

    def add_point(self, x: int, y: int, index: Optional[int] = None) -> None:
        """Add a point (x, y) at index or append to the end."""
        self._save_history()
        pt = (int(round(x)), int(round(y)))
        if index is None or index >= len(self._points):
            self._points.append(pt)
        else:
            self._points.insert(max(0, index), pt)
        self._notify()

    def remove_point(self, index: int) -> bool:
        """Remove point at given index."""
        if 0 <= index < len(self._points):
            self._save_history()
            self._points.pop(index)
            self._notify()
            return True
        return False

    def remove_point_at_pos(self, x: float, y: float, threshold: float = 8.0) -> bool:
        """Find and remove closest point within threshold distance."""
        idx = find_closest_point(self._points, (x, y), threshold=threshold)
        if idx >= 0:
            return self.remove_point(idx)
        return False

    def move_point_up(self, index: int) -> int:
        """Move point up in the list (towards index 0). Returns new index."""
        if 1 <= index < len(self._points):
            self._save_history()
            self._points[index - 1], self._points[index] = self._points[index], self._points[index - 1]
            self._notify()
            return index - 1
        return index

    def move_point_down(self, index: int) -> int:
        """Move point down in the list (towards end). Returns new index."""
        if 0 <= index < len(self._points) - 1:
            self._save_history()
            self._points[index], self._points[index + 1] = self._points[index + 1], self._points[index]
            self._notify()
            return index + 1
        return index

    def clear(self) -> None:
        """Remove all points."""
        if self._points:
            self._save_history()
            self._points.clear()
            self._notify()

    def can_undo(self) -> bool:
        """Check if undo is available."""
        return len(self._undo_stack) > 0

    def can_redo(self) -> bool:
        """Check if redo is available."""
        return len(self._redo_stack) > 0

    def undo(self) -> bool:
        """Undo last point change."""
        if self._undo_stack:
            self._redo_stack.append(list(self._points))
            self._points = self._undo_stack.pop()
            self._notify()
            return True
        return False

    def redo(self) -> bool:
        """Redo last undone change."""
        if self._redo_stack:
            self._undo_stack.append(list(self._points))
            self._points = self._redo_stack.pop()
            self._notify()
            return True
        return False

    def is_convex(self) -> bool:
        """Check if current polygon is convex."""
        return is_convex(self._points)

    def is_counter_clockwise(self, screen_coords: bool = True) -> bool:
        """Check if current polygon is ordered counter-clockwise."""
        return is_counter_clockwise(self._points, screen_coords=screen_coords)

    def enforce_counter_clockwise(self, screen_coords: bool = True) -> None:
        """Ensure current points are ordered counter-clockwise."""
        if len(self._points) >= 3 and not self.is_counter_clockwise(screen_coords=screen_coords):
            self._save_history()
            self._points = make_counter_clockwise(self._points, screen_coords=screen_coords)
            self._notify()


# Backwards compatibility alias
BumpBoxModel = CollisBoxModel
