// SPDX-License-Identifier: GPL-3.0-or-later
// vilikey-type: thử engine gõ tiếng Việt ngay trên terminal, không cần IBus.
//
//   echo "tieengs vieetj" | vilikey-type            -> tiếng việt
//   echo "tie61ng vie65t" | vilikey-type --vni      -> tiếng việt
//   Ký hiệu đặc biệt trong dữ liệu vào: "<BS>" = BackSpace, "<S-SPC>" = Shift+Space.
#include "core.h"

#include <cctype>
#include <cstring>
#include <iostream>
#include <string>

using vn::Core;

std::string typeString(Core& core, const std::string& keys) {
    std::u32string doc;
    core.reset();
    for (size_t i = 0; i < keys.size(); ++i) {
        vn::Edit e;
        if (keys.compare(i, 4, "<BS>") == 0) {
            e = core.backspace();
            if (!e.handled) e.erase = 1;   // ứng dụng tự xoá 1 ký tự
            i += 3;
        } else if (keys.compare(i, 7, "<S-SPC>") == 0) {
            e = core.key(' ', true, false);  // Shift+Space
            i += 6;
        } else {
            unsigned char c = static_cast<unsigned char>(keys[i]);
            e = core.key(c, std::isupper(c) != 0, false);
        }
        size_t n = static_cast<size_t>(e.erase);
        doc.erase(n >= doc.size() ? 0 : doc.size() - n);
        doc += e.insert;
    }
    return Core::toUtf8(doc);
}

int main(int argc, char** argv) {
    vn::Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--vni") opt.inputMethod = vn::InputMethod::Vni;
        else if (a == "--viqr") opt.inputMethod = vn::InputMethod::Viqr;
        else if (a == "--telex-gian-luoc" || a == "--stelex") opt.inputMethod = vn::InputMethod::SimpleTelex;
        else if (a == "--no-spell") opt.spellCheck = false;
        else if (a == "--no-restore") opt.autoRestore = false;
        else if (a == "--modern") opt.modernStyle = true;
        else if (a == "--no-standalone-w") opt.standaloneW = false;
        else if (a == "--macro" && i + 1 < argc) { opt.macroEnabled = true; opt.macroFile = argv[++i]; }
        else {
            std::cerr << "Dùng: vilikey-type [--vni|--viqr|--telex-gian-luoc] [--no-spell] [--no-restore] "
                         "[--modern] [--macro FILE]\n";
            return 2;
        }
    }
    Core core;
    core.apply(opt);
    std::string line;
    while (std::getline(std::cin, line))
        std::cout << typeString(core, line) << "\n";
    return 0;
}
