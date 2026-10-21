# Nhật ký thay đổi

## 0.2.1 — 2026-10-22
- Đổi tên thành **ViliKey** (`ibus-vilikey`). GNOME hiển thị "Vietnamese (ViliKey)", ký hiệu `vi`.
- Biểu tượng mới; công cụ chuyển mã đổi tên thành "ViliKey – Chuyển mã".

## 0.2.0
- Bỏ chế độ chuyển Việt/Anh riêng: dùng Super+Space của hệ thống (`en` ↔ `vi`).
- Thêm kiểu gõ **Telex giản lược** (như UniKey 4.6).
- **Shift+Space** để không bung gõ tắt.
- Thêm **công cụ chuyển mã** (cửa sổ GTK4 và lệnh `vilikey-convert`): TCVN3, VNI, VISCII, VPS, VIQR, CP1258, NCR, Unicode tổ hợp ↔ Unicode; sang chữ hoa, chữ thường, bỏ dấu.

## 0.1.x
- Bản đầu tiên: IBus engine dùng ukengine; Telex, VNI, VIQR; kiểm tra chính tả; tự khôi phục từ tiếng Anh; gõ tắt định dạng UniKey; chế độ Preedit và Gõ trực tiếp.
