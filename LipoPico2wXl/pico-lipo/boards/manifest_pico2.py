# micropython/ports/rp2/../../../
MODULES_PY = "$(BOARD_DIR)/../../modules_py"
MODULES_LOCAL = "$(BOARD_DIR)/../../modules/python"

# SD Card
require("sdcard")

freeze(MODULES_PY, "gfx_pack.py")

freeze(MODULES_PY, "pimoroni.py")
freeze(MODULES_PY, "boot.py")

freeze(MODULES_PY, "lte.py")

freeze(MODULES_LOCAL, "ezwifi.py")
