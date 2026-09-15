#!/usr/bin/env python3
"""Enter multi-select state in deepin-voice-note without fixed coordinates.

The helper creates/selects visible note rows through the AT-SPI tree.  It uses
runtime extents of list items to click their centers, then Shift-clicks the
second visible item so QML ItemListView enters range multi-select mode.
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


def _wait_for_name(root, name: str, timeout: float):
    deadline = time.time() + timeout
    last_error = None
    while time.time() < deadline:
        try:
            return _find_by_name(root, name)
        except RuntimeError as exc:
            last_error = exc
            time.sleep(0.1)
    raise last_error or RuntimeError(f"AT-SPI node not found by name: {name}")


def _press(node) -> None:
    try:
        n_actions = node.get_n_actions()
    except Exception:
        n_actions = 0
    for index in range(max(n_actions, 0)):
        try:
            if (node.get_action_name(index) or "").lower() == "press":
                node.do_action(index)
                return
        except Exception:
            continue
    if n_actions > 0:
        node.do_action(0)
        return
    # Some QtQuick buttons are visible/clickable but expose no AT-SPI action.
    # Fall back to a runtime-extents center click; this is not a fixed coordinate.
    _click_center(node)


def _click_center(node, modifiers: list[str] | None = None) -> None:
    ext = _visible_extents(node)
    if ext is None:
        raise RuntimeError(f"node is not visible: {_name(node)}")
    x = int(ext.x + ext.width / 2)
    y = int(ext.y + ext.height / 2)
    if modifiers:
        for key in modifiers:
            subprocess.run(["xdotool", "keydown", key], check=True)
    try:
        subprocess.run(["xdotool", "mousemove", str(x), str(y), "click", "1"], check=True)
    finally:
        if modifiers:
            for key in reversed(modifiers):
                subprocess.run(["xdotool", "keyup", key], check=True)
    time.sleep(0.3)


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
        # QML exposes real note rows as list items with the note title; in some
        # AT-SPI backends they appear as focusable labels.  Keep only rows large
        # enough to be clickable and ignore unnamed decorative controls.
        if name and (role in {"list item", "label", "panel"}) and ext.width >= 40 and ext.height >= 20:
            items.append((ext.y, node))
    # De-duplicate by vertical position/name to avoid labels inside one row.
    result = []
    seen_y = set()
    for y, node in sorted(items, key=lambda pair: pair[0]):
        bucket = int(y / 8)
        if bucket in seen_y:
            continue
        seen_y.add(bucket)
        result.append(node)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", default="deepin-voice-note")
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()

    try:
        Atspi.init()
    except Exception:
        pass

    app = _find_app(args.app)
    # Ensure at least two notes exist in the fixture session.
    new_note = _wait_for_name(app, "NewNoteButton", args.timeout)
    _press(new_note)
    time.sleep(0.5)
    _press(new_note)
    time.sleep(0.8)

    deadline = time.time() + args.timeout
    items = []
    while time.time() < deadline:
        items = _visible_note_items(app)
        if len(items) >= 2:
            break
        time.sleep(0.2)
    if len(items) < 2:
        raise RuntimeError(f"need at least two visible note items, got {len(items)}")

    _click_center(items[0])
    _click_center(items[1], modifiers=["Shift_L"])
    _wait_for_name(app, "MultipleChoicesView", args.timeout)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"select_two_notes_for_multiselect.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
