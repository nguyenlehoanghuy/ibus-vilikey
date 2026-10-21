// SPDX-License-Identifier: GPL-3.0-or-later
// ibus-vilikey — Bộ gõ tiếng Việt cho IBus dùng engine gốc của UniKey (ukengine).
//
// Core: lớp mỏng bọc ukengine, không phụ thuộc IBus.
// Được dùng chung bởi IBus engine, công cụ dòng lệnh và bộ test.
#pragma once

#include <string>
#include <cstdint>

namespace vn {

enum class InputMethod {
    Telex = 0,     // Telex đầy đủ (w -> ư, [ -> ơ, ] -> ư)
    Vni,
    Viqr,
    SimpleTelex,   // Telex giản lược (UniKey 4.6): w đầu từ giữ là w, không xử lý [ ]
};

struct Options {
    InputMethod inputMethod = InputMethod::Telex;
    bool spellCheck      = true;   // Kiểm tra chính tả
    bool autoRestore     = true;   // Tự khôi phục phím với từ không phải tiếng Việt
    bool modernStyle     = false;  // Bỏ dấu kiểu mới: oà, uý (thay vì òa, úy)
    bool freeMarking     = true;   // Cho phép gõ dấu tự do (gõ dấu sau cả từ)
    bool standaloneW     = true;   // Telex: "w" đứng đầu từ thành "ư"
    bool macroEnabled    = false;  // Gõ tắt (Shift+Space để không bung gõ tắt)
    std::string macroFile;         // Đường dẫn file gõ tắt (định dạng Unikey)
};

// Kết quả của một thao tác: xoá `erase` ký tự cuối, rồi chèn `insert`.
struct Edit {
    bool handled = false;   // ukengine có xử lý phím này không
    int erase = 0;          // số ký tự (Unicode) cần xoá ở cuối
    std::u32string insert;  // chuỗi cần chèn thêm
};

class Core {
public:
    Core();
    ~Core();

    void apply(const Options& opt);
    const Options& options() const { return opt_; }

    // Xử lý một ký tự ASCII in được (0x20..0x7E).
    Edit key(uint32_t ch, bool shift, bool capsLock);
    // Đưa ký tự vào mà không qua bộ lọc (dùng cho "w" đứng đầu từ khi tắt tuỳ chọn).
    Edit putChar(uint32_t ch);
    // Xử lý phím BackSpace.
    Edit backspace();
    // Khôi phục chuỗi phím gốc của từ hiện tại (vd: "vieetj" thay vì "việt").
    Edit restoreKeyStrokes();
    // Xoá toàn bộ trạng thái (khi đổi focus, di chuyển con trỏ, commit...).
    void reset();

    bool atWordBeginning() const;

    // Văn bản ukengine đang "nắm" kể từ lần reset gần nhất.
    const std::u32string& text() const { return text_; }

    static bool isWordBreak(uint32_t ch);
    static std::string toUtf8(const std::u32string& s);
    static std::u32string fromUtf8(const std::string& s);

private:
    Edit collect(uint32_t fallbackChar);
    void applyEdit(const Edit& e);

    Options opt_;
    std::u32string text_;
};

} // namespace vn
