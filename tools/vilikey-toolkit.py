#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ViliKey – Chuyển mã: cửa sổ tương đương "UniKey Toolkit".

Chuyển văn bản trong clipboard hoặc trong file giữa các bảng mã
(TCVN3/ABC, VNI Windows, VISCII, VIQR... và Unicode), kèm tuỳ chọn
sang chữ hoa, sang chữ thường, bỏ dấu. Việc chuyển mã do `vilikey-convert`
(engine VnConv của UniKey) thực hiện.
"""
import os
import shutil
import subprocess
import sys

import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gio, GLib, Gtk  # noqa: E402

CONVERTER = shutil.which("vilikey-convert") or "@VN_BINDIR@/vilikey-convert"

# (mã, nhãn, là bảng mã cũ 8-bit)
CHARSETS = [
    ("unicode",            "Unicode dựng sẵn",            False),
    ("unicode-decomposed", "Unicode tổ hợp",              False),
    ("tcvn3",              "TCVN3 (ABC) — font .VnTime…", True),
    ("vni",                "VNI Windows — font VNI-Times…", True),
    ("viscii",             "VISCII",                      True),
    ("vps",                "VPS",                         True),
    ("viqr",               "VIQR",                        False),
    ("cp1258",             "Windows CP 1258",             True),
    ("ncr-dec",            "NCR Decimal (&#7871;)",       False),
    ("ncr-hex",            "NCR Hex (&#x1EBF;)",          False),
]


def run_convert(src, dst, data, options, from_text=False, to_text=False):
    """Gọi vilikey-convert, trả về (bytes_ra, lỗi)."""
    args = [CONVERTER, "-f", src, "-t", dst] + options
    if from_text:
        args.append("--from-text")
    if to_text:
        args.append("--to-text")
    try:
        p = subprocess.run(args, input=data, capture_output=True, timeout=60)
    except (OSError, subprocess.TimeoutExpired) as e:
        return None, str(e)
    if p.returncode != 0:
        return None, p.stderr.decode("utf-8", "replace").strip() or "Lỗi không rõ"
    return p.stdout, None


class Toolkit(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app, title="ViliKey – Chuyển mã")
        self.set_default_size(520, -1)
        self.set_resizable(False)
        self.in_file = None

        root = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14,
                       margin_top=18, margin_bottom=18, margin_start=18, margin_end=18)
        self.set_child(root)

        # --- Bảng mã nguồn / đích -----------------------------------------
        grid = Gtk.Grid(column_spacing=10, row_spacing=8)
        names = Gtk.StringList.new([c[1] for c in CHARSETS])
        self.src = Gtk.DropDown(model=names, hexpand=True)
        self.dst = Gtk.DropDown(model=Gtk.StringList.new([c[1] for c in CHARSETS]), hexpand=True)
        self.src.set_selected(2)   # TCVN3
        self.dst.set_selected(0)   # Unicode
        swap = Gtk.Button(icon_name="object-flip-vertical-symbolic",
                          tooltip_text="Đảo nguồn và đích", valign=Gtk.Align.CENTER)
        swap.connect("clicked", self.on_swap)
        grid.attach(Gtk.Label(label="Nguồn:", xalign=0), 0, 0, 1, 1)
        grid.attach(self.src, 1, 0, 1, 1)
        grid.attach(Gtk.Label(label="Đích:", xalign=0), 0, 1, 1, 1)
        grid.attach(self.dst, 1, 1, 1, 1)
        grid.attach(swap, 2, 0, 1, 2)
        root.append(grid)

        # --- Tuỳ chọn ------------------------------------------------------
        opts = Gtk.Box(spacing=16)
        self.opt_upper = Gtk.CheckButton(label="Sang chữ hoa")
        self.opt_lower = Gtk.CheckButton(label="Sang chữ thường")
        self.opt_strip = Gtk.CheckButton(label="Loại bỏ dấu")
        self.opt_upper.connect("toggled", self.on_case, self.opt_lower)
        self.opt_lower.connect("toggled", self.on_case, self.opt_upper)
        for w in (self.opt_upper, self.opt_lower, self.opt_strip):
            opts.append(w)
        root.append(opts)

        root.append(Gtk.Separator())

        # --- Clipboard -----------------------------------------------------
        clip_btn = Gtk.Button(label="Chuyển mã clipboard")
        clip_btn.add_css_class("suggested-action")
        clip_btn.connect("clicked", self.on_clipboard)
        hint = Gtk.Label(xalign=0, wrap=True,
                         label="Sao chép (Ctrl+C) đoạn văn bản cần chuyển, bấm nút trên, "
                               "rồi dán (Ctrl+V) vào nơi cần dùng.")
        hint.add_css_class("dim-label")
        root.append(clip_btn)
        root.append(hint)

        root.append(Gtk.Separator())

        # --- File ----------------------------------------------------------
        file_box = Gtk.Box(spacing=8)
        self.file_label = Gtk.Label(label="Chưa chọn file (.txt, .html, .csv…)",
                                    xalign=0, hexpand=True, ellipsize=3)
        pick = Gtk.Button(label="Chọn file…")
        pick.connect("clicked", self.on_pick)
        file_box.append(self.file_label)
        file_box.append(pick)
        root.append(file_box)
        self.file_btn = Gtk.Button(label="Chuyển mã file", sensitive=False)
        self.file_btn.connect("clicked", self.on_file)
        root.append(self.file_btn)

        # --- Trạng thái ----------------------------------------------------
        self.status = Gtk.Label(xalign=0, wrap=True, selectable=True)
        root.append(self.status)

    # ------------------------------------------------------------------
    def charset(self, dropdown):
        return CHARSETS[dropdown.get_selected()]

    def options(self):
        o = []
        if self.opt_upper.get_active():
            o.append("--upper")
        if self.opt_lower.get_active():
            o.append("--lower")
        if self.opt_strip.get_active():
            o.append("--strip")
        return o

    def say(self, text, error=False):
        self.status.set_text(text)
        self.status.remove_css_class("error")
        self.status.remove_css_class("success")
        self.status.add_css_class("error" if error else "success")

    def on_swap(self, _btn):
        a, b = self.src.get_selected(), self.dst.get_selected()
        self.src.set_selected(b)
        self.dst.set_selected(a)

    def on_case(self, btn, other):
        if btn.get_active():
            other.set_active(False)

    # --- Clipboard -------------------------------------------------------
    def on_clipboard(self, _btn):
        self.get_clipboard().read_text_async(None, self.on_clip_text)

    def on_clip_text(self, clipboard, result):
        try:
            text = clipboard.read_text_finish(result)
        except GLib.Error as e:
            self.say(f"Không đọc được clipboard: {e.message}", True)
            return
        if not text:
            self.say("Clipboard không có văn bản.", True)
            return
        src, dst = self.charset(self.src), self.charset(self.dst)
        out, err = run_convert(src[0], dst[0], text.encode("utf-8"), self.options(),
                               from_text=src[2], to_text=dst[2])
        if err:
            self.say(f"Lỗi: {err}", True)
            return
        clipboard.set(out.decode("utf-8", "replace"))
        self.say(f"Đã chuyển {len(text)} ký tự từ {src[1]} sang {dst[1]}. "
                 "Kết quả đã nằm trong clipboard.")

    # --- File ------------------------------------------------------------
    def on_pick(self, _btn):
        dlg = Gtk.FileDialog(title="Chọn file cần chuyển mã")
        dlg.open(self, None, self.on_picked)

    def on_picked(self, dlg, result):
        try:
            f = dlg.open_finish(result)
        except GLib.Error:
            return
        self.in_file = f.get_path()
        self.file_label.set_text(os.path.basename(self.in_file))
        self.file_btn.set_sensitive(True)

    def on_file(self, _btn):
        if not self.in_file:
            return
        src, dst = self.charset(self.src), self.charset(self.dst)
        base, ext = os.path.splitext(self.in_file)
        dlg = Gtk.FileDialog(title="Lưu kết quả",
                             initial_name=f"{os.path.basename(base)}-{dst[0]}{ext}")
        dlg.set_initial_folder(Gio.File.new_for_path(os.path.dirname(self.in_file)))
        dlg.save(self, None, self.on_save, (src, dst))

    def on_save(self, dlg, result, pair):
        try:
            out_path = dlg.save_finish(result).get_path()
        except GLib.Error:
            return
        src, dst = pair
        try:
            with open(self.in_file, "rb") as f:
                data = f.read()
        except OSError as e:
            self.say(f"Không đọc được file: {e}", True)
            return
        out, err = run_convert(src[0], dst[0], data, self.options())
        if err:
            self.say(f"Lỗi: {err}", True)
            return
        try:
            with open(out_path, "wb") as f:
                f.write(out)
        except OSError as e:
            self.say(f"Không ghi được file: {e}", True)
            return
        self.say(f"Đã lưu {os.path.basename(out_path)} ({dst[1]}).")


class App(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="io.github.ibus_vilikey.Toolkit",
                         flags=Gio.ApplicationFlags.DEFAULT_FLAGS)

    def do_activate(self):
        win = self.props.active_window or Toolkit(self)
        win.present()


if __name__ == "__main__":
    sys.exit(App().run(sys.argv))
