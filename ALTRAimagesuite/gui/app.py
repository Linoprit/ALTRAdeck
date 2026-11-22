"""Application entry point and GUI app wrapper."""

import sys
from typing import Optional
import wx

from gui.main_win import MainWindow


class ImageSuiteApp(wx.App):
    """wx.App subclass for ALTRAimagesuite."""

    def __init__(self, initial_file: Optional[str] = None, byteorder: str = "little"):
        self.initial_file = initial_file
        self.byteorder = byteorder
        super().__init__(clearSigInt=True)

    def OnInit(self) -> bool:
        self.SetAppName("ALTRAimagesuite")
        frame = MainWindow(initial_file=self.initial_file, byteorder=self.byteorder)
        frame.Show()
        self.SetTopWindow(frame)
        return True
