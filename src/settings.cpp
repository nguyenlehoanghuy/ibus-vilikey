// SPDX-License-Identifier: GPL-3.0-or-later
#include "settings.h"

#include <glib.h>
#include <glib/gstdio.h>

namespace vn {

namespace {

const char* kGroup = "vilikey";

const char* imName(InputMethod im) {
    switch (im) {
    case InputMethod::Vni:         return "vni";
    case InputMethod::Viqr:        return "viqr";
    case InputMethod::SimpleTelex: return "simple-telex";
    default:                       return "telex";
    }
}

InputMethod imFromName(const std::string& s) {
    if (s == "vni")          return InputMethod::Vni;
    if (s == "viqr")         return InputMethod::Viqr;
    if (s == "simple-telex") return InputMethod::SimpleTelex;
    return InputMethod::Telex;
}

bool getBool(GKeyFile* kf, const char* key, bool def) {
    GError* err = nullptr;
    gboolean v = g_key_file_get_boolean(kf, kGroup, key, &err);
    if (err) { g_error_free(err); return def; }
    return v;
}

std::string getStr(GKeyFile* kf, const char* key, const std::string& def) {
    gchar* v = g_key_file_get_string(kf, kGroup, key, nullptr);
    if (!v) return def;
    std::string s = v;
    g_free(v);
    return s;
}

} // namespace

std::string Settings::configDir() {
    gchar* p = g_build_filename(g_get_user_config_dir(), "ibus-vilikey", nullptr);
    std::string s = p;
    g_free(p);
    return s;
}

std::string Settings::configFile() { return configDir() + "/config.ini"; }
std::string Settings::macroFile()  { return configDir() + "/macro.txt"; }

void Settings::load() {
    core.macroFile = macroFile();

    GKeyFile* kf = g_key_file_new();
    if (g_key_file_load_from_file(kf, configFile().c_str(), G_KEY_FILE_NONE, nullptr)) {
        core.inputMethod  = imFromName(getStr(kf, "input-method", "telex"));
        core.spellCheck   = getBool(kf, "spell-check", core.spellCheck);
        core.autoRestore  = getBool(kf, "auto-restore", core.autoRestore);
        core.modernStyle  = getBool(kf, "modern-style", core.modernStyle);
        core.freeMarking  = getBool(kf, "free-marking", core.freeMarking);
        core.standaloneW  = getBool(kf, "standalone-w", core.standaloneW);
        core.macroEnabled = getBool(kf, "macro", core.macroEnabled);
        mode      = getStr(kf, "mode", "preedit") == "direct" ? OutputMode::Direct
                                                             : OutputMode::Preedit;
        underline = getBool(kf, "underline", underline);
    }
    g_key_file_free(kf);
}

void Settings::save() const {
    g_mkdir_with_parents(configDir().c_str(), 0700);

    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string (kf, kGroup, "input-method", imName(core.inputMethod));
    g_key_file_set_boolean(kf, kGroup, "spell-check",  core.spellCheck);
    g_key_file_set_boolean(kf, kGroup, "auto-restore", core.autoRestore);
    g_key_file_set_boolean(kf, kGroup, "modern-style", core.modernStyle);
    g_key_file_set_boolean(kf, kGroup, "free-marking", core.freeMarking);
    g_key_file_set_boolean(kf, kGroup, "standalone-w", core.standaloneW);
    g_key_file_set_boolean(kf, kGroup, "macro",        core.macroEnabled);
    g_key_file_set_string (kf, kGroup, "mode", mode == OutputMode::Direct ? "direct" : "preedit");
    g_key_file_set_boolean(kf, kGroup, "underline",    underline);
    g_key_file_save_to_file(kf, configFile().c_str(), nullptr);
    g_key_file_free(kf);
}

} // namespace vn
