"""Dialogs for ALTRAimagesuite."""

from pathlib import Path
from typing import Optional, Tuple
from PIL import Image
import wx

from app_parts.img_conversion.resizer import calculate_aspect_ratio_size


class ResizeDialog(wx.Dialog):
    """Dialog for resizing the current image with aspect ratio locking."""

    def __init__(self, parent: wx.Window, orig_width: int, orig_height: int):
        super().__init__(parent, title="Resize Image", style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)

        self.orig_width = orig_width
        self.orig_height = orig_height
        self._updating = False

        vbox = wx.BoxSizer(wx.VERTICAL)

        info_lbl = wx.StaticText(self, label=f"Original Size: {orig_width} × {orig_height} pixels")
        info_lbl.SetFont(info_lbl.GetFont().Bold())
        vbox.Add(info_lbl, 0, wx.ALL, 10)

        # Dimension controls
        grid = wx.FlexGridSizer(rows=2, cols=2, vgap=10, hgap=10)

        grid.Add(wx.StaticText(self, label="New Width (px):"), 0, wx.ALIGN_CENTER_VERTICAL)
        self.spin_w = wx.SpinCtrl(self, value=str(orig_width), min=1, max=16384)
        grid.Add(self.spin_w, 1, wx.EXPAND)

        grid.Add(wx.StaticText(self, label="New Height (px):"), 0, wx.ALIGN_CENTER_VERTICAL)
        self.spin_h = wx.SpinCtrl(self, value=str(orig_height), min=1, max=16384)
        grid.Add(self.spin_h, 1, wx.EXPAND)

        vbox.Add(grid, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 15)

        # Aspect ratio checkbox
        self.chk_aspect = wx.CheckBox(self, label="Preserve aspect ratio")
        self.chk_aspect.SetValue(True)
        vbox.Add(self.chk_aspect, 0, wx.ALL, 15)

        # Resampling filter
        filter_box = wx.BoxSizer(wx.HORIZONTAL)
        filter_box.Add(wx.StaticText(self, label="Interpolation:"), 0, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 10)
        self.choice_filter = wx.Choice(self, choices=["Lanczos (High Quality)", "Bilinear", "Bicubic", "Nearest Neighbor"])
        self.choice_filter.SetSelection(0)
        filter_box.Add(self.choice_filter, 1, wx.EXPAND)
        vbox.Add(filter_box, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 15)

        # Standard dialog buttons
        btn_sizer = self.CreateButtonSizer(wx.OK | wx.CANCEL)
        vbox.Add(btn_sizer, 0, wx.ALIGN_RIGHT | wx.ALL, 10)

        self.SetSizer(vbox)
        vbox.Fit(self)
        self.CenterOnParent()

        # Bind events
        self.spin_w.Bind(wx.EVT_SPINCTRL, self._on_width_change)
        self.spin_h.Bind(wx.EVT_SPINCTRL, self._on_height_change)

    def _on_width_change(self, event):
        if self._updating or not self.chk_aspect.GetValue():
            return
        self._updating = True
        try:
            new_w = self.spin_w.GetValue()
            _, new_h = calculate_aspect_ratio_size(self.orig_width, self.orig_height, target_w=new_w)
            self.spin_h.SetValue(new_h)
        finally:
            self._updating = False

    def _on_height_change(self, event):
        if self._updating or not self.chk_aspect.GetValue():
            return
        self._updating = True
        try:
            new_h = self.spin_h.GetValue()
            new_w, _ = calculate_aspect_ratio_size(self.orig_width, self.orig_height, target_h=new_h)
            self.spin_w.SetValue(new_w)
        finally:
            self._updating = False

    def get_result(self) -> Tuple[int, int, int]:
        """Return (width, height, pil_resample_filter)."""
        w = self.spin_w.GetValue()
        h = self.spin_h.GetValue()
        filter_idx = self.choice_filter.GetSelection()
        filters = [
            Image.Resampling.LANCZOS,
            Image.Resampling.BILINEAR,
            Image.Resampling.BICUBIC,
            Image.Resampling.NEAREST,
        ]
        resample = filters[filter_idx] if filter_idx < len(filters) else Image.Resampling.LANCZOS
        return w, h, resample


class SaveFxbufDialog(wx.Dialog):
    """Dialog for saving fxbuf (.data + .json) with byte-order selection."""

    def __init__(self, parent: wx.Window, default_dir: str = "", default_name: str = "image", default_byteorder: str = "little"):
        super().__init__(parent, title="Save fxbuf (ALTRAgraphix)", style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)

        vbox = wx.BoxSizer(wx.VERTICAL)

        info = wx.StaticText(
            self,
            label="This will save two paired files: <filename>.data and <filename>.json\nfor use with ALTRAgraphix fxbuf."
        )
        vbox.Add(info, 0, wx.ALL, 12)

        # File path picker
        path_box = wx.StaticBox(self, label="Output Location")
        path_sizer = wx.StaticBoxSizer(path_box, wx.VERTICAL)

        file_sizer = wx.BoxSizer(wx.HORIZONTAL)
        initial_path = str(Path(default_dir) / default_name) if default_dir else default_name
        self.txt_path = wx.TextCtrl(self, value=initial_path)
        btn_browse = wx.Button(self, label="Browse...")
        btn_browse.Bind(wx.EVT_BUTTON, self._on_browse)

        file_sizer.Add(self.txt_path, 1, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
        file_sizer.Add(btn_browse, 0, wx.ALIGN_CENTER_VERTICAL)
        path_sizer.Add(file_sizer, 0, wx.EXPAND | wx.ALL, 5)

        self.lbl_preview = wx.StaticText(self, label=f"Files: {default_name}.data  and  {default_name}.json")
        path_sizer.Add(self.lbl_preview, 0, wx.LEFT | wx.BOTTOM, 5)

        vbox.Add(path_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 10)

        # Byte order selection
        bo_box = wx.StaticBox(self, label="RGB565 Byte Order")
        bo_sizer = wx.StaticBoxSizer(bo_box, wx.VERTICAL)

        self.rb_little = wx.RadioButton(self, label="Little-Endian (ARM / RP2040 native)", style=wx.RB_GROUP)
        self.rb_big = wx.RadioButton(self, label="Big-Endian (Standard Network / SPI)")

        if default_byteorder == "big":
            self.rb_big.SetValue(True)
        else:
            self.rb_little.SetValue(True)

        bo_sizer.Add(self.rb_little, 0, wx.ALL, 5)
        bo_sizer.Add(self.rb_big, 0, wx.ALL, 5)
        vbox.Add(bo_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 10)

        # Button sizer
        btn_sizer = self.CreateButtonSizer(wx.OK | wx.CANCEL)
        vbox.Add(btn_sizer, 0, wx.ALIGN_RIGHT | wx.ALL, 10)

        self.SetSizer(vbox)
        vbox.Fit(self)
        self.CenterOnParent()

        self.txt_path.Bind(wx.EVT_TEXT, self._on_text_change)

    def _on_browse(self, event):
        curr = self.txt_path.GetValue().strip()
        default_dir = str(Path(curr).parent) if curr else ""
        default_file = Path(curr).stem if curr else "image"

        with wx.FileDialog(
            self,
            message="Choose destination base name for fxbuf",
            defaultDir=default_dir,
            defaultFile=default_file,
            wildcard="fxbuf data (*.data)|*.data|All files (*.*)|*.*",
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                chosen = dlg.GetPath()
                # Strip .data or .json if present
                if chosen.endswith(".data") or chosen.endswith(".json"):
                    chosen = chosen[:-5]
                self.txt_path.SetValue(chosen)

    def _on_text_change(self, event):
        val = self.txt_path.GetValue().strip()
        stem = Path(val).stem if val else "image"
        self.lbl_preview.SetLabel(f"Files: {stem}.data  and  {stem}.json")

    def get_result(self) -> Tuple[str, str]:
        """Return (base_path, byteorder)."""
        base_path = self.txt_path.GetValue().strip()
        byteorder = "big" if self.rb_big.GetValue() else "little"
        return base_path, byteorder


class OpenFxbufDialog(wx.Dialog):
    """Dialog for opening an fxbuf with byte-order specification."""

    def __init__(self, parent: wx.Window, default_byteorder: str = "little"):
        super().__init__(parent, title="Open fxbuf", style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)

        vbox = wx.BoxSizer(wx.VERTICAL)

        info = wx.StaticText(self, label="Select fxbuf .data or .json file and the byte-order:")
        vbox.Add(info, 0, wx.ALL, 10)

        # File picker
        file_sizer = wx.BoxSizer(wx.HORIZONTAL)
        self.txt_file = wx.TextCtrl(self)
        btn_browse = wx.Button(self, label="Browse...")
        btn_browse.Bind(wx.EVT_BUTTON, self._on_browse)

        file_sizer.Add(self.txt_file, 1, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
        file_sizer.Add(btn_browse, 0, wx.ALIGN_CENTER_VERTICAL)
        vbox.Add(file_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 10)

        # Byte order selection
        bo_box = wx.StaticBox(self, label="RGB565 Byte Order")
        bo_sizer = wx.StaticBoxSizer(bo_box, wx.VERTICAL)

        self.rb_little = wx.RadioButton(self, label="Little-Endian (ARM / RP2040 native)", style=wx.RB_GROUP)
        self.rb_big = wx.RadioButton(self, label="Big-Endian (Standard SPI / Network)")

        if default_byteorder == "big":
            self.rb_big.SetValue(True)
        else:
            self.rb_little.SetValue(True)

        bo_sizer.Add(self.rb_little, 0, wx.ALL, 5)
        bo_sizer.Add(self.rb_big, 0, wx.ALL, 5)
        vbox.Add(bo_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 10)

        btn_sizer = self.CreateButtonSizer(wx.OK | wx.CANCEL)
        vbox.Add(btn_sizer, 0, wx.ALIGN_RIGHT | wx.ALL, 10)

        self.SetSizer(vbox)
        vbox.Fit(self)
        self.CenterOnParent()

    def _on_browse(self, event):
        with wx.FileDialog(
            self,
            message="Open fxbuf file",
            wildcard="fxbuf files (*.data;*.json)|*.data;*.json|All files (*.*)|*.*",
            style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                self.txt_file.SetValue(dlg.GetPath())

    def get_result(self) -> Tuple[str, str]:
        """Return (filepath, byteorder)."""
        path = self.txt_file.GetValue().strip()
        byteorder = "big" if self.rb_big.GetValue() else "little"
        return path, byteorder
