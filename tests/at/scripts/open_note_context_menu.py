#!/usr/bin/env python3
"""Open deepin-voice-note note-list context menu by runtime AT-SPI extents.

This helper avoids fixed screen coordinates: it locates NoteItemListView and
right-clicks the center of a visible note row.  It is used only to expose the
real QML/DTK context menu; menu item selection remains in YAML via AT-SPI.
"""
from __future__ import annotations

import argparse
import subprocess
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


def _find_app(app_name: str):
    desktop = Atspi.get_desktop(0)
    for app in _children(desktop):
        if _name(app) == app_name:
            return app
    raise RuntimeError(f"application not found: {app_name}")


def _find_by_name(root, name: str):
    for node in _walk(root):
        if _name(node) == name:
            return node
    raise RuntimeError(f"AT-SPI node not found by name: {name}")


def _visible_note_items(app):
    list_view = _find_by_name(app, "NoteItemListView")
    items = []
    for node in _walk(list_view):
        if node is list_view:
            continue
        ext = _visible_extents(node)
        if ext is None:
            continue
        role = _role(node)
        name = _name(node)
        if name and (role in {"list item", "label", "panel"}) and ext.width >= 40 and ext.height >= 20:
            items.append((ext.y, node))
    result = []
    seen_y = set()
    for y, node in sorted(items, key=lambda pair: pair[0]):
        bucket = int(y / 8)
        if bucket in seen_y:
            continue
        seen_y.add(bucket)
        result.append(node)
    return result


def _right_click_center(node) -> None:
    ext = _visible_extents(node)
    if ext is None:
        raise RuntimeError(f"node is not visible: {_name(node)}")
    x = int(ext.x + ext.width / 2)
    y = int(ext.y + ext.height / 2)
    subprocess.run(["xdotool", "mousemove", str(x), str(y), "click", "3"], check=True)
    time.sleep(0.5)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", default="deepin-voice-note")
    parser.add_argument("--index", type=int, default=0)
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

    try:
        Atspi.init()
    except Exception:
        pass

    deadline = time.time() + args.timeout
    last_count = 0
    while time.time() < deadline:
        app = _find_app(args.app)
        items = _visible_note_items(app)
        last_count = len(items)
        if len(items) > args.index:
            _right_click_center(items[args.index])
            return 0
        time.sleep(0.2)
    raise RuntimeError(f"need visible note item at index {args.index}, got {last_count}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"open_note_context_menu.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
