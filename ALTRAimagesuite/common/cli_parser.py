"""Command-line argument parser for ALTRAimagesuite."""

import argparse
from typing import List, Optional


def create_parser() -> argparse.ArgumentParser:
    """Create and return CLI parser."""
    parser = argparse.ArgumentParser(
        prog="ALTRAimagesuite",
        description="ALTRAimagesuite: Image converter and collis-box editor for ALTRAgraphix fxbuf format"
    )

    parser.add_argument(
        "file",
        nargs="?",
        default=None,
        help="Path to an image file or fxbuf (basename without extension, or .data / .json) to open on launch"
    )

    parser.add_argument(
        "-i", "--image",
        dest="image_file",
        default=None,
        help="Specify image file or fxbuf basename without extension to open"
    )

    parser.add_argument(
        "-b", "--byte-order",
        dest="byte_order",
        choices=["little", "big"],
        default="little",
        help="Byte order for RGB565 data (default: little)"
    )

    parser.add_argument(
        "-r", "--resize",
        dest="resize",
        metavar="WIDTHxHEIGHT",
        default=None,
        help="Resize dimensions (e.g. 320x240)"
    )

    parser.add_argument(
        "-t", "--trans-col",
        dest="trans_col",
        default="0",
        help="Transparent RGB888 color value in integer or hex (e.g. 0 or 0xFF00FF)"
    )

    parser.add_argument(
        "-c", "--convert",
        dest="convert_out",
        metavar="OUTPUT_BASE",
        default=None,
        help="Headless conversion mode: convert input file to fxbuf with OUTPUT_BASE.data and OUTPUT_BASE.json"
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug logging output"
    )

    return parser


def parse_args(args: Optional[List[str]] = None) -> argparse.Namespace:
    """Parse CLI arguments."""
    parser = create_parser()
    return parser.parse_args(args)
