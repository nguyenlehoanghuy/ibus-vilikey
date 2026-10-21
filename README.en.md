<p align="center">
  <img src="data/icons/ibus-vilikey.svg" width="96" alt="ViliKey">
</p>

<h1 align="center">ViliKey</h1>
<p align="center"><b>Vietnamese input method for Linux — types like UniKey</b><br>
IBus · Telex · VNI · VIQR · the original UniKey engine</p>

<p align="center"><a href="README.md">Tiếng Việt</a></p>

---

**ViliKey** (*Vi*etnamese + *Li*nux) is a Vietnamese input method for Linux desktops that use **IBus** (Ubuntu, Fedora, Debian with GNOME, …). It is built on **ukengine**, the input engine of UniKey written by Pham Kim Long, so tone placement, spell checking and restoring of non-Vietnamese words behave like UniKey on Windows.

ViliKey only types Vietnamese. To type English, switch input source with **Super+Space** (`en` ↔ `vi`).

## Features

- **Input methods:** Telex, simple Telex (as in UniKey 4.6), VNI, VIQR
- **UniKey behaviour:** spell checking, automatic restore of non-Vietnamese words (`windows` stays `windows`), free tone placement, modern (*oà, uý*) or classic (*òa, úy*) tone style
- **Macros:** UniKey-compatible macro file, case follows what you type (`ctu` / `Ctu` / `CTU`), **Shift+Space** to skip expansion
- **Converter** (like UniKey Toolkit): TCVN3 (ABC), VNI Windows, VISCII, VPS, VIQR, CP1258, NCR, decomposed Unicode ↔ Unicode; upper/lower case, strip tones; clipboard or files
- **Two display modes:** *preedit* (default, works everywhere) or *direct* editing (like UniKey on Windows; falls back to preedit where unsupported)

## Install

### Ubuntu 24.04+ (.deb)

Download `ibus-vilikey_*.deb` from **Releases**, then:

```bash
sudo apt install ./ibus-vilikey_*_amd64.deb
gsettings set org.gnome.desktop.input-sources sources "[('xkb', 'us'), ('ibus', 'vilikey')]"
ibus restart
```

Or use **Settings → Keyboard → Input Sources → + → Vietnamese → ViliKey**.

### From source

```bash
sudo apt install build-essential cmake pkg-config libibus-1.0-dev libglib2.0-dev
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j"$(nproc)"
sudo cmake --install build
ibus restart
```

## Usage

Click `vi` in the top bar to choose the input method, display mode, options, macros, or open the **ViliKey Converter**.

Command-line converter:

```bash
vilikey-convert -f tcvn3 -t unicode old.txt new.txt
echo "Đại học Cần Thơ" | vilikey-convert -f unicode --strip    # Dai hoc Can Tho
```

Configuration lives in `~/.config/ibus-vilikey/` (`config.ini`, `macro.txt`).

## Development

```bash
cmake -S . -B build && cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
dbus-run-session -- tests/run_integration.sh build/ibus-engine-vilikey
```

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License and credits

ViliKey is released under **GPL-3.0-or-later**. The `ukengine/` directory is UniKey's engine by **Pham Kim Long** (GPL v2 or later), taken via [ibus-unikey](https://github.com/vn-input/ibus-unikey); see [CREDITS.md](CREDITS.md).

ViliKey is not affiliated with or endorsed by the UniKey project.
