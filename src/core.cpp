// SPDX-License-Identifier: GPL-3.0-or-later
#include "core.h"

#include <glib.h>
#include <cstring>

#include "unikey.h"
#include "vnconv.h"

namespace vn {

namespace {

// Ký tự ngắt từ (giống x-unikey / Unikey Windows).
const char kWordBreak[] = ",;:.\"'!?<>=+-*/\\_~`@#$%^&(){}[]| ";

// ukengine là một engine toàn cục (singleton), chỉ cần khởi tạo một lần.
int g_refcount = 0;

UkInputMethod toUk(InputMethod im) {
    switch (im) {
    case InputMethod::Vni:         return UkVni;
    case InputMethod::Viqr:        return UkViqr;
    case InputMethod::SimpleTelex: return UkSimpleTelex2;
    case InputMethod::Telex:
    default:                       return UkTelex;
    }
}

} // namespace

Core::Core() {
    if (g_refcount++ == 0) {
        UnikeySetup();                               // mặc định: Telex, UTF-8
        UnikeySetOutputCharset(CONV_CHARSET_XUTF8);  // luôn xuất Unicode UTF-8
    }
    apply(opt_);
}

Core::~Core() {
    if (--g_refcount == 0)
        UnikeyCleanup();
}

void Core::apply(const Options& opt) {
    opt_ = opt;

    UnikeyOptions uk;
    CreateDefaultUnikeyOptions(&uk);
    uk.freeMarking       = opt.freeMarking;
    uk.modernStyle       = opt.modernStyle;
    uk.macroEnabled      = opt.macroEnabled;
    uk.spellCheckEnabled = opt.spellCheck;
    uk.autoNonVnRestore  = opt.autoRestore;
    UnikeySetOptions(&uk);
    UnikeySetInputMethod(toUk(opt.inputMethod));
    UnikeySetOutputCharset(CONV_CHARSET_XUTF8);

    if (opt.macroEnabled && !opt.macroFile.empty())
        UnikeyLoadMacroTable(opt.macroFile.c_str());

    reset();
}

void Core::reset() {
    UnikeyResetBuf();
    text_.clear();
}

bool Core::atWordBeginning() const {
    return UnikeyAtWordBeginning();
}

bool Core::isWordBreak(uint32_t ch) {
    return ch < 0x80 && ch != 0 && std::strchr(kWordBreak, static_cast<int>(ch)) != nullptr;
}

std::string Core::toUtf8(const std::u32string& s) {
    if (s.empty()) return {};
    gchar* p = g_ucs4_to_utf8(reinterpret_cast<const gunichar*>(s.data()),
                              static_cast<glong>(s.size()), nullptr, nullptr, nullptr);
    std::string out = p ? p : "";
    g_free(p);
    return out;
}

std::u32string Core::fromUtf8(const std::string& s) {
    if (s.empty()) return {};
    glong n = 0;
    gunichar* p = g_utf8_to_ucs4(s.data(), static_cast<glong>(s.size()), nullptr, &n, nullptr);
    std::u32string out;
    if (p) out.assign(reinterpret_cast<char32_t*>(p), static_cast<size_t>(n));
    g_free(p);
    return out;
}

Edit Core::collect(uint32_t fallbackChar) {
    Edit e;
    e.erase = UnikeyBackspaces > 0 ? UnikeyBackspaces : 0;
    if (UnikeyBufChars > 0) {
        e.handled = true;
        e.insert = fromUtf8(std::string(reinterpret_cast<const char*>(UnikeyBuf),
                                        static_cast<size_t>(UnikeyBufChars)));
    } else if (fallbackChar) {
        // ukengine không biến đổi phím -> ký tự được giữ nguyên.
        e.insert.push_back(static_cast<char32_t>(fallbackChar));
    }
    if (e.erase > 0) e.handled = true;
    return e;
}

void Core::applyEdit(const Edit& e) {
    size_t n = static_cast<size_t>(e.erase);
    if (n >= text_.size()) text_.clear();
    else text_.erase(text_.size() - n);
    text_ += e.insert;

    // Chỉ cần nhớ phần đuôi; ukengine không bao giờ sửa xa hơn một từ.
    const size_t kMax = 256;
    if (text_.size() > kMax) text_.erase(0, text_.size() - kMax);
}

Edit Core::key(uint32_t ch, bool shift, bool capsLock) {
    // Telex giản lược (hoặc Telex khi tắt "w đầu từ thành ư"): giữ nguyên w ở đầu từ.
    if ((ch == 'w' || ch == 'W') && atWordBeginning() &&
        (opt_.inputMethod == InputMethod::SimpleTelex ||
         (opt_.inputMethod == InputMethod::Telex && !opt_.standaloneW))) {
        return putChar(ch);
    }
    // Shift+Space: chèn dấu cách mà không bung gõ tắt (giống UniKey).
    if (ch == ' ' && shift && opt_.macroEnabled) {
        UnikeyResetBuf();
        Edit e;
        e.insert.push_back(U' ');
        applyEdit(e);
        return e;
    }
    UnikeySetCapsState(shift ? 1 : 0, capsLock ? 1 : 0);
    UnikeyFilter(ch);
    Edit e = collect(ch);
    applyEdit(e);
    return e;
}

Edit Core::putChar(uint32_t ch) {
    UnikeyPutChar(ch);
    Edit e;
    e.handled = true;
    e.insert.push_back(static_cast<char32_t>(ch));
    applyEdit(e);
    return e;
}

Edit Core::backspace() {
    if (text_.empty()) {
        // Không có gì trong bộ đệm: để ứng dụng tự xử lý BackSpace.
        UnikeyResetBuf();
        return Edit{};
    }
    UnikeyBackspacePress();
    Edit e = collect(0);
    if (e.erase == 0 && e.insert.empty()) {
        // ukengine không theo dõi ký tự này -> xoá 1 ký tự như bình thường.
        e.erase = 1;
    }
    e.handled = true;
    applyEdit(e);
    return e;
}

Edit Core::restoreKeyStrokes() {
    UnikeyRestoreKeyStrokes();
    Edit e = collect(0);
    applyEdit(e);
    return e;
}

} // namespace vn
