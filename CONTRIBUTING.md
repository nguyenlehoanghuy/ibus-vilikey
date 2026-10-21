# Đóng góp cho ViliKey

Cảm ơn bạn muốn góp sức! Mọi đóng góp (báo lỗi, sửa lỗi, tài liệu, dịch) đều được hoan nghênh.

## Báo lỗi
Mở *Issue* theo mẫu "Báo lỗi". Lỗi gõ sai hãy ghi rõ **chuỗi phím đã gõ**, **kết quả mong đợi**
và **kết quả thực tế**, cùng tên ứng dụng.

## Gửi mã
1. Build và chạy test trước khi gửi:
   ```bash
   cmake -S . -B build && cmake --build build -j"$(nproc)"
   ctest --test-dir build --output-on-failure
   dbus-run-session -- tests/run_integration.sh build/ibus-engine-vilikey
   ```
2. Lỗi gõ sai: thêm một dòng vào `tests/test_core.cpp` tái hiện lỗi trước khi sửa.
3. Sửa đổi trong `ukengine/` phải ghi lại trong `ukengine/README` (yêu cầu của giấy phép GPL).
4. Mỗi pull request nên tập trung vào một thay đổi.

## Giấy phép
Đóng góp của bạn được phát hành theo GPL-3.0-or-later như phần còn lại của dự án.
