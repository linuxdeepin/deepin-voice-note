#!/usr/bin/env python3
"""Open deepin-voice-note's QML title-bar menu, optionally select one item.

This helper is intentionally selector/action based: it locates the application
through AT-SPI, finds the named WebViewTitleBar node, invokes the first visible
child button's AT-SPI Press action, and can invoke a menu item's Press action by
its accessible name. It does not use screen coordinates.
"""
from __future__ import annotations

import argparse
import sys
import time
import warnings
from collections.abc import Iterable

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi  # noqa: E402

warnings.filterwarnings("ignore", category=DeprecationWarning)


def _children(node) -> Iterable:
    try:
        count = node.get_child_count()
    except Exception:
        return []
    result = []
    for i in range(count):
        try:
            child = node.get_child_at_index(i)
        except Exception:
            continue
        if child is not None:
            result.append(child)
    return result


def _walk(node):
    yield node
    for child in _children(node):
        yield from _walk(child)


def _name(node) -> str:
    try:
        return node.get_name() or ""
    except Exception:
        return ""


def _role(node) -> str:
    try:
        return node.get_role_name() or ""
    except Exception:
        return ""


def _visible_extents(node):
    try:
        ext = node.get_extents(Atspi.CoordType.SCREEN)
    except Exception:
        return None
    if ext.width <= 0 or ext.height <= 0:
        return None
    return ext


def _visible(node) -> bool:
    return _visible_extents(node) is not None


def _find_app(app_name: str):
    desktop = Atspi.get_desktop(0)
    for app in _children(desktop):
        if _name(app) == app_name:
            return app
    raise RuntimeError(f"application not found: {app_name}")


def _find_by_name(root, name: str, visible: bool = False):
    for node in _walk(root):
        if _name(node) == name and (not visible or _visible(node)):
            return node
    suffix = " visible" if visible else ""
    raise RuntimeError(f"AT-SPI{suffix} node not found by name: {name}")


def _press(node) -> None:
    try:
        n_actions = node.get_n_actions()
    except Exception as exc:
        raise RuntimeError(f"node has no actions: {_name(node)}") from exc
    if n_actions <= 0:
        raise RuntimeError(f"node action count is 0: {_name(node)}")

    # Prefer the explicit Press action when provided by Qt/DTK.
    for index in range(n_actions):
        try:
            if (node.get_action_name(index) or "").lower() == "press":
                node.do_action(index)
                return
        except Exception:
            continue
    node.do_action(0)


def _wait_for_name(root, name: str, timeout: float, visible: bool = False):
    deadline = time.time() + timeout
    last_error = None
    while time.time() < deadline:
        try:
            return _find_by_name(root, name, visible=visible)
        except RuntimeError as exc:
            last_error = exc
            time.sleep(0.1)
    suffix = " visible" if visible else ""
    raise last_error or RuntimeError(f"AT-SPI{suffix} node not found by name: {name}")


def open_titlebar_menu(app_name: str, timeout: float):
    app = _find_app(app_name)
    titlebar = _wait_for_name(app, "WebViewTitleBar", timeout, visible=True)
    buttons = [n for n in _walk(titlebar) if _role(n) == "button" and _visible(n)]
    if not buttons:
        raise RuntimeError("no visible button under WebViewTitleBar")
    _press(buttons[0])
    # QML Menu items can remain in the AT-SPI tree after a previous menu closes.
    # Treat the menu as opened only when the concrete item is visible.
    _wait_for_name(app, "SettingsMenuItem", timeout, visible=True)
    return app


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", default="deepin-voice-note")
    parser.add_argument("--select", help="accessible name of menu item to press after opening")
    parser.add_argument("--wait-visible", help="accessible name that must become visible after selection")
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()

    try:
        Atspi.init()
    except Exception:
        pass

    app = open_titlebar_menu(args.app, args.timeout)
    # TitleBarMenu is a QML Menu root and is not exposed stably by AT-SPI.
    # Wait for a concrete menu item instead.
    wait_name = args.select or "SettingsMenuItem"
    item = _wait_for_name(app, wait_name, args.timeout, visible=True)
    if args.select:
        try:
            _press(item)
        except Exception as exc:
            # Selecting ExitMenuItem closes the application immediately; some
            # AT-SPI backends report the disappearing app as an exception after
            # the Press action has already taken effect.
            if args.select == "ExitMenuItem" and "no longer exists" in str(exc):
                return 0
            raise
        if args.wait_visible:
            app = _find_app(args.app)
            _wait_for_name(app, args.wait_visible, args.timeout, visible=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"titlebar_menu_press.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
