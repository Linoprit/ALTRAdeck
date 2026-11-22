"""Main application window for ALTRAimagesuite."""

import os
from pathlib import Path
from typing import Optional, Tuple
from PIL import Image
import wx

from app_parts.collis_box.collis_box_model import CollisBoxModel
from app_parts.collis_box.polygon import compute_convex_hull
from app_parts.img_conversion.fxbuf_io import FxbufImage, load_any_image, load_fxbuf, save_fxbuf
from app_parts.img_conversion.resizer import resize_image
from app_parts.img_conversion.rgb565 import swap_image_endianness
from common.file_io import get_fxbuf_paths
from common.logger import logger
from common.settings import SettingsManager
from gui.center_view import CenterView
from gui.data_bar import DataBar
from gui.dialogs import OpenFxbufDialog, ResizeDialog, SaveFxbufDialog
from gui.toolbar import LeftToolbar


class MainWindow(wx.Frame):
    """Main Application Window."""

    def __init__(self, parent=None, initial_file: Optional[str] = None, byteorder: str = "little"):
        super().__init__(parent, title="ALTRAimagesuite", size=(1100, 750))

        self.settings = SettingsManager()
        self.byteorder = byteorder or self.settings.get("byte_order", "little")
        self.trans_col: int = 0
        self.current_file_path: Optional[str] = None
        self.current_image: Optional[Image.Image] = None
        self._is_dirty: bool = False
        self._loading: bool = False

        self.collis_box_model = CollisBoxModel()

        self._init_menu()
        self._init_status_bar()
        self._init_layout()
        self._bind_events()

        self.Bind(wx.EVT_CLOSE, self.on_close)

        if initial_file:
            self.load_file(initial_file, byteorder=self.byteorder)

        self._update_title()
        self.Center()

    def _init_menu(self) -> None:
        menubar = wx.MenuBar()

        # File Menu
        file_menu = wx.Menu()
        m_open_img = file_menu.Append(wx.ID_OPEN, "&Open Image...\tCtrl+O", "Open an image file (PNG, JPG, BMP, etc.)")
        m_open_fxbuf = file_menu.Append(wx.ID_ANY, "Open &fxbuf...\tCtrl+Shift+O", "Open an fxbuf (.data / .json)")
        file_menu.AppendSeparator()
        m_save_fxbuf = file_menu.Append(wx.ID_SAVE, "&Save fxbuf...\tCtrl+S", "Save image, trans_col, and collis-box to fxbuf")
        m_save_as_img = file_menu.Append(wx.ID_SAVEAS, "Save &As Image...", "Export current image as PNG/BMP/JPG")
        file_menu.AppendSeparator()
        m_exit = file_menu.Append(wx.ID_EXIT, "E&xit\tCtrl+Q", "Exit application")
        menubar.Append(file_menu, "&File")

        # Edit Menu
        edit_menu = wx.Menu()
        m_undo = edit_menu.Append(wx.ID_UNDO, "&Undo Point\tCtrl+Z", "Undo collis-box change")
        m_redo = edit_menu.Append(wx.ID_REDO, "&Redo Point\tCtrl+Y", "Redo collis-box change")
        edit_menu.AppendSeparator()
        m_swap_endian = edit_menu.Append(wx.ID_ANY, "Swap &Endianness (LE/BE)\tCtrl+E", "Toggle RGB565 byte order of image")
        edit_menu.AppendSeparator()
        m_make_ccw = edit_menu.Append(wx.ID_ANY, "Ensure &Counter-Clockwise (CCW)", "Reverse points if polygon is clockwise")
        m_auto_hull = edit_menu.Append(wx.ID_ANY, "Auto &Convex Hull", "Compute convex hull bounding box")
        m_clear_cb = edit_menu.Append(wx.ID_CLEAR, "Clear &Collis-box", "Remove all collis-box points")
        menubar.Append(edit_menu, "&Edit")

        # View Menu
        view_menu = wx.Menu()
        m_zoom_in = view_menu.Append(wx.ID_ZOOM_IN, "Zoom &In\tCtrl++", "Zoom in")
        m_zoom_out = view_menu.Append(wx.ID_ZOOM_OUT, "Zoom &Out\tCtrl+-", "Zoom out")
        m_zoom_reset = view_menu.Append(wx.ID_ZOOM_100, "&Reset Zoom (100%)\tCtrl+0", "Reset zoom level to 100%")
        m_zoom_fit = view_menu.Append(wx.ID_ZOOM_FIT, "&Fit to Window\tCtrl+F", "Fit image within view")
        menubar.Append(view_menu, "&View")

        # Tools Menu
        tools_menu = wx.Menu()
        m_tool_conv = tools_menu.Append(wx.ID_ANY, "Tool: &Image Conversion\tCtrl+1", "Activate Image Conversion tool")
        m_tool_cb = tools_menu.Append(wx.ID_ANY, "Tool: &Draw Collis-box\tCtrl+2", "Activate Draw Collis-box tool")
        menubar.Append(tools_menu, "&Tools")

        # Help Menu
        help_menu = wx.Menu()
        m_guide = help_menu.Append(wx.ID_HELP, "&Usage Guide", "View instructions and shortcuts")
        m_about = help_menu.Append(wx.ID_ABOUT, "&About", "About ALTRAimagesuite")
        menubar.Append(help_menu, "&Help")

        self.SetMenuBar(menubar)

        # Bind menu actions
        self.Bind(wx.EVT_MENU, lambda e: self.on_import_image(), m_open_img)
        self.Bind(wx.EVT_MENU, lambda e: self.on_open_fxbuf(), m_open_fxbuf)
        self.Bind(wx.EVT_MENU, lambda e: self.on_save_fxbuf(), m_save_fxbuf)
        self.Bind(wx.EVT_MENU, lambda e: self.on_save_as_image(), m_save_as_img)
        self.Bind(wx.EVT_MENU, lambda e: self.Close(), m_exit)

        self.Bind(wx.EVT_MENU, lambda e: self.collis_box_model.undo(), m_undo)
        self.Bind(wx.EVT_MENU, lambda e: self.collis_box_model.redo(), m_redo)
        self.Bind(wx.EVT_MENU, lambda e: self.on_swap_endianness(), m_swap_endian)
        self.Bind(wx.EVT_MENU, lambda e: self.collis_box_model.enforce_counter_clockwise(screen_coords=True), m_make_ccw)
        self.Bind(wx.EVT_MENU, lambda e: self.on_auto_convex_hull(), m_auto_hull)
        self.Bind(wx.EVT_MENU, lambda e: self.collis_box_model.clear(), m_clear_cb)

        self.Bind(wx.EVT_MENU, lambda e: self.center_view.set_zoom(self.center_view.zoom * 1.25), m_zoom_in)
        self.Bind(wx.EVT_MENU, lambda e: self.center_view.set_zoom(self.center_view.zoom / 1.25), m_zoom_out)
        self.Bind(wx.EVT_MENU, lambda e: self.center_view.set_zoom(1.0), m_zoom_reset)
        self.Bind(wx.EVT_MENU, lambda e: self.center_view.fit_to_window(), m_zoom_fit)

        self.Bind(wx.EVT_MENU, lambda e: self.toolbar.set_active_tool(LeftToolbar.TOOL_CONVERT), m_tool_conv)
        self.Bind(wx.EVT_MENU, lambda e: self.toolbar.set_active_tool(LeftToolbar.TOOL_COLLISBOX), m_tool_cb)

        self.Bind(wx.EVT_MENU, self._on_show_guide, m_guide)
        self.Bind(wx.EVT_MENU, self._on_show_about, m_about)

    def _init_status_bar(self) -> None:
        self.status_bar = self.CreateStatusBar(4)
        self.status_bar.SetStatusWidths([-3, -2, -2, -2])
        self.status_bar.SetStatusText("Ready", 0)
        self.status_bar.SetStatusText("Tool: Image Conversion", 1)
        self.status_bar.SetStatusText("X: -  Y: -", 2)
        self.status_bar.SetStatusText("No image", 3)

    def _init_layout(self) -> None:
        main_sizer = wx.BoxSizer(wx.HORIZONTAL)

        # Left Toolbar
        self.toolbar = LeftToolbar(self)
        self.toolbar.SetMinSize((180, -1))
        main_sizer.Add(self.toolbar, 0, wx.EXPAND)

        # Center Viewport
        self.center_view = CenterView(self, self.collis_box_model)
        main_sizer.Add(self.center_view, 1, wx.EXPAND)

        # Right Databar
        self.data_bar = DataBar(self, self.collis_box_model)
        self.data_bar.SetMinSize((260, -1))
        main_sizer.Add(self.data_bar, 0, wx.EXPAND)

        self.SetSizer(main_sizer)

    def _bind_events(self) -> None:
        # CollisBoxModel change listener
        self.collis_box_model.add_listener(self._on_collis_box_changed)

        # Toolbar callbacks
        self.toolbar.on_tool_changed = self._on_tool_changed
        self.toolbar.on_import_clicked = self.on_import_image
        self.toolbar.on_resize_clicked = self.on_resize_image
        self.toolbar.on_swap_endianness_clicked = self.on_swap_endianness
        self.toolbar.on_pick_trans_col_toggled = self._on_pick_trans_col_toggled
        self.toolbar.on_save_fxbuf_clicked = self.on_save_fxbuf

        self.toolbar.on_collisbox_mode_changed = self._on_collisbox_mode_changed
        self.toolbar.on_make_ccw_clicked = lambda: self.collis_box_model.enforce_counter_clockwise(screen_coords=True)
        self.toolbar.on_auto_hull_clicked = self.on_auto_convex_hull
        self.toolbar.on_clear_collisbox_clicked = lambda: self.collis_box_model.clear()

        # CenterView callbacks
        self.center_view.on_mouse_pos_change = self._on_mouse_move_in_image
        self.center_view.on_point_selected = self._on_point_selected_in_canvas
        self.center_view.on_trans_col_picked = self._on_trans_col_picked

        # DataBar callbacks
        self.data_bar.on_point_selected_in_table = self._on_point_selected_in_table

    def _update_title(self) -> None:
        """Update window title with filename and dirty indicator."""
        filename = Path(self.current_file_path).name if self.current_file_path else "Untitled"
        dirty_indicator = " *" if self._is_dirty else ""
        self.SetTitle(f"ALTRAimagesuite - {filename}{dirty_indicator}")

    def mark_dirty(self) -> None:
        """Mark document as having unsaved modifications."""
        if not self._is_dirty:
            self._is_dirty = True
            self._update_title()

    def _on_collis_box_changed(self) -> None:
        """Handle collis-box model notifications."""
        if not self._loading and self.current_image is not None:
            self.mark_dirty()

    def prompt_save_if_dirty(self) -> bool:
        """
        Prompt user to save changes if unsaved data exists.
        Returns True if safe to proceed (saved or discarded), False if cancelled.
        """
        if not self._is_dirty or self.current_image is None:
            return True

        msg = "You have unsaved changes.\n\nDo you want to save your changes before proceeding?"
        with wx.MessageDialog(self, msg, "Unsaved Changes", wx.YES_NO | wx.CANCEL | wx.ICON_QUESTION) as dlg:
            res = dlg.ShowModal()
            if res == wx.ID_YES:
                return self.on_save_fxbuf()
            elif res == wx.ID_NO:
                return True
            else:
                return False

    def on_close(self, event: wx.CloseEvent) -> None:
        """Handle application close event, prompting if unsaved changes exist."""
        if self._is_dirty and self.current_image is not None:
            msg = "You have unsaved changes.\n\nDo you want to save your changes before exiting?"
            with wx.MessageDialog(self, msg, "Unsaved Changes", wx.YES_NO | wx.CANCEL | wx.ICON_QUESTION) as dlg:
                res = dlg.ShowModal()
                if res == wx.ID_YES:
                    if not self.on_save_fxbuf():
                        if event.CanVeto():
                            event.Veto()
                        return
                    event.Skip()
                elif res == wx.ID_NO:
                    event.Skip()
                else:
                    if event.CanVeto():
                        event.Veto()
                    return
        else:
            event.Skip()

    def _on_tool_changed(self, tool: str) -> None:
        """Handle tool selection switch."""
        if tool == LeftToolbar.TOOL_CONVERT:
            self.center_view.set_mode(CenterView.MODE_INSPECT)
            self.toolbar.set_pick_trans_col_active(False)
            self.data_bar.show_tool_view("convert")
            self.status_bar.SetStatusText("Tool: Image Conversion", 1)
        else:
            self.toolbar.set_pick_trans_col_active(False)
            mode = CenterView.MODE_ADD if self.toolbar.current_collisbox_action == "add" else CenterView.MODE_REMOVE
            self.center_view.set_mode(mode)
            self.data_bar.show_tool_view("collisbox")
            mode_lbl = "Add Point" if self.toolbar.current_collisbox_action == "add" else "Remove Point"
            self.status_bar.SetStatusText(f"Tool: Draw Collis-box [{mode_lbl}]", 1)

    def _on_pick_trans_col_toggled(self, is_toggled: bool) -> None:
        """Handle 'Pick Trans Color' toggle button."""
        if not self.current_image:
            self.toolbar.set_pick_trans_col_active(False)
            wx.MessageBox("No image opened. Open an image first to pick a transparent color.", "Pick Color", wx.OK | wx.ICON_INFORMATION, self)
            return

        if is_toggled:
            self.center_view.set_mode(CenterView.MODE_PICK_TRANS_COL)
            self.status_bar.SetStatusText("Click any pixel in the image to set as transparent color", 0)
        else:
            self.center_view.set_mode(CenterView.MODE_INSPECT)
            self.status_bar.SetStatusText("Ready", 0)

    def _on_trans_col_picked(self, ix: int, iy: int, rgb: Tuple[int, int, int], trans_val: int) -> None:
        """Handle pixel click when picking transparent color."""
        self.trans_col = trans_val
        self.mark_dirty()
        if self.current_image:
            w, h = self.current_image.size
            self.data_bar.update_metadata(w, h, byteorder=self.byteorder, trans_col=self.trans_col)

        # Deactivate pick mode
        self.toolbar.set_pick_trans_col_active(False)
        self.center_view.set_mode(CenterView.MODE_INSPECT)

        msg = f"Set trans_col to 0x{trans_val:06X} (RGB: {rgb[0]}, {rgb[1]}, {rgb[2]}) from pixel ({ix}, {iy})"
        self.status_bar.SetStatusText(msg, 0)
        logger.info(msg)

    def _on_collisbox_mode_changed(self, action: str) -> None:
        """Handle Add vs Remove point toggle."""
        mode = CenterView.MODE_ADD if action == "add" else CenterView.MODE_REMOVE
        self.center_view.set_mode(mode)
        mode_lbl = "Add Point" if action == "add" else "Remove Point"
        self.status_bar.SetStatusText(f"Tool: Draw Collis-box [{mode_lbl}]", 1)

    def _on_mouse_move_in_image(self, ix: int, iy: int) -> None:
        self.status_bar.SetStatusText(f"X: {ix} px  Y: {iy} px", 2)

    def _on_point_selected_in_canvas(self, idx: int) -> None:
        self.data_bar.select_row(idx)

    def _on_point_selected_in_table(self, idx: int) -> None:
        self.center_view.selected_point_idx = idx
        self.center_view.Refresh()

    def load_file(self, path: str, byteorder: str = "little") -> bool:
        """Load an image or fxbuf into the application."""
        try:
            self._loading = True
            fx_img = load_any_image(path, byteorder=byteorder)
            self.current_image = fx_img.image
            self.current_file_path = path
            self.byteorder = byteorder
            self.trans_col = fx_img.trans_col

            self.collis_box_model.set_points(fx_img.collis_box, record_history=False)
            self.center_view.set_image(self.current_image, reset_zoom=True)

            w, h = self.current_image.size
            self.data_bar.update_metadata(w, h, byteorder=byteorder, trans_col=self.trans_col)
            self.status_bar.SetStatusText(f"Loaded: {Path(path).name}", 0)
            self.status_bar.SetStatusText(f"{w} × {h} px | {int(self.center_view.zoom * 100)}%", 3)

            self._is_dirty = False
            self._update_title()

            self.settings.add_recent_file(os.path.abspath(path))
            return True
        except Exception as e:
            logger.error("Failed to load file %s: %s", path, e)
            wx.MessageBox(f"Failed to load file:\n{e}", "Open Error", wx.OK | wx.ICON_ERROR, self)
            return False
        finally:
            self._loading = False

    def on_import_image(self) -> None:
        """Open file dialog to import an image."""
        if not self.prompt_save_if_dirty():
            return

        wildcard = (
            "All Supported (*.png;*.jpg;*.jpeg;*.bmp;*.data;*.json)|*.png;*.jpg;*.jpeg;*.bmp;*.data;*.json|"
            "PNG images (*.png)|*.png|"
            "JPEG images (*.jpg;*.jpeg)|*.jpg;*.jpeg|"
            "BMP images (*.bmp)|*.bmp|"
            "fxbuf files (*.data;*.json)|*.data;*.json|"
            "All files (*.*)|*.*"
        )
        with wx.FileDialog(
            self,
            message="Import Image",
            wildcard=wildcard,
            style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                path = dlg.GetPath()
                if path.endswith(".data") or path.endswith(".json"):
                    self.on_open_fxbuf(path)
                else:
                    self.load_file(path, byteorder=self.byteorder)

    def on_open_fxbuf(self, prefill_path: Optional[str] = None) -> None:
        """Open fxbuf with byte-order dialog."""
        if not self.prompt_save_if_dirty():
            return

        dlg = OpenFxbufDialog(self, default_byteorder=self.byteorder)
        if prefill_path:
            dlg.txt_file.SetValue(prefill_path)
        if dlg.ShowModal() == wx.ID_OK:
            path, bo = dlg.get_result()
            if path:
                self.load_file(path, byteorder=bo)
        dlg.Destroy()

    def on_swap_endianness(self) -> None:
        """Change/swap endianness (Little-Endian <-> Big-Endian) of the currently opened image."""
        if not self.current_image:
            wx.MessageBox("No image opened to change endianness.", "Swap Endianness", wx.OK | wx.ICON_INFORMATION, self)
            return

        swapped_img, new_bo = swap_image_endianness(self.current_image, from_byteorder=self.byteorder)
        self.current_image = swapped_img
        self.byteorder = new_bo
        self.center_view.set_image(self.current_image, reset_zoom=False)

        w, h = self.current_image.size
        self.data_bar.update_metadata(w, h, byteorder=self.byteorder, trans_col=self.trans_col)
        self.mark_dirty()

        bo_name = "Little-Endian (ARM native)" if new_bo == "little" else "Big-Endian (SPI/Network)"
        self.status_bar.SetStatusText(f"Swapped RGB565 byte order to {bo_name}", 0)
        logger.info("Swapped image endianness to %s", new_bo)

    def on_resize_image(self) -> None:
        """Open resize dialog and resize current image."""
        if not self.current_image:
            wx.MessageBox("No image opened to resize.", "Resize", wx.OK | wx.ICON_INFORMATION, self)
            return

        orig_w, orig_h = self.current_image.size
        dlg = ResizeDialog(self, orig_w, orig_h)
        if dlg.ShowModal() == wx.ID_OK:
            new_w, new_h, filter_mode = dlg.get_result()
            if new_w != orig_w or new_h != orig_h:
                resized = resize_image(self.current_image, new_w, new_h, resample=filter_mode)
                self.current_image = resized
                self.mark_dirty()
                self.center_view.set_image(self.current_image, reset_zoom=False)
                self.data_bar.update_metadata(new_w, new_h, byteorder=self.byteorder, trans_col=self.trans_col)
                self.status_bar.SetStatusText(f"Resized to {new_w} × {new_h} px", 0)
                self.status_bar.SetStatusText(f"{new_w} × {new_h} px | {int(self.center_view.zoom * 100)}%", 3)
        dlg.Destroy()

    def on_save_fxbuf(self) -> bool:
        """Open save fxbuf dialog and save .data and .json files. Returns True if saved."""
        if not self.current_image:
            wx.MessageBox("No image available to save.", "Save fxbuf", wx.OK | wx.ICON_INFORMATION, self)
            return False

        default_dir = ""
        default_stem = "image"
        if self.current_file_path:
            p = Path(self.current_file_path)
            default_dir = str(p.parent)
            default_stem = p.stem

        dlg = SaveFxbufDialog(self, default_dir=default_dir, default_name=default_stem, default_byteorder=self.byteorder)
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return False

        base_path, bo = dlg.get_result()
        dlg.Destroy()

        if not base_path:
            return False

        # Ask before overwriting existing files
        data_p, json_p = get_fxbuf_paths(base_path)
        existing = []
        if os.path.exists(data_p):
            existing.append(Path(data_p).name)
        if os.path.exists(json_p):
            existing.append(Path(json_p).name)

        if existing:
            file_list = "\n  • ".join(existing)
            msg = f"The following file(s) already exist:\n  • {file_list}\n\nDo you want to overwrite them?"
            with wx.MessageDialog(self, msg, "Confirm Overwrite", wx.YES_NO | wx.NO_DEFAULT | wx.ICON_WARNING) as dlg_ovr:
                if dlg_ovr.ShowModal() != wx.ID_YES:
                    return False

        try:
            saved_data, saved_json = save_fxbuf(
                base_path,
                self.current_image,
                collis_box=self.collis_box_model.points,
                trans_col=self.trans_col,
                byteorder=bo
            )
            self.byteorder = bo
            self.current_file_path = base_path
            self._is_dirty = False
            self._update_title()
            self.status_bar.SetStatusText(f"Saved: {Path(saved_data).name} & {Path(saved_json).name}", 0)
            wx.MessageBox(f"Successfully saved fxbuf files:\n{saved_data}\n{saved_json}", "Save fxbuf", wx.OK | wx.ICON_INFORMATION, self)
            return True
        except Exception as e:
            logger.error("Failed to save fxbuf %s: %s", base_path, e)
            wx.MessageBox(f"Failed to save fxbuf:\n{e}", "Save Error", wx.OK | wx.ICON_ERROR, self)
            return False

    def on_save_as_image(self) -> None:
        """Export current image to standard formats (PNG, BMP, JPG)."""
        if not self.current_image:
            wx.MessageBox("No image available to export.", "Export Image", wx.OK | wx.ICON_INFORMATION, self)
            return

        wildcard = "PNG image (*.png)|*.png|Bitmap (*.bmp)|*.bmp|JPEG image (*.jpg)|*.jpg"
        with wx.FileDialog(
            self,
            message="Export Image As",
            wildcard=wildcard,
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                out_path = dlg.GetPath()
                try:
                    self.current_image.save(out_path)
                    self.status_bar.SetStatusText(f"Exported image: {Path(out_path).name}", 0)
                except Exception as e:
                    wx.MessageBox(f"Failed to export image:\n{e}", "Export Error", wx.OK | wx.ICON_ERROR, self)

    def on_auto_convex_hull(self) -> None:
        """Compute convex hull from current points or image boundaries."""
        if len(self.collis_box_model.points) >= 3:
            hull = compute_convex_hull(self.collis_box_model.points, screen_coords=True)
            self.collis_box_model.set_points(hull)
            self.status_bar.SetStatusText("Computed convex hull from points", 0)
        elif self.current_image:
            w, h = self.current_image.size
            box = [(0, 0), (0, h - 1), (w - 1, h - 1), (w - 1, 0)]
            self.collis_box_model.set_points(box)
            self.status_bar.SetStatusText("Created default convex collis-box", 0)
        else:
            wx.MessageBox("Open an image or add at least 3 points to compute convex hull.", "Auto Hull", wx.OK | wx.ICON_INFORMATION, self)

    def _on_show_guide(self, event) -> None:
        msg = (
            "ALTRAimagesuite Usage Guide:\n\n"
            "Navigation:\n"
            "• Zoom: Ctrl + Mouse Wheel (or Ctrl + / Ctrl -)\n"
            "• Pan: Ctrl + Right-Click Drag (or Right-Click Drag)\n\n"
            "Tools:\n"
            "• Image Conversion: Import images, resize with aspect-ratio locking, pick transparent color, and export to fxbuf format.\n"
            "• Draw Collis-box: Click to add vertices, click near a vertex to remove it, or click-and-drag to move existing vertices.\n"
            "• Collis-box points form a convex polygon ordered counter-clockwise (CCW) for ALTRAgraphix collision detection.\n"
            "• Reorder points using the Up and Down buttons on the right databar table.\n\n"
            "Shortcuts:\n"
            "• Ctrl+O: Open Image\n"
            "• Ctrl+Shift+O: Open fxbuf\n"
            "• Ctrl+S: Save fxbuf\n"
            "• Ctrl+Z / Ctrl+Y: Undo / Redo point edit\n"
            "• Ctrl+1 / Ctrl+2: Switch tools\n"
        )
        wx.MessageBox(msg, "ALTRAimagesuite Guide", wx.OK | wx.ICON_INFORMATION, self)

    def _on_show_about(self, event) -> None:
        msg = (
            "ALTRAimagesuite\n\n"
            "Image manipulation and collis-box editor for ALTRAgraphix fxbuf format.\n"
            "Supports RGB565 conversion (both Little and Big Endian),\n"
            "transparent color (trans_col) selection, and collision detection convex polygon collis-boxes."
        )
        wx.MessageBox(msg, "About ALTRAimagesuite", wx.OK | wx.ICON_INFORMATION, self)
