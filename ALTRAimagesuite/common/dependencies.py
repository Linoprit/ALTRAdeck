"""Pre-startup Python package dependency checking."""

import importlib.util
import sys
from typing import Dict, List, Optional, Tuple

# Mapping of import module name to user-friendly package name / pip install target
CORE_DEPENDENCIES: Dict[str, str] = {
    "PIL": "Pillow",
    "numpy": "numpy",
}

GUI_DEPENDENCIES: Dict[str, str] = {
    "wx": "wxPython",
}


def check_missing_packages(check_gui: bool = True) -> List[Tuple[str, str]]:
    """
    Check if required packages are installed in current Python environment.

    Args:
        check_gui: Whether to include GUI packages (wxPython).

    Returns:
        List of tuples (import_name, pip_package_name) for missing packages.
    """
    required = dict(CORE_DEPENDENCIES)
    if check_gui:
        required.update(GUI_DEPENDENCIES)

    missing = []
    for mod_name, pkg_name in required.items():
        spec = importlib.util.find_spec(mod_name)
        if spec is None:
            missing.append((mod_name, pkg_name))

    return missing


def format_missing_report(missing: List[Tuple[str, str]]) -> str:
    """Format missing packages into a clear, user-facing error message."""
    lines = [
        "============================================================",
        " ALTRAimagesuite - Missing Required Dependencies",
        "============================================================",
        "The following required Python package(s) are not installed:\n",
    ]
    pip_pkgs = []
    for mod, pkg in missing:
        lines.append(f"  • {pkg} (module: '{mod}')")
        pip_pkgs.append(pkg)

    install_cmd = f"{sys.executable} -m pip install " + " ".join(pip_pkgs)
    lines.extend([
        "",
        "Please install the missing packages by running:",
        f"  {install_cmd}",
        "============================================================",
    ])
    return "\n".join(lines)


def ensure_dependencies_or_exit(check_gui: bool = True) -> None:
    """
    Check for required dependencies prior to startup.
    If any are missing, report them to stderr and exit gracefully with code 1.
    """
    missing = check_missing_packages(check_gui=check_gui)
    if missing:
        print(format_missing_report(missing), file=sys.stderr)
        sys.exit(1)
