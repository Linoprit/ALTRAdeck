"""Center view canvas displaying the image and collis-box polygon."""

import math
from typing import Callable, Optional, Tuple
from PIL import Image
import wx

from app_parts.collis_box.collis_box_model import CollisBoxModel


class CenterView(wx.ScrolledWindow):
    """
    Central scrollable viewport for displaying image, zooming, panning,
    picking transparent color, cropping/cutting, and interactive collis-box polygon creation.
    """

    MODE_INSPECT = "inspect"
    MODE_ADD = "add"
    MODE_REMOVE = "remove"
    MODE_PICK_TRANS_COL = "pick_trans_col"
    MODE_CUT = "cut"

    def __init__(self, parent: wx.Window, collis_box_model: CollisBoxModel):
        super().__init__(parent, style=wx.VSCROLL | wx.HSCROLL | wx.NO_BORDER)
        self.SetBackgroundStyle(wx.BG_STYLE_PAINT)
        self.SetBackgroundColour(wx.Colour(40, 42, 48))
        self.SetDoubleBuffered(True)

        self.model = collis_box_model
        self.pil_image: Optional[Image.Image] = None
        self._cached_bitmap: Optional[wx.Bitmap] = None
        self._cached_zoom: float = 1.0

        self.zoom: float = 1.0
        self.mode: str = self.MODE_INSPECT
        self.selected_point_idx: int = -1
        self.hover_point_idx: int = -1
        self.dragging_point_idx: int = -1
        self._drag_snapshot_taken: bool = False

        # Cutting tool state
        self.cut_rect: Optional[list] = None  # [x, y, w, h] in image pixel coordinates
        self.preserve_aspect_ratio_in_cut: bool = False
        self._cut_aspect_ratio: float = 1.0
        self._cut_drag_part: Optional[str] = None
        self._cut_hover_part: Optional[str] = None
        self._cut_drag_start_mouse = wx.Point(0, 0)
        self._cut_drag_start_rect: Optional[list] = None

        # Pan state
        self._panning: bool = False
        self._pan_start_mouse = wx.Point(0, 0)
        self._pan_start_scroll = (0, 0)

        # Callbacks
        self.on_mouse_pos_change: Optional[Callable[[int, int], None]] = None
        self.on_point_selected: Optional[Callable[[int], None]] = None
        self.on_trans_col_picked: Optional[Callable[[int, int, Tuple[int, int, int], int], None]] = None

        # Listen to model changes
        self.model.add_listener(self._on_model_changed)

        # Event bindings
        self.Bind(wx.EVT_PAINT, self._on_paint)
        self.Bind(wx.EVT_ERASE_BACKGROUND, self._on_erase_background)
        self.Bind(wx.EVT_SIZE, self._on_size)
        self.Bind(wx.EVT_MOUSEWHEEL, self._on_mouse_wheel)
        self.Bind(wx.EVT_LEFT_DOWN, self._on_left_down)
        self.Bind(wx.EVT_LEFT_UP, self._on_left_up)
        self.Bind(wx.EVT_RIGHT_DOWN, self._on_right_down)
        self.Bind(wx.EVT_RIGHT_UP, self._on_right_up)
        self.Bind(wx.EVT_MOTION, self._on_motion)
        self.Bind(wx.EVT_LEAVE_WINDOW, self._on_leave)

    def start_cut_mode(self, preserve_aspect: bool = False) -> None:
        """Activate cut tool mode with interactive crop frame."""
        self.mode = self.MODE_CUT
        self.preserve_aspect_ratio_in_cut = preserve_aspect
        if self.pil_image:
            iw, ih = self.pil_image.size
            self._cut_aspect_ratio = (iw / ih) if ih > 0 else 1.0
            pad_x = max(1, iw // 10)
            pad_y = max(1, ih // 10)
            init_w = max(1, iw - 2 * pad_x)
            init_h = max(1, ih - 2 * pad_y)
            if preserve_aspect and self._cut_aspect_ratio > 0:
                init_h = max(1, min(ih - pad_y, round(init_w / self._cut_aspect_ratio)))
            self.cut_rect = [pad_x, pad_y, init_w, init_h]
        self._cut_drag_part = None
        self._cut_hover_part = None
        self._update_cursor()
        self.Refresh()

    def end_cut_mode(self) -> None:
        """Deactivate cut tool mode and clear crop frame."""
        self.mode = self.MODE_INSPECT
        self.cut_rect = None
        self._cut_drag_part = None
        self._cut_hover_part = None
        if self.HasCapture():
            self.ReleaseMouse()
        self._update_cursor()
        self.Refresh()

    def set_preserve_aspect_in_cut(self, preserve: bool) -> None:
        """Update aspect ratio preservation setting during cut mode."""
        self.preserve_aspect_ratio_in_cut = preserve
        if preserve and self.cut_rect and self.pil_image:
            iw, ih = self.pil_image.size
            rx, ry, rw, _ = self.cut_rect
            if self._cut_aspect_ratio > 0:
                new_h = max(1, min(ih - ry, round(rw / self._cut_aspect_ratio)))
                self.cut_rect[3] = new_h
                self.Refresh()

    def get_cut_rect(self) -> Optional[Tuple[int, int, int, int]]:
        """Return (x, y, w, h) of current crop frame or None."""
        if self.cut_rect and len(self.cut_rect) == 4 and self.cut_rect[2] > 0 and self.cut_rect[3] > 0:
            return (int(self.cut_rect[0]), int(self.cut_rect[1]), int(self.cut_rect[2]), int(self.cut_rect[3]))
        return None

    def _hit_test_cut_frame(self, pt: wx.Point) -> str:
        """Determine which part of crop frame is under mouse."""
        if not self.cut_rect or not self.pil_image:
            return "NEW"

        rx, ry, rw, rh = self.cut_rect
        dl, dt = self.image_to_doc_coords(rx, ry, center_in_pixel=False)
        dr, db = self.image_to_doc_coords(rx + rw, ry + rh, center_in_pixel=False)

        vs_x, vs_y = self.GetViewStart()
        ppu_x, ppu_y = self.GetScrollPixelsPerUnit()
        mx = vs_x * ppu_x + pt.x
        my = vs_y * ppu_y + pt.y

        tol = 10.0

        # Corners
        if math.hypot(mx - dl, my - dt) <= tol:
            return "TL"
        if math.hypot(mx - dr, my - dt) <= tol:
            return "TR"
        if math.hypot(mx - dl, my - db) <= tol:
            return "BL"
        if math.hypot(mx - dr, my - db) <= tol:
            return "BR"

        # Borders
        if abs(my - dt) <= tol and dl - tol <= mx <= dr + tol:
            return "T"
        if abs(my - db) <= tol and dl - tol <= mx <= dr + tol:
            return "B"
        if abs(mx - dl) <= tol and dt - tol <= my <= db + tol:
            return "L"
        if abs(mx - dr) <= tol and dt - tol <= my <= db + tol:
            return "R"

        # Inside vs Outside
        if dl < mx < dr and dt < my < db:
            return "INSIDE"

        return "NEW"

    def set_image(self, pil_image: Optional[Image.Image], reset_zoom: bool = False) -> None:
        """Set or update displayed PIL image."""
        self.pil_image = pil_image
        self._cached_bitmap = None
        if reset_zoom or self.zoom <= 0:
            self.zoom = 1.0
        self._update_virtual_size()
        self.Refresh()

    def set_mode(self, mode: str) -> None:
        """Set interaction mode (MODE_INSPECT, MODE_ADD, MODE_REMOVE, MODE_PICK_TRANS_COL, MODE_CUT)."""
        self.mode = mode
        self._update_cursor()
        self.Refresh()

    def set_zoom(self, zoom: float, anchor_pt: Optional[wx.Point] = None) -> None:
        """Set zoom factor clamped between 0.1 and 32.0."""
        new_zoom = max(0.1, min(32.0, zoom))
        if abs(new_zoom - self.zoom) < 1e-4:
            return

        old_zoom = self.zoom
        self.zoom = new_zoom
        self._cached_bitmap = None

        # Adjust scroll to keep anchor position centered
        if anchor_pt is not None:
            view_start_x, view_start_y = self.GetViewStart()
            ppu_x, ppu_y = self.GetScrollPixelsPerUnit()
            if ppu_x > 0 and ppu_y > 0:
                doc_x = (view_start_x * ppu_x + anchor_pt.x) / old_zoom
                doc_y = (view_start_y * ppu_y + anchor_pt.y) / old_zoom
                self._update_virtual_size()
                new_scroll_x = int(round((doc_x * new_zoom - anchor_pt.x) / ppu_x))
                new_scroll_y = int(round((doc_y * new_zoom - anchor_pt.y) / ppu_y))
                self.Scroll(max(0, new_scroll_x), max(0, new_scroll_y))
            else:
                self._update_virtual_size()
        else:
            self._update_virtual_size()

        self.Refresh()

    def fit_to_window(self) -> None:
        """Fit image inside client area."""
        if not self.pil_image:
            return
        cw, ch = self.GetClientSize()
        iw, ih = self.pil_image.size
        if iw <= 0 or ih <= 0 or cw <= 0 or ch <= 0:
            return
        scale_x = (cw - 20) / iw
        scale_y = (ch - 20) / ih
        self.set_zoom(min(scale_x, scale_y))

    def _update_virtual_size(self) -> None:
        """Update virtual scroll size based on image size and zoom factor."""
        if not self.pil_image:
            self.SetVirtualSize(1, 1)
            self.SetScrollRate(10, 10)
            return

        iw, ih = self.pil_image.size
        cw, ch = self.GetClientSize()
        disp_w = int(round(iw * self.zoom))
        disp_h = int(round(ih * self.zoom))

        total_w = max(disp_w + 40, cw)
        total_h = max(disp_h + 40, ch)

        self.SetVirtualSize(total_w, total_h)
        self.SetScrollRate(10, 10)

    def _get_image_draw_offset(self) -> Tuple[int, int]:
        """Calculate top-left coordinate of image inside virtual canvas."""
        if not self.pil_image:
            return 0, 0
        iw, ih = self.pil_image.size
        disp_w = int(round(iw * self.zoom))
        disp_h = int(round(ih * self.zoom))
        vw, vh = self.GetVirtualSize()

        ox = max(20, (vw - disp_w) // 2)
        oy = max(20, (vh - disp_h) // 2)
        return ox, oy

    def client_to_image_coords(self, pt: wx.Point) -> Tuple[int, int]:
        """Convert client window coordinates to image pixel coordinates."""
        if not self.pil_image:
            return 0, 0
        vs_x, vs_y = self.GetViewStart()
        ppu_x, ppu_y = self.GetScrollPixelsPerUnit()
        doc_x = vs_x * ppu_x + pt.x
        doc_y = vs_y * ppu_y + pt.y

        ox, oy = self._get_image_draw_offset()
        img_x = int(math.floor((doc_x - ox) / self.zoom))
        img_y = int(math.floor((doc_y - oy) / self.zoom))
        return img_x, img_y

    def image_to_doc_coords(self, ix: float, iy: float, center_in_pixel: bool = True) -> Tuple[float, float]:
        """
        Convert image pixel coordinates to virtual document coordinates.
        Points describe pixels where (0,0) is the top-left pixel. When center_in_pixel
        is True (default), the coordinate is positioned in the exact middle of the pixel
        (ix + 0.5, iy + 0.5).
        """
        ox, oy = self._get_image_draw_offset()
        offset = 0.5 if center_in_pixel else 0.0
        return ox + (ix + offset) * self.zoom, oy + (iy + offset) * self.zoom

    def find_point_near_client_coords(self, pt: wx.Point, threshold_px: float = 10.0) -> int:
        """Find index of point whose drawn handle center is within threshold screen pixels."""
        if not self.model.points:
            return -1
        vs_x, vs_y = self.GetViewStart()
        ppu_x, ppu_y = self.GetScrollPixelsPerUnit()
        doc_x = vs_x * ppu_x + pt.x
        doc_y = vs_y * ppu_y + pt.y

        best_idx = -1
        best_dist = threshold_px
        for i, (px, py) in enumerate(self.model.points):
            dx, dy = self.image_to_doc_coords(px, py, center_in_pixel=True)
            dist = math.hypot(doc_x - dx, doc_y - dy)
            if dist <= best_dist:
                best_dist = dist
                best_idx = i
        return best_idx

    def _update_cursor(self) -> None:
        """Update mouse cursor according to current mode, hover state, and cut frame handles."""
        if self._panning:
            self.SetCursor(wx.Cursor(wx.CURSOR_SIZING))
            return

        if self.mode == self.MODE_CUT:
            part = self._cut_drag_part or self._cut_hover_part or "NEW"
            if part in ("TL", "BR"):
                self.SetCursor(wx.Cursor(wx.CURSOR_SIZENWSE))
            elif part in ("TR", "BL"):
                self.SetCursor(wx.Cursor(wx.CURSOR_SIZENESW))
            elif part in ("T", "B"):
                self.SetCursor(wx.Cursor(wx.CURSOR_SIZENS))
            elif part in ("L", "R"):
                self.SetCursor(wx.Cursor(wx.CURSOR_SIZEWE))
            elif part == "INSIDE":
                self.SetCursor(wx.Cursor(wx.CURSOR_SIZING))
            else:
                self.SetCursor(wx.Cursor(wx.CURSOR_CROSS))
            return

        if self.mode == self.MODE_PICK_TRANS_COL:
            self.SetCursor(wx.Cursor(wx.CURSOR_CROSS))
        elif self.dragging_point_idx >= 0:
            self.SetCursor(wx.Cursor(wx.CURSOR_HAND))
        elif self.hover_point_idx >= 0:
            if self.mode == self.MODE_REMOVE:
                self.SetCursor(wx.Cursor(wx.CURSOR_BULLSEYE))
            else:
                self.SetCursor(wx.Cursor(wx.CURSOR_HAND))
        elif self.mode == self.MODE_ADD:
            self.SetCursor(wx.Cursor(wx.CURSOR_CROSS))
        elif self.mode == self.MODE_REMOVE:
            self.SetCursor(wx.Cursor(wx.CURSOR_BULLSEYE))
        else:
            self.SetCursor(wx.Cursor(wx.CURSOR_ARROW))

    def _on_model_changed(self) -> None:
        """Refresh canvas when collis-box model updates."""
        if self.selected_point_idx >= len(self.model.points):
            self.selected_point_idx = len(self.model.points) - 1
        self.Refresh()

    def _on_erase_background(self, event):
        pass

    def _on_size(self, event):
        self._update_virtual_size()
        event.Skip()

    def _on_mouse_wheel(self, event: wx.MouseEvent):
        if event.ControlDown():
            rot = event.GetWheelRotation()
            scale = 1.2 if rot > 0 else (1.0 / 1.2)
            self.set_zoom(self.zoom * scale, anchor_pt=event.GetPosition())
        else:
            event.Skip()

    def _on_right_down(self, event: wx.MouseEvent):
        self._panning = True
        self._pan_start_mouse = event.GetPosition()
        self._pan_start_scroll = self.GetViewStart()
        self._update_cursor()
        if not self.HasCapture():
            self.CaptureMouse()

    def _on_right_up(self, event: wx.MouseEvent):
        if self._panning:
            self._panning = False
            if self.HasCapture():
                self.ReleaseMouse()
            self._update_cursor()

    def _on_left_down(self, event: wx.MouseEvent):
        if not self.pil_image:
            return

        pt = event.GetPosition()
        ix, iy = self.client_to_image_coords(pt)
        iw, ih = self.pil_image.size

        # Mode: Cut / Crop Image
        if self.mode == self.MODE_CUT:
            part = self._hit_test_cut_frame(pt)
            if part == "NEW":
                cix = max(0, min(iw - 1, ix))
                ciy = max(0, min(ih - 1, iy))
                self.cut_rect = [cix, ciy, 1, 1]
                self._cut_drag_part = "BR"
                self._cut_drag_start_mouse = pt
                self._cut_drag_start_rect = list(self.cut_rect)
            else:
                self._cut_drag_part = part
                self._cut_drag_start_mouse = pt
                self._cut_drag_start_rect = list(self.cut_rect) if self.cut_rect else [0, 0, iw, ih]

            if not self.HasCapture():
                self.CaptureMouse()
            self._update_cursor()
            self.Refresh()
            return

        # Mode: Pick Transparent Color
        if self.mode == self.MODE_PICK_TRANS_COL:
            if 0 <= ix < iw and 0 <= iy < ih:
                r, g, b = self.pil_image.getpixel((ix, iy))[:3]
                rgb888 = (r << 16) | (g << 8) | b
                if self.on_trans_col_picked:
                    self.on_trans_col_picked(ix, iy, (r, g, b), rgb888)
            return

        # Check if clicked near an existing point
        closest = self.find_point_near_client_coords(pt, threshold_px=10.0)

        # In Remove mode: click removes clicked point
        if self.mode == self.MODE_REMOVE:
            if closest >= 0:
                self.model.remove_point(closest)
                self.selected_point_idx = -1
                if self.on_point_selected:
                    self.on_point_selected(-1)
            return

        # If an existing point was clicked in Add or Inspect mode: select and prepare to drag
        if closest >= 0:
            self.selected_point_idx = closest
            self.dragging_point_idx = closest
            self._drag_snapshot_taken = False
            if not self.HasCapture():
                self.CaptureMouse()
            if self.on_point_selected:
                self.on_point_selected(closest)
            self._update_cursor()
            self.Refresh()
            return

        # In Add mode: click on empty image space adds a new point to collis-box
        if self.mode == self.MODE_ADD:
            if 0 <= ix < iw and 0 <= iy < ih:
                self.model.add_point(ix, iy)
                new_idx = len(self.model.points) - 1
                self.selected_point_idx = new_idx
                self.dragging_point_idx = new_idx
                self._drag_snapshot_taken = True
                if not self.HasCapture():
                    self.CaptureMouse()
                if self.on_point_selected:
                    self.on_point_selected(new_idx)
                self._update_cursor()
                self.Refresh()
                return

    def _on_left_up(self, event: wx.MouseEvent):
        if self.mode == self.MODE_CUT:
            if self._cut_drag_part:
                self._cut_drag_part = None
                self._cut_drag_start_rect = None
                if self.HasCapture():
                    self.ReleaseMouse()
                if self.cut_rect and self.pil_image:
                    iw, ih = self.pil_image.size
                    x, y, w, h = self.cut_rect
                    if w < 0:
                        x = x + w
                        w = abs(w)
                    if h < 0:
                        y = y + h
                        h = abs(h)
                    x = max(0, min(iw - 1, x))
                    y = max(0, min(ih - 1, y))
                    w = max(1, min(iw - x, w))
                    h = max(1, min(ih - y, h))
                    self.cut_rect = [x, y, w, h]
                self._cut_hover_part = self._hit_test_cut_frame(event.GetPosition())
                self._update_cursor()
                self.Refresh()
            return

        if self.dragging_point_idx >= 0:
            self.dragging_point_idx = -1
            self._drag_snapshot_taken = False
            if self.HasCapture():
                self.ReleaseMouse()
            self._update_cursor()
            self.Refresh()

    def _on_motion(self, event: wx.MouseEvent):
        pt = event.GetPosition()

        # Handle panning
        if self._panning:
            dx = pt.x - self._pan_start_mouse.x
            dy = pt.y - self._pan_start_mouse.y
            ppu_x, ppu_y = self.GetScrollPixelsPerUnit()
            if ppu_x > 0 and ppu_y > 0:
                new_sx = self._pan_start_scroll[0] - int(dx / ppu_x)
                new_sy = self._pan_start_scroll[1] - int(dy / ppu_y)
                self.Scroll(max(0, new_sx), max(0, new_sy))
            return

        if not self.pil_image:
            return

        ix, iy = self.client_to_image_coords(pt)
        iw, ih = self.pil_image.size

        if 0 <= ix < iw and 0 <= iy < ih:
            if self.on_mouse_pos_change:
                self.on_mouse_pos_change(ix, iy)

        # Handle cutting mode dragging & hovering
        if self.mode == self.MODE_CUT:
            if event.LeftIsDown() and self._cut_drag_part and self.cut_rect and self._cut_drag_start_rect:
                dx = int(round((pt.x - self._cut_drag_start_mouse.x) / self.zoom))
                dy = int(round((pt.y - self._cut_drag_start_mouse.y) / self.zoom))
                sx, sy, sw, sh = self._cut_drag_start_rect
                part = self._cut_drag_part

                if part == "INSIDE":
                    nx = max(0, min(iw - sw, sx + dx))
                    ny = max(0, min(ih - sh, sy + dy))
                    self.cut_rect = [nx, ny, sw, sh]
                else:
                    x1, y1 = sx, sy
                    x2, y2 = sx + sw, sy + sh

                    if "L" in part:
                        x1 = min(sx + sw - 1, max(0, sx + dx))
                    if "R" in part:
                        x2 = max(sx + 1, min(iw, sx + sw + dx))
                    if "T" in part:
                        y1 = min(sy + sh - 1, max(0, sy + dy))
                    if "B" in part:
                        y2 = max(sy + 1, min(ih, sy + sh + dy))

                    cur_w = x2 - x1
                    cur_h = y2 - y1

                    if self.preserve_aspect_ratio_in_cut and self._cut_aspect_ratio > 0:
                        ratio = self._cut_aspect_ratio
                        if part in ("L", "R"):
                            cur_h = max(1, min(ih, round(cur_w / ratio)))
                            y1 = max(0, min(ih - cur_h, sy + (sh - cur_h) // 2))
                            y2 = y1 + cur_h
                        elif part in ("T", "B"):
                            cur_w = max(1, min(iw, round(cur_h * ratio)))
                            x1 = max(0, min(iw - cur_w, sx + (sw - cur_w) // 2))
                            x2 = x1 + cur_w
                        else:
                            adj_h = max(1, round(cur_w / ratio))
                            if "T" in part:
                                new_y1 = y2 - adj_h
                                if new_y1 < 0:
                                    adj_h = y2
                                    cur_w = max(1, round(adj_h * ratio))
                                    if "L" in part:
                                        x1 = x2 - cur_w
                                    else:
                                        x2 = x1 + cur_w
                                    new_y1 = y2 - adj_h
                                y1 = max(0, new_y1)
                            else:
                                new_y2 = y1 + adj_h
                                if new_y2 > ih:
                                    adj_h = ih - y1
                                    cur_w = max(1, round(adj_h * ratio))
                                    if "L" in part:
                                        x1 = x2 - cur_w
                                    else:
                                        x2 = x1 + cur_w
                                    new_y2 = y1 + adj_h
                                y2 = min(ih, new_y2)

                    self.cut_rect = [x1, y1, max(1, x2 - x1), max(1, y2 - y1)]

                self.Refresh()
                return
            else:
                hover_part = self._hit_test_cut_frame(pt)
                if hover_part != self._cut_hover_part:
                    self._cut_hover_part = hover_part
                    self._update_cursor()
                return

        # Handle point dragging if active
        if event.LeftIsDown() and self.dragging_point_idx >= 0:
            clamped_x = max(0, min(iw - 1, ix))
            clamped_y = max(0, min(ih - 1, iy))
            if not self._drag_snapshot_taken:
                self.model.save_undo_snapshot()
                self._drag_snapshot_taken = True
            self.model.update_point_position(self.dragging_point_idx, clamped_x, clamped_y, record_history=False)
            return

        # Check hover point for visual handle highlight and hand cursor
        closest = self.find_point_near_client_coords(pt, threshold_px=10.0)
        if closest != self.hover_point_idx:
            self.hover_point_idx = closest
            self._update_cursor()
            self.Refresh()

    def _on_leave(self, event):
        self.hover_point_idx = -1
        self._cut_hover_part = None
        self._update_cursor()
        self.Refresh()
        event.Skip()

    def _get_render_bitmap(self) -> Optional[wx.Bitmap]:
        """Convert PIL Image to wx.Bitmap, scaling according to zoom."""
        if not self.pil_image:
            return None

        iw, ih = self.pil_image.size
        dw = int(round(iw * self.zoom))
        dh = int(round(ih * self.zoom))

        if self._cached_bitmap and abs(self._cached_zoom - self.zoom) < 1e-4:
            return self._cached_bitmap

        if self.zoom >= 1.0:
            resized_pil = self.pil_image.resize((dw, dh), resample=Image.Resampling.NEAREST)
        else:
            resized_pil = self.pil_image.resize((dw, dh), resample=Image.Resampling.BILINEAR)

        wx_img = wx.Image(dw, dh)
        wx_img.SetData(resized_pil.convert("RGB").tobytes())
        self._cached_bitmap = wx.Bitmap(wx_img)
        self._cached_zoom = self.zoom
        return self._cached_bitmap

    def _on_paint(self, event):
        dc = wx.AutoBufferedPaintDC(self)
        self.DoPrepareDC(dc)

        # Background
        dc.SetBackground(wx.Brush(wx.Colour(35, 37, 42)))
        dc.Clear()

        if not self.pil_image:
            dc.SetTextForeground(wx.Colour(160, 160, 160))
            font = wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
            dc.SetFont(font)
            dc.DrawText("No image opened. Click 'Import Image...' in the toolbar to open an image.", 40, 40)
            return

        ox, oy = self._get_image_draw_offset()
        bmp = self._get_render_bitmap()
        if bmp:
            dc.DrawBitmap(bmp, ox, oy, True)

            dc.SetPen(wx.Pen(wx.Colour(80, 80, 80), 1))
            dc.SetBrush(wx.Brush(wx.Colour(0, 0, 0), wx.BRUSHSTYLE_TRANSPARENT))
            dc.DrawRectangle(ox - 1, oy - 1, bmp.GetWidth() + 2, bmp.GetHeight() + 2)

        # Draw Collis-box polygon
        self._draw_collis_box(dc, ox, oy)

        # Draw cut overlay if in cut mode
        if self.mode == self.MODE_CUT and self.cut_rect:
            self._draw_cut_overlay(dc, ox, oy)

    def _draw_cut_overlay(self, dc: wx.DC, ox: int, oy: int) -> None:
        """Draw crop frame rectangle, handles, dimension badge, and darkened surrounding area."""
        if not self.cut_rect or not self.pil_image:
            return

        rx, ry, rw, rh = self.cut_rect
        iw, ih = self.pil_image.size

        dl_f, dt_f = self.image_to_doc_coords(rx, ry, center_in_pixel=False)
        dr_f, db_f = self.image_to_doc_coords(rx + rw, ry + rh, center_in_pixel=False)

        dl = int(round(dl_f))
        dt = int(round(dt_f))
        dr = int(round(dr_f))
        db = int(round(db_f))

        im_l = ox
        im_t = oy
        im_r = ox + int(round(iw * self.zoom))
        im_b = oy + int(round(ih * self.zoom))

        # Clamp bounds
        dl = max(im_l, min(im_r, dl))
        dt = max(im_t, min(im_b, dt))
        dr = max(im_l, min(im_r, dr))
        db = max(im_t, min(im_b, db))

        # Semi-transparent overlay surrounding the crop area
        try:
            gcdc = wx.GCDC(dc)
            gcdc.SetPen(wx.Pen(wx.Colour(0, 0, 0, 0), 0, wx.PENSTYLE_TRANSPARENT))
            gcdc.SetBrush(wx.Brush(wx.Colour(0, 0, 0, 150)))
            # Top
            if dt > im_t:
                gcdc.DrawRectangle(im_l, im_t, im_r - im_l, dt - im_t)
            # Bottom
            if db < im_b:
                gcdc.DrawRectangle(im_l, db, im_r - im_l, im_b - db)
            # Left
            if dl > im_l:
                gcdc.DrawRectangle(im_l, dt, dl - im_l, db - dt)
            # Right
            if dr < im_r:
                gcdc.DrawRectangle(dr, dt, im_r - dr, db - dt)
        except Exception:
            pass

        # Border for crop rectangle
        dc.SetBrush(wx.Brush(wx.Colour(0, 0, 0), wx.BRUSHSTYLE_TRANSPARENT))
        dc.SetPen(wx.Pen(wx.Colour(20, 20, 20), 2))
        dc.DrawRectangle(dl - 1, dt - 1, (dr - dl) + 2, (db - dt) + 2)
        dc.SetPen(wx.Pen(wx.Colour(0, 220, 255), 2))
        dc.DrawRectangle(dl, dt, dr - dl, db - dt)

        # 8 sizing handles
        handle_size = 8
        hs = handle_size // 2
        mid_x = (dl + dr) // 2
        mid_y = (dt + db) // 2
        handles = [
            (dl, dt), (mid_x, dt), (dr, dt),
            (dl, mid_y),            (dr, mid_y),
            (dl, db), (mid_x, db), (dr, db),
        ]

        dc.SetPen(wx.Pen(wx.Colour(30, 30, 30), 1))
        dc.SetBrush(wx.Brush(wx.Colour(255, 255, 255)))
        for hx, hy in handles:
            dc.DrawRectangle(hx - hs, hy - hs, handle_size, handle_size)

        # Dimensions badge
        dim_text = f"{rw} × {rh} px"
        font = wx.Font(9, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
        dc.SetFont(font)
        tw, th = dc.GetTextExtent(dim_text)

        badge_x = dl + 6
        badge_y = dt + 6
        if badge_y + th + 6 > db:
            badge_y = max(im_t + 2, dt - th - 8)

        dc.SetPen(wx.Pen(wx.Colour(10, 10, 10), 1))
        dc.SetBrush(wx.Brush(wx.Colour(30, 32, 40)))
        dc.DrawRoundedRectangle(badge_x - 3, badge_y - 2, tw + 6, th + 4, 3)
        dc.SetTextForeground(wx.Colour(255, 255, 255))
        dc.DrawText(dim_text, badge_x, badge_y)

    def _draw_collis_box(self, dc: wx.DC, ox: int, oy: int) -> None:
        """Draw vertices and connecting lines of the collis-box polygon."""
        points = self.model.points
        if not points:
            return

        is_convex_poly = self.model.is_convex()
        is_ccw = self.model.is_counter_clockwise(screen_coords=True)

        if len(points) >= 3 and is_convex_poly and is_ccw:
            line_color = wx.Colour(0, 230, 120)       # Healthy emerald
        elif len(points) >= 3:
            line_color = wx.Colour(255, 170, 0)       # Warning amber
        else:
            line_color = wx.Colour(0, 200, 255)       # Cyan drafting

        # Convert points to document coordinates centered in pixels
        doc_pts = [self.image_to_doc_coords(p[0], p[1], center_in_pixel=True) for p in points]
        wx_pts = [wx.Point(int(round(p[0])), int(round(p[1]))) for p in doc_pts]

        # Draw closed polygon connecting lines
        if len(wx_pts) >= 2:
            dc.SetPen(wx.Pen(line_color, 2))
            for i in range(len(wx_pts)):
                p1 = wx_pts[i]
                p2 = wx_pts[(i + 1) % len(wx_pts)]
                dc.DrawLine(p1.x, p1.y, p2.x, p2.y)

        # Draw vertex points and labels
        font = wx.Font(8, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
        dc.SetFont(font)

        handle_radius = max(4, min(8, int(round(4 * math.sqrt(self.zoom)))))

        for i, (px, py) in enumerate(wx_pts):
            is_selected = (i == self.selected_point_idx)
            is_hover = (i == self.hover_point_idx)

            if is_selected:
                pt_pen = wx.Pen(wx.Colour(255, 255, 255), 2)
                pt_brush = wx.Brush(wx.Colour(255, 215, 0))   # Gold
                r = handle_radius + 3
            elif is_hover and self.mode == self.MODE_REMOVE:
                pt_pen = wx.Pen(wx.Colour(255, 255, 255), 2)
                pt_brush = wx.Brush(wx.Colour(255, 60, 60))    # Red on remove hover
                r = handle_radius + 2
            else:
                pt_pen = wx.Pen(wx.Colour(20, 20, 20), 1)
                pt_brush = wx.Brush(line_color)
                r = handle_radius

            dc.SetPen(pt_pen)
            dc.SetBrush(pt_brush)
            dc.DrawCircle(px, py, r)

            lbl = str(i + 1)
            tw, th = dc.GetTextExtent(lbl)
            dc.SetTextForeground(wx.Colour(255, 255, 255))
            dc.DrawText(lbl, px + r + 3, py - th // 2)
