---
name: Báo lỗi
about: Gõ sai, không gõ được, hoặc lỗi trong một ứng dụng cụ thể
labels: bug
---

**Mô tả lỗi**
Gõ phím gì, mong đợi ra chữ gì, thực tế ra chữ gì.
Ví dụ: gõ `vieetj` trong Firefox ra `vieetj` thay vì `việt`.

**Môi trường** (chạy lệnh dưới và dán kết quả)
```bash
lsb_release -ds; echo "$XDG_SESSION_TYPE / $XDG_CURRENT_DESKTOP"
dpkg -l ibus-vilikey | tail -1
gsettings get org.gnome.desktop.input-sources sources
cat ~/.config/ibus-vilikey/config.ini
```

**Ứng dụng gặp lỗi** (tên và phiên bản):

**Chế độ:** Preedit / Gõ trực tiếp
