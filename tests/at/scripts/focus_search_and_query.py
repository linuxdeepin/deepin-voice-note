#!/usr/bin/env python3
"""Focus the visible search edit and type a query via AT-SPI + xdotool."""
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
    out = []
    for i in range(count):
        try:
            child = node.get_child_at_index(i)
        except Exception:
            continue
        if child is not None:
            out.append(child)
    return out


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


def _states(node):
    try:
        return {node.get_state_set().get_states()[i].value_nick for i in range(len(node.get_state_set().get_states()))}
    except Exception:
        try:
            ss = node.get_state_set()
            names = set()
            for state in (Atspi.StateType.EDITABLE, Atspi.StateType.SHOWING, Atspi.StateType.VISIBLE, Atspi.StateType.ENABLED):
                if ss.contains(state):
                    names.add(str(state.value_nick))
            return names
        except Exception:
            return set()


def _ext(node):
    try:
        e = node.get_extents(Atspi.CoordType.SCREEN)
    except Exception:
        return None
    if e.width <= 0 or e.height <= 0:
        return None
    return e


def _find_app(app_name: str):
    desktop = Atspi.get_desktop(0)
    for app in _children(desktop):
        if _name(app) == app_name:
            return app
    raise RuntimeError(f"application not found: {app_name}")


def _do_action(node, action_name: str) -> bool:
    try:
        count = node.get_n_actions()
    except Exception:
        return False
    for i in range(max(count, 0)):
        try:
            if (node.get_action_name(i) or "").lower() == action_name.lower():
                node.do_action(i)
                return True
        except Exception:
            continue
    return False


def _press_named(app, name: str) -> bool:
    for node in _walk(app):
        if _name(node) == name:
            return _do_action(node, "Press") or _do_action(node, "SetFocus")
    return False


def _candidate_search_edits(app):
    candidates = []
    for node in _walk(app):
        role = _role(node)
        states = _states(node)
        e = _ext(node)
        if e is None:
            continue
        if role == "text" and ("editable" in states or Atspi.StateType.EDITABLE.value_nick in states):
            # Search edit is the small top-middle text field.  The WebEngine
            # editor is much larger and lower, so sort by y then height.
            candidates.append((e.y, e.height, e.width, node))
    return [n for *_ignore, n in sorted(candidates, key=lambda item: (item[0], item[1], item[2]))]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("query")
    parser.add_argument("--app", default="deepin-voice-note")
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()
    try:
        Atspi.init()
    except Exception:
        pass
    app = _find_app(args.app)
    # If the search edit is hidden in narrow mode, the toolbar button exposes it.
    _press_named(app, "ToolBarMoreButton")
    time.sleep(0.3)
    deadline = time.time() + args.timeout
    candidates = []
    while time.time() < deadline:
        candidates = _candidate_search_edits(app)
        if candidates:
            break
        time.sleep(0.1)
    if not candidates:
        raise RuntimeError("no visible editable search field found")
    search = candidates[0]
    _do_action(search, "SetFocus")
    time.sleep(0.2)
    subprocess.run(["xdotool", "key", "ctrl+a"], check=False)
    subprocess.run(["xdotool", "type", "--delay", "1", args.query], check=True)
    subprocess.run(["xdotool", "key", "Return"], check=True)
    time.sleep(1.0)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"focus_search_and_query.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
