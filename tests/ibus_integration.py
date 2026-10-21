#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Kiểm thử tích hợp: chạy ibus-engine-vilikey thật dưới ibus-daemon và gõ phím
như một ứng dụng (input context) thật, rồi so sánh văn bản thu được.

Cách chạy (cần ibus, python3-gi, gir1.2-ibus-1.0, dbus):
    dbus-run-session -- tests/run_integration.sh build/ibus-engine-vilikey
"""
import os
import sys
import time

import gi
gi.require_version("IBus", "1.0")
from gi.repository import GLib, IBus  # noqa: E402


class FakeApp:
    """Mô phỏng một ô nhập văn bản: nhận commit / preedit / delete-surrounding."""

    def __init__(self, bus, surrounding):
        self.text = ""
        self.preedit = ""
        self.ic = bus.create_input_context("vn-test")
        caps = IBus.Capabilite.PREEDIT_TEXT | IBus.Capabilite.FOCUS
        if surrounding:
            caps |= IBus.Capabilite.SURROUNDING_TEXT
        self.ic.set_capabilities(caps)
        self.ic.connect("commit-text", self.on_commit)
        self.ic.connect("update-preedit-text", self.on_preedit)
        self.ic.connect("hide-preedit-text", self.on_hide_preedit)
        self.ic.connect("delete-surrounding-text", self.on_delete)
        self.ic.focus_in()
        self.ic.set_engine("vilikey")
        self.pump()

    def on_commit(self, ic, text):
        self.text += text.get_text()

    def on_preedit(self, ic, text, cursor, visible):
        self.preedit = text.get_text() if visible else ""

    def on_hide_preedit(self, ic):
        self.preedit = ""

    def on_delete(self, ic, offset, nchars):
        assert offset < 0 and nchars == -offset, (offset, nchars)
        self.text = self.text[:len(self.text) - nchars]

    def pump(self, t=0.03):
        end = time.time() + t
        ctx = GLib.MainContext.default()
        while time.time() < end:
            while ctx.pending():
                ctx.iteration(False)
            time.sleep(0.002)

    def key(self, keyval, state=0):
        handled = self.ic.process_key_event(keyval, 0, state)
        self.pump(0.01)
        if not handled and not (state & IBus.ModifierType.RELEASE_MASK):
            # Ứng dụng tự xử lý phím mà IBus không dùng.
            if keyval == IBus.KEY_BackSpace:
                self.text = self.text[:-1]
            elif 0x20 <= keyval <= 0x7E and not (state & IBus.ModifierType.CONTROL_MASK):
                self.text += chr(keyval)
        self.ic.process_key_event(keyval, 0, state | IBus.ModifierType.RELEASE_MASK)
        self.pump(0.005)
        return handled

    def type(self, keys):
        i = 0
        while i < len(keys):
            if keys.startswith("<BS>", i):
                self.key(IBus.KEY_BackSpace)
                i += 4
                continue
            ch = keys[i]
            state = IBus.ModifierType.SHIFT_MASK if ch.isupper() else 0
            self.key(ord(ch), state)
            i += 1

    def visible(self):
        return self.text + self.preedit

    def finish(self):
        self.key(IBus.KEY_Return)   # Enter: chốt preedit
        self.pump(0.05)
        return self.text


CASES = [
    ("tieengs vieetj ", "tiếng việt "),
    ("Truwowngf DDaij hocj Caanf Thow", "Trường Đại học Cần Thơ"),
    ("chuyeenr ddooir soos", "chuyển đổi số"),
    ("windows linux ", "windows linux "),
    ("vieetj<BS><BS>", "vi"),
    ("hoaf binhf", "hòa bình"),
]


def run_suite(bus, label, surrounding):
    fails = 0
    print(f"== {label}")
    for keys, expect in CASES:
        app = FakeApp(bus, surrounding)
        app.type(keys)
        got = app.finish()
        ok = got == expect
        fails += 0 if ok else 1
        print(f"  {'OK  ' if ok else 'FAIL'} {keys!r:38} -> {got!r}" +
              ("" if ok else f"   (mong đợi {expect!r})"))
        app.ic.focus_out()
        app.ic.destroy()
    return fails


def test_no_language_toggle(bus):
    """Ctrl+Shift không còn chuyển Việt/Anh: vẫn gõ tiếng Việt sau khi nhấn."""
    print("== Không có chế độ Việt/Anh riêng (Ctrl+Shift không làm gì)")
    app = FakeApp(bus, surrounding=False)
    app.type("vieetj ")
    C, S, R = (IBus.ModifierType.CONTROL_MASK, IBus.ModifierType.SHIFT_MASK,
               IBus.ModifierType.RELEASE_MASK)
    app.ic.process_key_event(IBus.KEY_Control_L, 0, 0)
    app.ic.process_key_event(IBus.KEY_Shift_L, 0, C)
    app.ic.process_key_event(IBus.KEY_Shift_L, 0, C | S | R)
    app.ic.process_key_event(IBus.KEY_Control_L, 0, C | R)
    app.pump(0.05)
    app.type("Nam")
    got = app.finish()
    ok = got == "việt Nam"
    print(f"  {'OK  ' if ok else 'FAIL'} 'vieetj ' [Ctrl+Shift] 'Nam' -> {got!r}")
    app.ic.destroy()
    return 0 if ok else 1


def test_engine_desc(bus):
    print("== Thông tin engine")
    desc = bus.get_engines_by_names(["vilikey"])
    fails = 0
    for label, got, want in [
        ("ký hiệu", desc[0].get_symbol() if desc else None, "vi"),
        ("tên hiển thị", desc[0].get_longname() if desc else None, "ViliKey"),
        ("ngôn ngữ", desc[0].get_language() if desc else None, "vi"),
    ]:
        ok = got == want
        fails += 0 if ok else 1
        print(f"  {'OK  ' if ok else 'FAIL'} {label}: {got!r}")
    return fails


def main():
    IBus.init()
    bus = IBus.Bus()
    for _ in range(100):
        if bus.is_connected():
            break
        time.sleep(0.1)
        bus = IBus.Bus()
    if not bus.is_connected():
        print("không kết nối được ibus-daemon")
        return 2

    mode = sys.argv[1] if len(sys.argv) > 1 else "preedit"
    fails = 0
    if mode == "preedit":
        fails += test_engine_desc(bus)
        fails += run_suite(bus, "Chế độ Preedit", surrounding=False)
        fails += test_no_language_toggle(bus)
    else:
        fails += run_suite(bus, "Chế độ Gõ trực tiếp (ứng dụng hỗ trợ surrounding text)", True)
        fails += run_suite(bus, "Chế độ Gõ trực tiếp, ứng dụng KHÔNG hỗ trợ -> tự về Preedit", False)
    print("KẾT QUẢ:", "đạt" if fails == 0 else f"{fails} lỗi")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
