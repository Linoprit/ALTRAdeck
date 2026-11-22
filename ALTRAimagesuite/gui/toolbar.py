"""Left toolbar with tool selection and dynamic tool-specific panels."""

from typing import Callable, Optional
import wx


class LeftToolbar(wx.Panel):
    """
    Left toolbar component.
    Upper area: Tool select buttons with radio-button behavior.
    Lower area: Buttons available within the active tool.
    """

    TOOL_CONVERT = "convert"
    TOOL_COLLISBOX = "collisbox"

    def __init__(self, parent: wx.Window):
        super().__init__(parent, style=wx.BORDER_THEME)
        self.SetBackgroundColour(wx.Colour(48, 50, 56))

        self.current_tool = self.TOOL_CONVERT
        self.current_collisbox_action = "add"  # 'add' or 'remove'

        # Callbacks
        self.on_tool_changed: Optional[Callable[[str], None]] = None
        self.on_import_clicked: Optional[Callable[[], None]] = None
        self.on_resize_clicked: Optional[Callable[[], None]] = None
        self.on_pick_trans_col_toggled: Optional[Callable[[bool], None]] = None
        self.on_swap_endianness_clicked: Optional[Callable[[], None]] = None
        self.on_save_fxbuf_clicked: Optional[Callable[[], None]] = None

        self.on_collisbox_mode_changed: Optional[Callable[[str], None]] = None
        self.on_make_ccw_clicked: Optional[Callable[[], None]] = None
        self.on_auto_hull_clicked: Optional[Callable[[], None]] = None
        self.on_clear_collisbox_clicked: Optional[Callable[[], None]] = None

        self._init_ui()

    def _init_ui(self) -> None:
        main_sizer = wx.BoxSizer(wx.VERTICAL)

        # Upper section: Tool selector title
        lbl_tools = wx.StaticText(self, label="TOOLS")
        lbl_tools.SetForegroundColour(wx.Colour(180, 185, 195))
        lbl_tools.SetFont(lbl_tools.GetFont().Bold())
        main_sizer.Add(lbl_tools, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 8)

        # Tool select buttons (radio-button behavior)
        self.btn_tool_convert = wx.ToggleButton(self, label="Image Conversion")
        self.btn_tool_collisbox = wx.ToggleButton(self, label="Draw Collis-box")

        btn_font = self.btn_tool_convert.GetFont()
        btn_font.SetWeight(wx.FONTWEIGHT_BOLD)
        self.btn_tool_convert.SetFont(btn_font)
        self.btn_tool_collisbox.SetFont(btn_font)

        self.btn_tool_convert.SetValue(True)
        self.btn_tool_collisbox.SetValue(False)

        main_sizer.Add(self.btn_tool_convert, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 6)
        main_sizer.Add(self.btn_tool_collisbox, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 6)

        # Separator line
        sep = wx.StaticLine(self, style=wx.LI_HORIZONTAL)
        main_sizer.Add(sep, 0, wx.EXPAND | wx.TOP | wx.BOTTOM, 8)

        # Lower section: Dynamic panels container
        self.panel_container = wx.Panel(self)
        self.container_sizer = wx.BoxSizer(wx.VERTICAL)
        self.panel_container.SetSizer(self.container_sizer)

        # Sub-panel 1: Image Conversion Actions
        self.panel_convert = wx.Panel(self.panel_container)
        conv_sizer = wx.BoxSizer(wx.VERTICAL)

        lbl_conv = wx.StaticText(self.panel_convert, label="CONVERSION ACTIONS")
        lbl_conv.SetForegroundColour(wx.Colour(180, 185, 195))
        conv_sizer.Add(lbl_conv, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 6)

        self.btn_import = wx.Button(self.panel_convert, label="Import Image...")
        self.btn_resize = wx.Button(self.panel_convert, label="Resize...")
        self.btn_swap_endian = wx.Button(self.panel_convert, label="Swap Endianness (LE/BE)")
        self.btn_pick_trans_col = wx.ToggleButton(self.panel_convert, label="🎨 Pick Trans Color")
        self.btn_save_fxbuf = wx.Button(self.panel_convert, label="Save fxbuf...")

        conv_sizer.Add(self.btn_import, 0, wx.EXPAND | wx.ALL, 4)
        conv_sizer.Add(self.btn_resize, 0, wx.EXPAND | wx.ALL, 4)
        conv_sizer.Add(self.btn_swap_endian, 0, wx.EXPAND | wx.ALL, 4)
        conv_sizer.Add(self.btn_pick_trans_col, 0, wx.EXPAND | wx.ALL, 4)
        conv_sizer.Add(self.btn_save_fxbuf, 0, wx.EXPAND | wx.ALL, 4)
        self.panel_convert.SetSizer(conv_sizer)

        # Sub-panel 2: Collis-box Actions
        self.panel_collisbox = wx.Panel(self.panel_container)
        cb_sizer = wx.BoxSizer(wx.VERTICAL)

        lbl_cb = wx.StaticText(self.panel_collisbox, label="COLLIS-BOX ACTIONS")
        lbl_cb.SetForegroundColour(wx.Colour(180, 185, 195))
        cb_sizer.Add(lbl_cb, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 6)

        # Add point & Remove point with radio-button behavior
        self.btn_add_point = wx.ToggleButton(self.panel_collisbox, label="✚ Add Point")
        self.btn_remove_point = wx.ToggleButton(self.panel_collisbox, label="✖ Remove Point")

        self.btn_add_point.SetValue(True)
        self.btn_remove_point.SetValue(False)

        cb_sizer.Add(self.btn_add_point, 0, wx.EXPAND | wx.ALL, 4)
        cb_sizer.Add(self.btn_remove_point, 0, wx.EXPAND | wx.ALL, 4)

        sep_cb = wx.StaticLine(self.panel_collisbox, style=wx.LI_HORIZONTAL)
        cb_sizer.Add(sep_cb, 0, wx.EXPAND | wx.TOP | wx.BOTTOM, 8)

        self.btn_make_ccw = wx.Button(self.panel_collisbox, label="Ensure CCW")
        self.btn_auto_hull = wx.Button(self.panel_collisbox, label="Auto Convex Hull")
        self.btn_clear_points = wx.Button(self.panel_collisbox, label="Clear Points")

        cb_sizer.Add(self.btn_make_ccw, 0, wx.EXPAND | wx.ALL, 4)
        cb_sizer.Add(self.btn_auto_hull, 0, wx.EXPAND | wx.ALL, 4)
        cb_sizer.Add(self.btn_clear_points, 0, wx.EXPAND | wx.ALL, 4)
        self.panel_collisbox.SetSizer(cb_sizer)

        # Add subpanels to container
        self.container_sizer.Add(self.panel_convert, 1, wx.EXPAND)
        self.container_sizer.Add(self.panel_collisbox, 1, wx.EXPAND)
        self.panel_collisbox.Hide()

        main_sizer.Add(self.panel_container, 1, wx.EXPAND)
        self.SetSizer(main_sizer)

        # Bind events
        self.btn_tool_convert.Bind(wx.EVT_TOGGLEBUTTON, self._on_tool_toggle_convert)
        self.btn_tool_collisbox.Bind(wx.EVT_TOGGLEBUTTON, self._on_tool_toggle_collisbox)

        self.btn_import.Bind(wx.EVT_BUTTON, lambda e: self.on_import_clicked and self.on_import_clicked())
        self.btn_resize.Bind(wx.EVT_BUTTON, lambda e: self.on_resize_clicked and self.on_resize_clicked())
        self.btn_swap_endian.Bind(wx.EVT_BUTTON, lambda e: self.on_swap_endianness_clicked and self.on_swap_endianness_clicked())
        self.btn_pick_trans_col.Bind(wx.EVT_TOGGLEBUTTON, self._on_pick_trans_col_toggle)
        self.btn_save_fxbuf.Bind(wx.EVT_BUTTON, lambda e: self.on_save_fxbuf_clicked and self.on_save_fxbuf_clicked())

        self.btn_add_point.Bind(wx.EVT_TOGGLEBUTTON, self._on_add_point_toggled)
        self.btn_remove_point.Bind(wx.EVT_TOGGLEBUTTON, self._on_remove_point_toggled)

        self.btn_make_ccw.Bind(wx.EVT_BUTTON, lambda e: self.on_make_ccw_clicked and self.on_make_ccw_clicked())
        self.btn_auto_hull.Bind(wx.EVT_BUTTON, lambda e: self.on_auto_hull_clicked and self.on_auto_hull_clicked())
        self.btn_clear_points.Bind(wx.EVT_BUTTON, lambda e: self.on_clear_collisbox_clicked and self.on_clear_collisbox_clicked())

    def _on_tool_toggle_convert(self, event):
        self.btn_tool_convert.SetValue(True)
        self.btn_tool_collisbox.SetValue(False)
        self.set_active_tool(self.TOOL_CONVERT)

    def _on_tool_toggle_collisbox(self, event):
        self.btn_tool_convert.SetValue(False)
        self.btn_tool_collisbox.SetValue(True)
        self.set_active_tool(self.TOOL_COLLISBOX)

    def set_active_tool(self, tool: str) -> None:
        """Switch active tool panel."""
        self.current_tool = tool
        if tool == self.TOOL_CONVERT:
            self.btn_tool_convert.SetValue(True)
            self.btn_tool_collisbox.SetValue(False)
            self.panel_convert.Show()
            self.panel_collisbox.Hide()
        else:
            self.btn_tool_convert.SetValue(False)
            self.btn_tool_collisbox.SetValue(True)
            self.panel_convert.Hide()
            self.panel_collisbox.Show()

        self.panel_container.Layout()
        self.Layout()

        if self.on_tool_changed:
            self.on_tool_changed(tool)

    def _on_pick_trans_col_toggle(self, event):
        is_toggled = self.btn_pick_trans_col.GetValue()
        if self.on_pick_trans_col_toggled:
            self.on_pick_trans_col_toggled(is_toggled)

    def set_pick_trans_col_active(self, active: bool) -> None:
        """Update toggle state of pick trans color button."""
        self.btn_pick_trans_col.SetValue(active)

    def _on_add_point_toggled(self, event):
        self.btn_add_point.SetValue(True)
        self.btn_remove_point.SetValue(False)
        self.current_collisbox_action = "add"
        if self.on_collisbox_mode_changed:
            self.on_collisbox_mode_changed("add")

    def _on_remove_point_toggled(self, event):
        self.btn_add_point.SetValue(False)
        self.btn_remove_point.SetValue(True)
        self.current_collisbox_action = "remove"
        if self.on_collisbox_mode_changed:
            self.on_collisbox_mode_changed("remove")
