// SPDX-License-Identifier: GPL-3.0-or-later
// vilikey-convert: chuyển mã văn bản tiếng Việt (tương đương "Toolkit" của UniKey),
// dùng bộ chuyển mã VnConv có sẵn trong ukengine.
//
//   vilikey-convert -f tcvn3 -t unicode cu.txt moi.txt
//   vilikey-convert -f vni --from-text < van-ban-dan-tu-word.txt
//   vilikey-convert -f unicode --strip   <<< "Tiếng Việt"     -> Tieng Viet
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <glib.h>

#include "vnconv.h"

namespace {

struct Charset {
    const char* name;
    int id;
    const char* label;
};

// "unicode-decomposed" được xử lý riêng (VnConv dùng UCS-2 cho bảng mã này).
const int kDecomposedUtf8 = -100;

const Charset kCharsets[] = {
    {"unicode",            CONV_CHARSET_UNIUTF8,  "Unicode dựng sẵn (UTF-8)"},
    {"unicode-decomposed", kDecomposedUtf8,       "Unicode tổ hợp (UTF-8)"},
    {"tcvn3",              CONV_CHARSET_TCVN3,    "TCVN3 (ABC) — font .Vn..."},
    {"vni",                CONV_CHARSET_VNIWIN,   "VNI Windows — font VNI-..."},
    {"viscii",             CONV_CHARSET_VISCII,   "VISCII"},
    {"vps",                CONV_CHARSET_VPS,      "VPS"},
    {"viqr",               CONV_CHARSET_VIQR,     "VIQR"},
    {"cp1258",             CONV_CHARSET_WINCP1258,"Windows CP 1258"},
    {"ncr-dec",            CONV_CHARSET_UNIREF,   "NCR thập phân (&#7871;)"},
    {"ncr-hex",            CONV_CHARSET_UNIREF_HEX,"NCR thập lục phân (&#x1EBF;)"},
    {"bkhcm2",             CONV_CHARSET_BKHCM2,   "BK HCM 2"},
    {"vietware-x",         CONV_CHARSET_VIETWAREX,"Vietware X"},
};

const Charset* findCharset(const std::string& name) {
    for (const auto& c : kCharsets)
        if (name == c.name) return &c;
    return nullptr;
}

bool isLegacy(int id) {
    return id == CONV_CHARSET_TCVN3 || id == CONV_CHARSET_VNIWIN || id == CONV_CHARSET_VISCII ||
           id == CONV_CHARSET_VPS || id == CONV_CHARSET_WINCP1258 || id == CONV_CHARSET_BKHCM2 ||
           id == CONV_CHARSET_VIETWAREX;
}

// Windows-1252: các ký tự 0x80..0x9F. Văn bản mã cũ (TCVN3, VNI...) khi mở trong
// Word/LibreOffice hoặc dán qua clipboard được biểu diễn bằng các ký tự này.
const uint16_t kCp1252High[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
};

// Văn bản (UTF-8) -> chuỗi byte theo Windows-1252.
bool textToBytes(const std::string& in, std::string& out, std::string& err) {
    const char* p = in.c_str();
    const char* end = p + in.size();
    if (!g_utf8_validate(p, static_cast<gssize>(in.size()), nullptr)) {
        err = "dữ liệu vào không phải UTF-8 hợp lệ";
        return false;
    }
    while (p < end) {
        gunichar c = g_utf8_get_char(p);
        p = g_utf8_next_char(p);
        if (c < 0x80 || (c >= 0xA0 && c <= 0xFF)) {
            out.push_back(static_cast<char>(c));
            continue;
        }
        bool found = false;
        for (int i = 0; i < 32; ++i) {
            if (kCp1252High[i] == c) {
                out.push_back(static_cast<char>(0x80 + i));
                found = true;
                break;
            }
        }
        if (!found) {
            err = "văn bản có ký tự Unicode (vd: chữ có dấu thật), không phải mã cũ";
            return false;
        }
    }
    return true;
}

// Chuỗi byte Windows-1252 -> văn bản UTF-8.
std::string bytesToText(const std::string& in) {
    std::string out;
    char buf[8];
    for (unsigned char b : in) {
        gunichar c = (b >= 0x80 && b < 0xA0) ? kCp1252High[b - 0x80] : b;
        int n = g_unichar_to_utf8(c, buf);
        out.append(buf, static_cast<size_t>(n));
    }
    return out;
}

std::string utf8ToUcs2(const std::string& s) {
    std::string out;
    glong n = 0;
    gunichar2* w = g_utf8_to_utf16(s.c_str(), static_cast<glong>(s.size()), nullptr, &n, nullptr);
    if (w) out.assign(reinterpret_cast<const char*>(w), static_cast<size_t>(n) * 2);
    g_free(w);
    return out;
}

std::string ucs2ToUtf8(const std::string& s) {
    std::string out;
    gchar* u = g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(s.data()),
                               static_cast<glong>(s.size() / 2), nullptr, nullptr, nullptr);
    if (u) out = u;
    g_free(u);
    return out;
}

bool convert(int from, int to, const std::string& in, std::string& out, std::string& err) {
    std::string src = in;
    int realFrom = from, realTo = to;
    if (from == kDecomposedUtf8) { src = utf8ToUcs2(in); realFrom = CONV_CHARSET_UNIDECOMPOSED; }
    if (to == kDecomposedUtf8) realTo = CONV_CHARSET_UNIDECOMPOSED;

    std::vector<unsigned char> buf(src.size() * 8 + 64);
    int inLen = static_cast<int>(src.size());
    int outLen = static_cast<int>(buf.size());
    int ret = VnConvert(realFrom, realTo,
                        reinterpret_cast<UKBYTE*>(const_cast<char*>(src.data())),
                        buf.data(), &inLen, &outLen);
    if (ret != 0) {
        err = VnConvErrMsg(ret);
        return false;
    }
    out.assign(reinterpret_cast<const char*>(buf.data()), static_cast<size_t>(outLen));
    if (to == kDecomposedUtf8) out = ucs2ToUtf8(out);
    return true;
}

void usage() {
    std::cerr <<
"Dùng: vilikey-convert [-f MÃ_NGUỒN] [-t MÃ_ĐÍCH] [tuỳ chọn] [FILE_VÀO [FILE_RA]]\n"
"Chuyển mã văn bản tiếng Việt. Mặc định: -f tcvn3 -t unicode, đọc stdin, ghi stdout.\n\n"
"Tuỳ chọn:\n"
"  --upper        sang chữ HOA\n"
"  --lower        sang chữ thường\n"
"  --strip        bỏ dấu tiếng Việt\n"
"  --from-text    dữ liệu vào là văn bản UTF-8 chứa mã cũ (dán từ Word/LibreOffice,\n"
"                 hiển thị dạng \"Tr\xC2\xADêng\"), không phải file byte gốc\n"
"  --to-text      khi đích là mã cũ: xuất dưới dạng văn bản UTF-8 để dán vào ứng dụng\n"
"  --list         liệt kê các bảng mã\n\n"
"Bảng mã:";
    for (const auto& c : kCharsets) std::cerr << " " << c.name;
    std::cerr << "\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string from = "tcvn3", to = "unicode", inFile, outFile;
    bool fromText = false, toText = false;
    VnConvOptions opt;
    VnConvResetOptions(&opt);

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-f" || a == "--from") && i + 1 < argc) from = argv[++i];
        else if ((a == "-t" || a == "--to") && i + 1 < argc) to = argv[++i];
        else if (a == "--upper") opt.toUpper = 1;
        else if (a == "--lower") opt.toLower = 1;
        else if (a == "--strip") opt.removeTone = 1;
        else if (a == "--from-text") fromText = true;
        else if (a == "--to-text") toText = true;
        else if (a == "--list") {
            for (const auto& c : kCharsets) std::printf("%-20s %s\n", c.name, c.label);
            return 0;
        }
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (!a.empty() && a[0] == '-' && a != "-") { usage(); return 2; }
        else if (inFile.empty()) inFile = a;
        else if (outFile.empty()) outFile = a;
        else { usage(); return 2; }
    }

    const Charset* cf = findCharset(from);
    const Charset* ct = findCharset(to);
    if (!cf || !ct) {
        std::cerr << "Bảng mã không hợp lệ. Dùng --list để xem danh sách.\n";
        return 2;
    }
    VnConvSetOptions(&opt);

    std::string data;
    if (inFile.empty() || inFile == "-") {
        data.assign(std::istreambuf_iterator<char>(std::cin), {});
    } else {
        std::ifstream f(inFile, std::ios::binary);
        if (!f) { std::cerr << "Không mở được " << inFile << "\n"; return 1; }
        data.assign(std::istreambuf_iterator<char>(f), {});
    }

    std::string err, out;
    if (fromText && isLegacy(cf->id)) {
        std::string bytes;
        if (!textToBytes(data, bytes, err)) { std::cerr << "Lỗi: " << err << "\n"; return 1; }
        data.swap(bytes);
    }
    if (!convert(cf->id, ct->id, data, out, err)) {
        std::cerr << "Lỗi chuyển mã: " << err << "\n";
        return 1;
    }
    if (toText && isLegacy(ct->id))
        out = bytesToText(out);

    if (outFile.empty() || outFile == "-") {
        std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
    } else {
        std::ofstream f(outFile, std::ios::binary);
        if (!f) { std::cerr << "Không ghi được " << outFile << "\n"; return 1; }
        f.write(out.data(), static_cast<std::streamsize>(out.size()));
    }
    return 0;
}
