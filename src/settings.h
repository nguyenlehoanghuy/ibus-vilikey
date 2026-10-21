// SPDX-License-Identifier: GPL-3.0-or-later
// Cấu hình người dùng, lưu tại ~/.config/ibus-vilikey/config.ini
#pragma once

#include <string>
#include "core.h"

namespace vn {

enum class OutputMode {
    Preedit = 0,   // Từ đang gõ nằm trong preedit (ổn định nhất)
    Direct,        // Sửa trực tiếp trong ứng dụng, giống UniKey trên Windows
};

struct Settings {
    Options core;
    OutputMode mode = OutputMode::Preedit;
    bool underline = false;        // Gạch chân từ đang gõ (Preedit)

    static std::string configDir();
    static std::string configFile();
    static std::string macroFile();

    void load();
    void save() const;
};

} // namespace vn
