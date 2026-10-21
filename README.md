<p align="center">
  <img src="data/icons/ibus-vilikey.svg" width="96" alt="ViliKey">
</p>

<h1 align="center">ViliKey</h1>
<p align="center"><b>Bộ gõ tiếng Việt cho Linux — gõ như UniKey</b><br>
IBus · Telex · VNI · VIQR · engine gốc của UniKey</p>

<p align="center"><a href="README.en.md">English</a></p>

---

**ViliKey** (*Vi*ệt + *Li*nux) là bộ gõ tiếng Việt cho các bản Linux dùng **IBus**, như Ubuntu, Fedora hay Debian với GNOME. Phần xử lý tiếng Việt dùng **ukengine**, engine gốc của UniKey do Phạm Kim Long công bố mã nguồn. Vì vậy cách đặt dấu, kiểm tra chính tả và khôi phục từ tiếng Anh giống UniKey trên Windows.

ViliKey chỉ làm một việc: **gõ tiếng Việt**. Muốn gõ tiếng Anh, chuyển nguồn nhập bằng **Super+Space** (`en` ↔ `vi`).

## Tính năng

- **Kiểu gõ:** Telex, Telex giản lược (như UniKey 4.6), VNI, VIQR
- **Như UniKey:** kiểm tra chính tả, tự khôi phục phím với từ sai (`windows` không thành `ưindows`), gõ dấu tự do, đặt dấu *oà, uý* hoặc *òa, úy*
- **Gõ tắt:** dùng chung file gõ tắt của UniKey; tự đổi hoa/thường theo cách gõ (`ctu` / `Ctu` / `CTU`); **Shift+Space** để không bung gõ tắt
- **Chuyển mã** (như UniKey Toolkit): TCVN3 (ABC), VNI Windows, VISCII, VPS, VIQR, CP1258, NCR, Unicode tổ hợp ↔ Unicode; sang chữ hoa, chữ thường, bỏ dấu; chuyển clipboard hoặc file
- **Hai chế độ hiển thị:** *Preedit* (mặc định, ổn định trên mọi ứng dụng) hoặc *Gõ trực tiếp* (sửa thẳng trong ứng dụng, giống UniKey Windows)

## Cài đặt

### Ubuntu 24.04 trở lên (gói .deb)

Tải file `ibus-vilikey_*.deb` ở trang **Releases**, rồi chạy:

```bash
sudo apt install ./ibus-vilikey_*_amd64.deb
gsettings set org.gnome.desktop.input-sources sources "[('xkb', 'us'), ('ibus', 'vilikey')]"
ibus restart
```

Bạn cũng có thể thêm bằng giao diện: **Settings → Keyboard → Input Sources → + → Vietnamese → ViliKey**.

Trên thanh trạng thái, `vi` là đang gõ tiếng Việt và `en` là tiếng Anh. Nhấn **Super+Space** để chuyển.

> Trên GNOME phải thêm ViliKey vào Input Sources như trên. Lệnh `ibus engine vilikey` không có tác dụng lâu dài, vì GNOME Shell sẽ đặt lại engine theo danh sách của nó.

### Build từ mã nguồn (mọi bản phân phối)

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake pkg-config libibus-1.0-dev libglib2.0-dev
# Fedora
# sudo dnf install gcc-c++ cmake pkgconf ibus-devel glib2-devel

cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j"$(nproc)"
sudo cmake --install build
ibus restart
```

### Gỡ cài đặt

```bash
sudo apt purge ibus-vilikey
rm -rf ~/.config/ibus-vilikey
```

## Sử dụng

Bấm vào `vi` trên thanh trạng thái để mở menu:

| Mục | Nội dung |
|---|---|
| Kiểu gõ | Telex · Telex giản lược · VNI · VIQR |
| Chế độ | Preedit · Gõ trực tiếp |
| Tuỳ chọn | Kiểm tra chính tả · Tự khôi phục phím với từ sai · Gõ dấu tự do · Đặt dấu oà, uý · Telex: w đầu từ thành ư · Gạch chân từ đang gõ |
| Gõ tắt | Bật/tắt · Sửa bảng gõ tắt · Nạp lại |
| Công cụ chuyển mã… | Mở cửa sổ **ViliKey – Chuyển mã** |

### Chuyển mã văn bản cũ (font .VnTime, VNI-Times…)

1. Mở tài liệu cũ, chọn đoạn văn bản và nhấn **Ctrl+C**.
2. Mở **ViliKey – Chuyển mã**, chọn Nguồn là **TCVN3** (font .Vn…) hoặc **VNI Windows** (font VNI-…), Đích là **Unicode dựng sẵn**, rồi bấm **Chuyển mã clipboard**.
3. Dán (**Ctrl+V**) vào tài liệu mới và đổi sang font Unicode.

Trên dòng lệnh:

```bash
vilikey-convert -f tcvn3 -t unicode cu.txt moi.txt
vilikey-convert -f vni --from-text < dan-tu-word.txt
echo "Đại học Cần Thơ" | vilikey-convert -f unicode --strip    # Dai hoc Can Tho
vilikey-convert --list
```

### Gõ tắt

Bảng gõ tắt nằm tại `~/.config/ibus-vilikey/macro.txt`, cùng định dạng với UniKey Windows, nên có thể chép file gõ tắt cũ vào dùng luôn:

```
;DO NOT DELETE THIS LINE***version=1***
vn:Việt Nam
ko:không
```

### Cấu hình

Cấu hình lưu tại `~/.config/ibus-vilikey/config.ini` và thường chỉnh qua menu:

```ini
[vilikey]
input-method=telex      # telex | simple-telex | vni | viqr
mode=preedit            # preedit | direct
spell-check=true
auto-restore=true
free-marking=true
modern-style=false      # true: oà, uý
standalone-w=true       # Telex: w đầu từ -> ư
underline=false
macro=false
```

## Chọn chế độ nào?

- **Preedit** (mặc định): từ đang gõ nằm tạm trong vùng soạn thảo, và được đưa vào ứng dụng khi gõ dấu cách hoặc dấu câu. Chạy tốt trên mọi ứng dụng, kể cả terminal, trình duyệt, VS Code và các ứng dụng Electron.
- **Gõ trực tiếp**: sửa chữ ngay trong ứng dụng, giống UniKey Windows. Chế độ này cần ứng dụng hỗ trợ *surrounding text*. Với ứng dụng không hỗ trợ, ViliKey tự quay về Preedit.

## Phát triển

```bash
cmake -S . -B build && cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure                       # lõi gõ + chuyển mã
dbus-run-session -- tests/run_integration.sh build/ibus-engine-vilikey   # với ibus-daemon thật
echo "tieengs vieetj" | ./build/vilikey-type                     # thử gõ trên terminal
```

```
ukengine/                   Engine gốc của UniKey (GPLv2+), sửa đổi ghi trong ukengine/README
src/core.*                  Lớp bọc ukengine, không phụ thuộc IBus
src/engine.*                IBus engine: xử lý phím, Preedit/Gõ trực tiếp, menu
src/settings.*              Cấu hình người dùng
tools/vilikey-type.cpp      Thử gõ trên terminal
tools/vilikey-convert.cpp   Chuyển mã (VnConv của ukengine)
tools/vilikey-toolkit.py    Cửa sổ chuyển mã (GTK4)
tests/                      Test lõi, chuyển mã, tích hợp IBus
```

Xem thêm [CONTRIBUTING.md](CONTRIBUTING.md) và [CHANGELOG.md](CHANGELOG.md).

## Giấy phép và ghi công

ViliKey phát hành theo **GPL-3.0-or-later** (xem [LICENSE](LICENSE)).

Engine `ukengine/` là của **Phạm Kim Long** (UniKey), phát hành theo GPL v2 trở lên, được lấy qua dự án [ibus-unikey](https://github.com/vn-input/ibus-unikey). Chi tiết xem [CREDITS.md](CREDITS.md).

ViliKey không phải là sản phẩm chính thức của UniKey.
