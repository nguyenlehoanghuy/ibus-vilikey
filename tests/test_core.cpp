// SPDX-License-Identifier: GPL-3.0-or-later
// Bộ test cho lõi gõ: chuỗi phím -> kết quả mong đợi.
#include "core.h"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using vn::Core;
using vn::InputMethod;

static std::string typeString(Core& core, const std::string& keys) {
    std::u32string doc;
    core.reset();
    for (size_t i = 0; i < keys.size(); ++i) {
        vn::Edit e;
        if (keys.compare(i, 4, "<BS>") == 0) {
            e = core.backspace();
            if (!e.handled) e.erase = 1;
            i += 3;
        } else if (keys.compare(i, 7, "<S-SPC>") == 0) {
            e = core.key(' ', true, false);   // Shift+Space
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

struct Case { const char* keys; const char* expect; };

static int g_fail = 0, g_total = 0;

static void run(const char* title, const vn::Options& opt, const std::vector<Case>& cases) {
    Core core;
    core.apply(opt);
    std::printf("== %s\n", title);
    for (const auto& c : cases) {
        ++g_total;
        std::string got = typeString(core, c.keys);
        bool ok = got == c.expect;
        if (!ok) ++g_fail;
        std::printf("  %s %-28s -> %-22s%s%s\n", ok ? "OK  " : "FAIL", c.keys, got.c_str(),
                    ok ? "" : "  (mong đợi: ", ok ? "" : (std::string(c.expect) + ")").c_str());
    }
}

int main() {
    vn::Options telex;  // mặc định giống Unikey: Telex, chính tả + khôi phục bật

    run("Telex cơ bản", telex, {
        {"tieengs vieetj", "tiếng việt"},
        {"Vieetj Nam", "Việt Nam"},
        {"nguowif", "người"},
        {"dduwowngf", "đường"},
        {"DDaij hocj Caanf Thow", "Đại học Cần Thơ"},
        {"trungw taam", "trưng tâm"},     // w thêm móc cho u, giống Unikey
        {"chuyeenr ddooir soos", "chuyển đổi số"},
        {"hoaf binhf", "hòa bình"},
        {"khuyeens khichs", "khuyến khích"},
        {"giuwax", "giữa"},
        {"quas", "quá"}, {"quaanf", "quần"},
        {"gif", "gì"},
        {"thuowr", "thuở"},
        {"uw", "ư"},
        {"w", "ư"},
        {"aa ee oo dd", "â ê ô đ"},
        {"tooi yeeu", "tôi yêu"},
    });

    run("Telex: bỏ dấu tự do và sửa dấu", telex, {
        {"vietej", "việt"},          // gõ dấu mũ sau
        {"tiesnge", "tiếng"},        // dấu thanh và dấu mũ gõ sau cùng
        {"vieetsj", "việt"},         // đổi dấu thanh: s -> j
        {"vieetjf", "việtf"},        // "viềt" sai chính tả -> giữ phím f
        {"aas", "ấ"},
        {"ass", "as"},               // gõ lại phím dấu -> huỷ dấu
        {"ddd", "dd"},
        {"tiengs<BS>", "tién"}, {"vieet<BS><BS>", "vi"},
    });

    run("Telex: khôi phục từ tiếng Anh (chính tả bật)", telex, {
        {"windows ", "windows "},
        {"class", "class"},
        {"email", "email"},
        {"Linux", "Linux"},
    });

    vn::Options noW = telex;
    noW.standaloneW = false;
    run("Telex, tắt \"w đầu từ thành ư\"", noW, {
        {"w", "w"},
        {"wifi ", "wifi "},
        {"tuwf", "từ"},
        {"[", "ơ"},                  // Telex đầy đủ vẫn xử lý [ ]
    });

    vn::Options simple = telex;
    simple.inputMethod = InputMethod::SimpleTelex;
    run("Telex giản lược (UniKey 4.6)", simple, {
        {"w", "w"},
        {"wifi ", "wifi "},
        {"web ", "web "},
        {"tuwf", "từ"},
        {"nguowif", "người"},
        {"dduwowngf", "đường"},
        {"tieengs vieetj", "tiếng việt"},
        {"a[b]", "a[b]"},            // không xử lý [ ]
        {"[", "["},
    });

    vn::Options modern = telex;
    modern.modernStyle = true;
    run("Kiểu bỏ dấu mới (oà, uý)", modern, {
        {"hoaf", "hoà"},
        {"thuys", "thuý"},
        {"khoer", "khoẻ"},
    });

    vn::Options vni = telex;
    vni.inputMethod = InputMethod::Vni;
    run("VNI", vni, {
        {"tie61ng vie65t", "tiếng việt"},
        {"Vie65t Nam", "Việt Nam"},
        {"ngu7o72i", "người"},
        {"d9u7o7ng2", "đường"},
        {"Ca62n Tho7", "Cần Thơ"},
    });

    vn::Options viqr = telex;
    viqr.inputMethod = InputMethod::Viqr;
    run("VIQR", viqr, {
        {"tie^'ng vie^.t", "tiếng việt"},
        {"ddu+o+`ng", "đường"},
    });

    // Gõ tắt: dùng định dạng file gõ tắt của Unikey.
    const char* macroPath = "test-macro.txt";
    {
        std::ofstream f(macroPath);
        f << ";DO NOT DELETE THIS LINE***version=1***\n"
             "ctu:Trường Đại học Cần Thơ\n"
             "vn:Việt Nam\n";
    }
    vn::Options mac = telex;
    mac.macroEnabled = true;
    mac.macroFile = macroPath;
    run("Gõ tắt (macro)", mac, {
        {"ctu ", "Trường Đại học Cần Thơ "},
        {"Ctu ", "Trường Đại học Cần Thơ "},
        {"CTU ", "TRƯỜNG ĐẠI HỌC CẦN THƠ "},
        {"vn ", "Việt Nam "},
        {"vnn ", "vnn "},
        {"vn<S-SPC>", "vn "},        // Shift+Space: không bung gõ tắt
        {"vn<S-SPC>vn ", "vn Việt Nam "},
    });
    std::remove(macroPath);

    std::printf("\n%d/%d trường hợp đạt\n", g_total - g_fail, g_total);
    return g_fail == 0 ? 0 : 1;
}
