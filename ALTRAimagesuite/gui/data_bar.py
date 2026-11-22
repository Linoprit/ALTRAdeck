"""Right databar displaying collis-box point table or image metadata."""

import json
from typing import Callable, List, Optional, Tuple
import wx

from app_parts.collis_box.collis_box_model import CollisBoxModel


class DataBar(wx.Panel):
    """
    Right sidebar panel.
    - When Draw Collis-box tool is active: shows table with points in order, with Up/Down reordering buttons.
    - When Image Conversion tool is active: shows metadata representing the JSON file and image properties.
    """

    def __init__(self, parent: wx.Window, collis_box_model: CollisBoxModel):
        super().__init__(parent, style=wx.BORDER_THEME)
        self.SetBackgroundColour(wx.Colour(45, 47, 52))

        self.model = collis_box_model
        self.on_point_selected_in_table: Optional[Callable[[int], None]] = None

        self._updating_table = False
        self._current_width = 0
        self._current_height = 0
        self._current_byteorder = "little"
        self._current_trans_col = 0

        self._init_ui()

        # Listen to model updates
        self.model.add_listener(self._on_model_updated)

    def _init_ui(self) -> None:
        self.main_sizer = wx.BoxSizer(wx.VERTICAL)

        # Container for swappable views
        self.container = wx.ScrolledWindow(self, style=wx.VSCROLL)
        self.container.SetScrollRate(5, 5)
        self.container_sizer = wx.BoxSizer(wx.VERTICAL)
        self.container.SetSizer(self.container_sizer)

        # --- View 1: Collis-box Points Table View ---
        self.panel_collisbox = wx.Panel(self.container)
        cb_sizer = wx.BoxSizer(wx.VERTICAL)

        lbl_cb = wx.StaticText(self.panel_collisbox, label="COLLIS-BOX POINTS")
        lbl_cb.SetForegroundColour(wx.Colour(220, 225, 235))
        lbl_cb.SetFont(lbl_cb.GetFont().Bold())
        cb_sizer.Add(lbl_cb, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 6)

        # Reorder buttons above table
        btn_box = wx.BoxSizer(wx.HORIZONTAL)
        self.btn_up = wx.Button(self.panel_collisbox, label="▲ Move Up")
        self.btn_down = wx.Button(self.panel_collisbox, label="▼ Move Down")
        self.btn_del = wx.Button(self.panel_collisbox, label="✕ Delete")

        btn_box.Add(self.btn_up, 1, wx.RIGHT, 2)
        btn_box.Add(self.btn_down, 1, wx.RIGHT, 2)
        btn_box.Add(self.btn_del, 1)
        cb_sizer.Add(btn_box, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 6)

        # Points ListCtrl
        self.list_points = wx.ListCtrl(
            self.panel_collisbox,
            style=wx.LC_REPORT | wx.LC_SINGLE_SEL | wx.BORDER_SUNKEN
        )
        self.list_points.InsertColumn(0, "#", width=40)
        self.list_points.InsertColumn(1, "X (px)", width=75)
        self.list_points.InsertColumn(2, "Y (px)", width=75)
        cb_sizer.Add(self.list_points, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 6)

        # Polygon Analysis / Validation Box
        self.stat_box = wx.StaticBox(self.panel_collisbox, label="Polygon Analysis")
        stat_sizer = wx.StaticBoxSizer(self.stat_box, wx.VERTICAL)

        self.lbl_pt_count = wx.StaticText(self.panel_collisbox, label="Points: 0")
        self.lbl_convex = wx.StaticText(self.panel_collisbox, label="Convex: -")
        self.lbl_winding = wx.StaticText(self.panel_collisbox, label="Order: -")

        stat_sizer.Add(self.lbl_pt_count, 0, wx.ALL, 3)
        stat_sizer.Add(self.lbl_convex, 0, wx.ALL, 3)
        stat_sizer.Add(self.lbl_winding, 0, wx.ALL, 3)
        cb_sizer.Add(stat_sizer, 0, wx.EXPAND | wx.ALL, 6)

        self.panel_collisbox.SetSizer(cb_sizer)

        # --- View 2: Image Metadata View ---
        self.panel_meta = wx.Panel(self.container)
        meta_sizer = wx.BoxSizer(wx.VERTICAL)

        lbl_meta = wx.StaticText(self.panel_meta, label="IMAGE METADATA")
        lbl_meta.SetForegroundColour(wx.Colour(220, 225, 235))
        lbl_meta.SetFont(lbl_meta.GetFont().Bold())
        meta_sizer.Add(lbl_meta, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 6)

        # Metadata grid
        meta_box = wx.StaticBox(self.panel_meta, label="Properties")
        mgrid_sizer = wx.StaticBoxSizer(meta_box, wx.VERTICAL)

        self.lbl_dim = wx.StaticText(self.panel_meta, label="Dimensions: -")
        self.lbl_fmt = wx.StaticText(self.panel_meta, label="Format: RGB565 (16-bit)")
        self.lbl_order = wx.StaticText(self.panel_meta, label="Byte-Order: Little-Endian")
        self.lbl_size = wx.StaticText(self.panel_meta, label="Framebuffer Size: -")

        # Transparent color display with swatch
        trans_box = wx.BoxSizer(wx.HORIZONTAL)
        self.lbl_trans_col = wx.StaticText(self.panel_meta, label="trans_col: 0 (0x000000)")
        self.swatch_trans = wx.Panel(self.panel_meta, size=(16, 16), style=wx.BORDER_SIMPLE)
        self.swatch_trans.SetBackgroundColour(wx.Colour(0, 0, 0))

        trans_box.Add(self.lbl_trans_col, 1, wx.ALIGN_CENTER_VERTICAL)
        trans_box.Add(self.swatch_trans, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 6)

        self.lbl_cb_count = wx.StaticText(self.panel_meta, label="collis_box points: 0")

        mgrid_sizer.Add(self.lbl_dim, 0, wx.ALL, 4)
        mgrid_sizer.Add(self.lbl_fmt, 0, wx.ALL, 4)
        mgrid_sizer.Add(self.lbl_order, 0, wx.ALL, 4)
        mgrid_sizer.Add(self.lbl_size, 0, wx.ALL, 4)
        mgrid_sizer.Add(trans_box, 0, wx.EXPAND | wx.ALL, 4)
        mgrid_sizer.Add(self.lbl_cb_count, 0, wx.ALL, 4)
        meta_sizer.Add(mgrid_sizer, 0, wx.EXPAND | wx.ALL, 6)

        # JSON Preview Box
        json_box = wx.StaticBox(self.panel_meta, label="JSON Metadata Preview")
        json_sizer = wx.StaticBoxSizer(json_box, wx.VERTICAL)

        self.txt_json = wx.TextCtrl(
            self.panel_meta,
            style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL
        )
        mono_font = wx.Font(9, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
        self.txt_json.SetFont(mono_font)
        json_sizer.Add(self.txt_json, 1, wx.EXPAND | wx.ALL, 4)
        meta_sizer.Add(json_sizer, 1, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 6)

        self.panel_meta.SetSizer(meta_sizer)

        # Add both panels to container
        self.container_sizer.Add(self.panel_collisbox, 1, wx.EXPAND)
        self.container_sizer.Add(self.panel_meta, 1, wx.EXPAND)
        self.panel_collisbox.Hide()

        self.main_sizer.Add(self.container, 1, wx.EXPAND)
        self.SetSizer(self.main_sizer)

        # Event bindings
        self.btn_up.Bind(wx.EVT_BUTTON, self._on_move_up)
        self.btn_down.Bind(wx.EVT_BUTTON, self._on_move_down)
        self.btn_del.Bind(wx.EVT_BUTTON, self._on_delete)
        self.list_points.Bind(wx.EVT_LIST_ITEM_SELECTED, self._on_list_selected)

    def show_tool_view(self, tool: str) -> None:
        """Switch between collis-box point table and metadata view."""
        if tool == "collisbox":
            self.panel_collisbox.Show()
            self.panel_meta.Hide()
        else:
            self.panel_collisbox.Hide()
            self.panel_meta.Show()
        self.container.Layout()
        self.Layout()

    def update_metadata(self, width: int, height: int, byteorder: str = "little", trans_col: int = 0) -> None:
        """Update image metadata view and JSON preview."""
        self._current_width = width
        self._current_height = height
        self._current_byteorder = byteorder
        self._current_trans_col = trans_col

        data_bytes = width * height * 2
        bo_str = "Little-Endian (ARM native)" if byteorder == "little" else "Big-Endian (SPI/Network)"

        self.lbl_dim.SetLabel(f"Dimensions: {width} × {height} px")
        self.lbl_order.SetLabel(f"Byte-Order: {bo_str}")
        self.lbl_size.SetLabel(f"Framebuffer Size: {data_bytes:,} bytes")

        # Color swatch
        r = (trans_col >> 16) & 0xFF
        g = (trans_col >> 8) & 0xFF
        b = trans_col & 0xFF
        self.lbl_trans_col.SetLabel(f"trans_col: {trans_col} (0x{trans_col:06X})")
        self.swatch_trans.SetBackgroundColour(wx.Colour(r, g, b))
        self.swatch_trans.Refresh()

        self.lbl_cb_count.SetLabel(f"collis_box points: {len(self.model.points)}")

        meta_dict = {
            "width": width,
            "height": height,
            "trans_col": trans_col,
            "collis_box": [[p[0], p[1]] for p in self.model.points]
        }
        self.txt_json.SetValue(json.dumps(meta_dict, indent=2))

    def _on_model_updated(self) -> None:
        """Populate table when model updates."""
        if self._updating_table:
            return
        self._updating_table = True
        try:
            sel = self.list_points.GetFirstSelected()
            self.list_points.DeleteAllItems()

            points = self.model.points
            for i, (x, y) in enumerate(points):
                idx = self.list_points.InsertItem(i, str(i + 1))
                self.list_points.SetItem(idx, 1, str(x))
                self.list_points.SetItem(idx, 2, str(y))

            if 0 <= sel < len(points):
                self.list_points.Select(sel)

            # Update validation status
            self.lbl_pt_count.SetLabel(f"Points: {len(points)}")

            if len(points) < 3:
                self.lbl_convex.SetLabel("Convex: Need >= 3 points")
                self.lbl_convex.SetForegroundColour(wx.Colour(200, 200, 200))
                self.lbl_winding.SetLabel("Order: -")
                self.lbl_winding.SetForegroundColour(wx.Colour(200, 200, 200))
            else:
                is_cvx = self.model.is_convex()
                is_ccw = self.model.is_counter_clockwise(screen_coords=True)

                if is_cvx:
                    self.lbl_convex.SetLabel("Convex: ✓ Yes")
                    self.lbl_convex.SetForegroundColour(wx.Colour(40, 220, 100))
                else:
                    self.lbl_convex.SetLabel("Convex: ⚠ No (Concave!)")
                    self.lbl_convex.SetForegroundColour(wx.Colour(255, 120, 80))

                if is_ccw:
                    self.lbl_winding.SetLabel("Order: ✓ Counter-Clockwise")
                    self.lbl_winding.SetForegroundColour(wx.Colour(40, 220, 100))
                else:
                    self.lbl_winding.SetLabel("Order: ⚠ Clockwise (Reverse needed)")
                    self.lbl_winding.SetForegroundColour(wx.Colour(255, 120, 80))

            if self._current_width > 0 and self._current_height > 0:
                self.update_metadata(
                    self._current_width,
                    self._current_height,
                    self._current_byteorder,
                    self._current_trans_col
                )

        finally:
            self._updating_table = False

    def select_row(self, index: int) -> None:
        """Select row in table corresponding to canvas selection."""
        if 0 <= index < self.list_points.GetItemCount():
            self._updating_table = True
            try:
                for i in range(self.list_points.GetItemCount()):
                    self.list_points.Select(i, i == index)
                self.list_points.EnsureVisible(index)
            finally:
                self._updating_table = False

    def _on_list_selected(self, event):
        if self._updating_table:
            return
        idx = event.GetIndex()
        if self.on_point_selected_in_table:
            self.on_point_selected_in_table(idx)

    def _on_move_up(self, event):
        sel = self.list_points.GetFirstSelected()
        if sel > 0:
            new_idx = self.model.move_point_up(sel)
            self.select_row(new_idx)
            if self.on_point_selected_in_table:
                self.on_point_selected_in_table(new_idx)

    def _on_move_down(self, event):
        sel = self.list_points.GetFirstSelected()
        if 0 <= sel < self.list_points.GetItemCount() - 1:
            new_idx = self.model.move_point_down(sel)
            self.select_row(new_idx)
            if self.on_point_selected_in_table:
                self.on_point_selected_in_table(new_idx)

    def _on_delete(self, event):
        sel = self.list_points.GetFirstSelected()
        if 0 <= sel < self.list_points.GetItemCount():
            self.model.remove_point(sel)
            if self.on_point_selected_in_table:
                self.on_point_selected_in_table(-1)
