#!/usr/bin/env python3
"""
ALTRAimagesuite - Image converter and collis-box editor for ALTRAgraphix fxbuf format.
"""

import os
import sys

# Ensure package directory is in sys.path
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

# Check required dependencies prior to startup
from common.dependencies import ensure_dependencies_or_exit
from common.cli_parser import parse_args


def run_headless_conversion(args) -> int:
    """Perform headless image conversion to fxbuf without launching GUI."""
    # Ensure core packages (Pillow, numpy) are available
    ensure_dependencies_or_exit(check_gui=False)

    from common.logger import logger
    from app_parts.img_conversion.fxbuf_io import load_any_image, save_fxbuf, parse_trans_col
    from app_parts.img_conversion.resizer import resize_image

    input_file = args.image_file or args.file
    if not input_file:
        logger.error("Headless conversion requires an input file (-i or positional argument)")
        return 1

    output_base = args.convert_out
    byteorder = args.byte_order or "little"
    trans_col = parse_trans_col(args.trans_col)

    try:
        logger.info("Loading %s...", input_file)
        fx_img = load_any_image(input_file, byteorder=byteorder)

        img = fx_img.image
        if args.resize:
            try:
                parts = args.resize.lower().split("x")
                w, h = int(parts[0]), int(parts[1])
                img = resize_image(img, w, h)
                logger.info("Resized image to %dx%d", w, h)
            except Exception as e:
                logger.error("Invalid resize argument '%s': %s", args.resize, e)
                return 1

        chosen_trans_col = trans_col if args.trans_col != "0" or fx_img.trans_col == 0 else fx_img.trans_col
        data_p, json_p = save_fxbuf(
            output_base,
            img,
            collis_box=fx_img.collis_box,
            trans_col=chosen_trans_col,
            byteorder=byteorder
        )
        logger.info("Headless conversion complete: %s, %s", data_p, json_p)
        return 0
    except Exception as e:
        logger.error("Conversion failed: %s", e)
        return 1


def main() -> int:
    args = parse_args()

    if args.debug:
        from common.logger import setup_logger
        setup_logger(level=10)  # DEBUG

    # Headless conversion mode
    if args.convert_out:
        return run_headless_conversion(args)

    # For GUI mode, ensure all dependencies including wxPython are available
    ensure_dependencies_or_exit(check_gui=True)

    # Launch GUI
    from gui.app import ImageSuiteApp

    initial_file = args.image_file or args.file
    app = ImageSuiteApp(initial_file=initial_file, byteorder=args.byte_order)
    app.MainLoop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
