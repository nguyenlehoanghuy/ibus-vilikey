// SPDX-License-Identifier: GPL-3.0-or-later
// IBus engine: nhận phím từ IBus, chuyển cho ukengine, đưa kết quả về ứng dụng.
//
// Bộ gõ này chỉ làm một việc: gõ tiếng Việt. Muốn gõ tiếng Anh thì chuyển nguồn
// nhập của hệ thống (Super+Space: "en" <-> "vi"), không có chế độ V/E riêng.
//
// Hai chế độ hiển thị:
//   * Preedit: từ đang gõ nằm trong vùng preedit, commit khi gặp ký tự ngắt từ.
//              Ổn định trên mọi ứng dụng (terminal, trình duyệt, Electron...).
//   * Direct : sửa thẳng văn bản trong ứng dụng (xoá bằng delete_surrounding_text
//              rồi commit), giống cảm giác UniKey trên Windows. Tự động quay về
//              Preedit với ứng dụng không hỗ trợ surrounding text.
#include "engine.h"

#include <fstream>
#include <string>

#include "core.h"
#include "settings.h"

using vn::Core;
using vn::Edit;
using vn::InputMethod;
using vn::OutputMode;
using vn::Settings;

struct _VnEngine {
    IBusEngine parent;
    IBusPropList* props;
    bool directActive;       // từ hiện tại đang được gõ theo chế độ Direct
};

G_DEFINE_TYPE(VnEngine, vn_engine, IBUS_TYPE_ENGINE)

// ---------------------------------------------------------------------------
// Trạng thái dùng chung trong tiến trình
// ---------------------------------------------------------------------------
static Settings* g_settings = nullptr;
static Core* g_core = nullptr;

void vn_engine_global_init(void) {
    g_settings = new Settings();
    g_settings->load();
    g_core = new Core();
    g_core->apply(g_settings->core);
}

void vn_engine_global_cleanup(void) {
    delete g_core;
    delete g_settings;
    g_core = nullptr;
    g_settings = nullptr;
}

// ---------------------------------------------------------------------------
// Tiện ích
// ---------------------------------------------------------------------------
static IBusText* make_text(const std::u32string& s) {
    std::string u = Core::toUtf8(s);
    return ibus_text_new_from_string(u.c_str());
}

static bool use_direct(VnEngine* self) {
    IBusEngine* e = IBUS_ENGINE(self);
    return g_settings->mode == OutputMode::Direct &&
           (e->client_capabilities & IBUS_CAP_SURROUNDING_TEXT);
}

static void update_preedit(VnEngine* self) {
    IBusEngine* engine = IBUS_ENGINE(self);
    const std::u32string& s = g_core->text();
    if (s.empty()) {
        ibus_engine_hide_preedit_text(engine);
        return;
    }
    IBusText* text = make_text(s);
    if (g_settings->underline)
        ibus_text_append_attribute(text, IBUS_ATTR_TYPE_UNDERLINE,
                                   IBUS_ATTR_UNDERLINE_SINGLE, 0, -1);
    ibus_engine_update_preedit_text_with_mode(engine, text, ibus_text_get_length(text),
                                              TRUE, IBUS_ENGINE_PREEDIT_COMMIT);
}

// Đưa toàn bộ preedit vào ứng dụng và xoá bộ đệm.
static void commit_all(VnEngine* self) {
    IBusEngine* engine = IBUS_ENGINE(self);
    if (!self->directActive && !g_core->text().empty()) {
        ibus_engine_hide_preedit_text(engine);
        ibus_engine_commit_text(engine, make_text(g_core->text()));
    }
    g_core->reset();
}

// Xoá trạng thái mà không commit (IBus tự commit preedit khi cần
// vì ta dùng IBUS_ENGINE_PREEDIT_COMMIT).
static void reset_state(VnEngine* self) {
    if (!self->directActive && !g_core->text().empty())
        ibus_engine_hide_preedit_text(IBUS_ENGINE(self));
    g_core->reset();
}

// Áp dụng một thay đổi ở chế độ Direct. Trả về FALSE nếu nên để ứng dụng tự
// xử lý phím (khi ukengine không thay đổi gì ngoài chính ký tự vừa gõ).
static gboolean apply_direct(VnEngine* self, const std::u32string& before,
                             const Edit& ed, uint32_t typed) {
    size_t del;
    std::u32string ins;
    if (static_cast<size_t>(ed.erase) <= before.size()) {
        std::u32string after = before.substr(0, before.size() - ed.erase) + ed.insert;
        size_t p = 0;
        while (p < before.size() && p < after.size() && before[p] == after[p]) ++p;
        del = before.size() - p;
        ins = after.substr(p);
    } else {
        del = static_cast<size_t>(ed.erase);
        ins = ed.insert;
    }

    if (del == 0 && typed && ins.size() == 1 && ins[0] == typed)
        return FALSE;  // phím thường: để ứng dụng tự chèn

    IBusEngine* engine = IBUS_ENGINE(self);
    if (del > 0)
        ibus_engine_delete_surrounding_text(engine, -static_cast<gint>(del),
                                            static_cast<guint>(del));
    if (!ins.empty())
        ibus_engine_commit_text(engine, make_text(ins));
    return TRUE;
}

// ---------------------------------------------------------------------------
// Menu trên biểu tượng "vi" của thanh trạng thái
// ---------------------------------------------------------------------------
static IBusProperty* make_prop(const char* key, IBusPropType type, const char* label,
                               bool checked = false, IBusPropList* sub = nullptr) {
    return ibus_property_new(key, type, ibus_text_new_from_string(label), nullptr,
                             ibus_text_new_from_string(label), TRUE, TRUE,
                             checked ? PROP_STATE_CHECKED : PROP_STATE_UNCHECKED, sub);
}

static const char* im_label(InputMethod im) {
    switch (im) {
    case InputMethod::SimpleTelex: return "Telex giản lược";
    case InputMethod::Vni:         return "VNI";
    case InputMethod::Viqr:        return "VIQR";
    default:                       return "Telex";
    }
}

static void build_props(VnEngine* self) {
    if (self->props)
        g_object_unref(self->props);
    self->props = ibus_prop_list_new();
    g_object_ref_sink(self->props);

    const Settings& s = *g_settings;
    const vn::Options& o = s.core;

    // Kiểu gõ
    IBusPropList* im = ibus_prop_list_new();
    ibus_prop_list_append(im, make_prop("im.telex", PROP_TYPE_RADIO, "Telex",
                                        o.inputMethod == InputMethod::Telex));
    ibus_prop_list_append(im, make_prop("im.simple-telex", PROP_TYPE_RADIO,
                                        "Telex giản lược (w, [ ] giữ nguyên)",
                                        o.inputMethod == InputMethod::SimpleTelex));
    ibus_prop_list_append(im, make_prop("im.vni", PROP_TYPE_RADIO, "VNI",
                                        o.inputMethod == InputMethod::Vni));
    ibus_prop_list_append(im, make_prop("im.viqr", PROP_TYPE_RADIO, "VIQR",
                                        o.inputMethod == InputMethod::Viqr));
    std::string imTitle = std::string("Kiểu gõ: ") + im_label(o.inputMethod);
    ibus_prop_list_append(self->props,
                          make_prop("im", PROP_TYPE_MENU, imTitle.c_str(), false, im));

    // Chế độ hiển thị
    IBusPropList* md = ibus_prop_list_new();
    ibus_prop_list_append(md, make_prop("mode.preedit", PROP_TYPE_RADIO,
                                        "Preedit (ổn định, khuyên dùng)",
                                        s.mode == OutputMode::Preedit));
    ibus_prop_list_append(md, make_prop("mode.direct", PROP_TYPE_RADIO,
                                        "Gõ trực tiếp (giống UniKey Windows)",
                                        s.mode == OutputMode::Direct));
    ibus_prop_list_append(self->props,
                          make_prop("mode", PROP_TYPE_MENU,
                                    s.mode == OutputMode::Direct ? "Chế độ: Gõ trực tiếp"
                                                                 : "Chế độ: Preedit",
                                    false, md));

    // Tuỳ chọn
    IBusPropList* op = ibus_prop_list_new();
    ibus_prop_list_append(op, make_prop("opt.spell-check", PROP_TYPE_TOGGLE,
                                        "Kiểm tra chính tả", o.spellCheck));
    ibus_prop_list_append(op, make_prop("opt.auto-restore", PROP_TYPE_TOGGLE,
                                        "Tự khôi phục phím với từ sai", o.autoRestore));
    ibus_prop_list_append(op, make_prop("opt.free-marking", PROP_TYPE_TOGGLE,
                                        "Cho phép gõ dấu tự do", o.freeMarking));
    ibus_prop_list_append(op, make_prop("opt.modern-style", PROP_TYPE_TOGGLE,
                                        "Đặt dấu oà, uý (thay vì òa, úy)", o.modernStyle));
    ibus_prop_list_append(op, make_prop("opt.standalone-w", PROP_TYPE_TOGGLE,
                                        "Telex: w đầu từ thành ư", o.standaloneW));
    ibus_prop_list_append(op, make_prop("opt.underline", PROP_TYPE_TOGGLE,
                                        "Gạch chân từ đang gõ (Preedit)", s.underline));
    ibus_prop_list_append(self->props, make_prop("opt", PROP_TYPE_MENU, "Tuỳ chọn", false, op));

    // Gõ tắt
    IBusPropList* mc = ibus_prop_list_new();
    ibus_prop_list_append(mc, make_prop("macro.enable", PROP_TYPE_TOGGLE,
                                        "Bật gõ tắt (Shift+Space để bỏ qua)",
                                        o.macroEnabled));
    ibus_prop_list_append(mc, make_prop("macro.edit", PROP_TYPE_NORMAL, "Sửa bảng gõ tắt…"));
    ibus_prop_list_append(mc, make_prop("macro.reload", PROP_TYPE_NORMAL,
                                        "Nạp lại bảng gõ tắt"));
    ibus_prop_list_append(self->props, make_prop("macro", PROP_TYPE_MENU, "Gõ tắt", false, mc));

    // Công cụ chuyển mã
    ibus_prop_list_append(self->props,
                          make_prop("toolkit", PROP_TYPE_NORMAL, "Công cụ chuyển mã…"));
}

static void refresh_props(VnEngine* self) {
    build_props(self);
    ibus_engine_register_properties(IBUS_ENGINE(self), self->props);
}

static void ensure_macro_file() {
    std::string path = Settings::macroFile();
    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
        return;
    g_mkdir_with_parents(Settings::configDir().c_str(), 0700);
    gchar* tmpl = nullptr;
    if (g_file_get_contents(VN_DATADIR "/macro.txt", &tmpl, nullptr, nullptr)) {
        g_file_set_contents(path.c_str(), tmpl, -1, nullptr);
        g_free(tmpl);
    } else {
        std::ofstream(path) << ";DO NOT DELETE THIS LINE***version=1***\n";
    }
}

static void spawn(const char* prog, const char* arg) {
    const gchar* argv[] = {prog, arg, nullptr};
    GError* err = nullptr;
    if (!g_spawn_async(nullptr, const_cast<gchar**>(argv), nullptr, G_SPAWN_SEARCH_PATH,
                       nullptr, nullptr, nullptr, &err)) {
        g_warning("Không chạy được %s: %s", prog, err->message);
        g_error_free(err);
    }
}

static void apply_settings(VnEngine* self) {
    reset_state(self);
    g_core->apply(g_settings->core);
    g_settings->save();
    refresh_props(self);
}

static void vn_engine_property_activate(IBusEngine* engine, const gchar* name, guint state) {
    VnEngine* self = VN_ENGINE(engine);
    Settings& s = *g_settings;
    std::string key = name;
    bool on = state == PROP_STATE_CHECKED;

    if (key.rfind("im.", 0) == 0) {
        if (!on) return;
        if (key == "im.vni") s.core.inputMethod = InputMethod::Vni;
        else if (key == "im.viqr") s.core.inputMethod = InputMethod::Viqr;
        else if (key == "im.simple-telex") s.core.inputMethod = InputMethod::SimpleTelex;
        else s.core.inputMethod = InputMethod::Telex;
    } else if (key == "mode.preedit" || key == "mode.direct") {
        if (!on) return;
        commit_all(self);
        s.mode = key == "mode.direct" ? OutputMode::Direct : OutputMode::Preedit;
    }
    else if (key == "opt.spell-check")  s.core.spellCheck = on;
    else if (key == "opt.auto-restore") s.core.autoRestore = on;
    else if (key == "opt.free-marking") s.core.freeMarking = on;
    else if (key == "opt.modern-style") s.core.modernStyle = on;
    else if (key == "opt.standalone-w") s.core.standaloneW = on;
    else if (key == "opt.underline")    s.underline = on;
    else if (key == "macro.enable") {
        s.core.macroEnabled = on;
        if (on) ensure_macro_file();
    } else if (key == "macro.edit") {
        ensure_macro_file();
        gchar* uri = g_filename_to_uri(Settings::macroFile().c_str(), nullptr, nullptr);
        if (uri) {
            spawn("xdg-open", uri);
            g_free(uri);
        }
        return;
    } else if (key == "macro.reload") {
        // apply_settings() bên dưới sẽ nạp lại file gõ tắt
    } else if (key == "toolkit") {
        commit_all(self);
        spawn(VN_BINDIR "/vilikey-toolkit", nullptr);
        return;
    } else {
        return;
    }
    apply_settings(self);
}

// ---------------------------------------------------------------------------
// Xử lý phím
// ---------------------------------------------------------------------------
static bool is_modifier_key(guint k) {
    return (k >= IBUS_KEY_Shift_L && k <= IBUS_KEY_Hyper_R) ||
           k == IBUS_KEY_ISO_Level3_Shift || k == IBUS_KEY_ISO_Level5_Shift ||
           k == IBUS_KEY_Mode_switch || k == IBUS_KEY_Num_Lock;
}

static gboolean process_backspace(VnEngine* self) {
    if (g_core->text().empty()) {
        g_core->reset();
        return FALSE;
    }
    if (self->directActive) {
        std::u32string before = g_core->text();
        Edit ed = g_core->backspace();
        if (ed.erase == 1 && ed.insert.empty())
            return FALSE;   // xoá 1 ký tự bình thường: để ứng dụng làm
        return apply_direct(self, before, ed, 0);
    }
    g_core->backspace();
    if (g_core->text().empty())
        g_core->reset();
    update_preedit(self);
    return TRUE;
}

static gboolean process_char(VnEngine* self, guint keyval, guint modifiers) {
    bool shift = modifiers & IBUS_SHIFT_MASK;
    bool caps  = modifiers & IBUS_LOCK_MASK;

    // Chọn chế độ cho từ mới; nếu khả năng của ứng dụng đổi giữa chừng thì chốt từ cũ.
    bool direct = use_direct(self);
    if (direct != self->directActive) {
        commit_all(self);
        self->directActive = direct;
    }

    std::u32string before = g_core->text();
    Edit ed = g_core->key(keyval, shift, caps);

    if (direct)
        return apply_direct(self, before, ed, keyval);

    // Preedit: chốt từ khi gặp ký tự ngắt từ mà ukengine giữ nguyên.
    const std::u32string& t = g_core->text();
    if (Core::isWordBreak(keyval) && !t.empty() && t.back() == static_cast<char32_t>(keyval)) {
        commit_all(self);
        return TRUE;
    }
    update_preedit(self);
    return TRUE;
}

static gboolean vn_engine_process_key_event(IBusEngine* engine, guint keyval,
                                            guint keycode, guint modifiers) {
    (void)keycode;
    VnEngine* self = VN_ENGINE(engine);

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;
    if (is_modifier_key(keyval))
        return FALSE;

    // Phím tắt của ứng dụng (Ctrl+C, Alt+F, Super+...): chốt từ rồi cho qua.
    if (modifiers & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_SUPER_MASK |
                     IBUS_HYPER_MASK | IBUS_META_MASK)) {
        commit_all(self);
        return FALSE;
    }

    if (keyval == IBUS_KEY_BackSpace)
        return process_backspace(self);

    if (keyval >= IBUS_KEY_space && keyval <= IBUS_KEY_asciitilde)
        return process_char(self, keyval, modifiers);

    // Enter, Tab, mũi tên, Delete, phím số bên phải, F1..F12, ký tự ngoài ASCII...
    commit_all(self);
    return FALSE;
}

// ---------------------------------------------------------------------------
// Vòng đời
// ---------------------------------------------------------------------------
static void vn_engine_focus_in(IBusEngine* engine) {
    VnEngine* self = VN_ENGINE(engine);
    g_core->reset();
    self->directActive = use_direct(self);
    refresh_props(self);
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->focus_in(engine);
}

static void vn_engine_focus_out(IBusEngine* engine) {
    reset_state(VN_ENGINE(engine));
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->focus_out(engine);
}

static void vn_engine_reset(IBusEngine* engine) {
    reset_state(VN_ENGINE(engine));
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->reset(engine);
}

static void vn_engine_enable(IBusEngine* engine) {
    // Yêu cầu ứng dụng gửi surrounding text (cần cho chế độ Gõ trực tiếp).
    ibus_engine_get_surrounding_text(engine, nullptr, nullptr, nullptr);
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->enable(engine);
}

static void vn_engine_disable(IBusEngine* engine) {
    reset_state(VN_ENGINE(engine));
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->disable(engine);
}

static void vn_engine_set_capabilities(IBusEngine* engine, guint caps) {
    VnEngine* self = VN_ENGINE(engine);
    reset_state(self);
    IBUS_ENGINE_CLASS(vn_engine_parent_class)->set_capabilities(engine, caps);
    self->directActive = use_direct(self);
}

static void vn_engine_destroy(IBusObject* object) {
    VnEngine* self = VN_ENGINE(object);
    g_clear_object(&self->props);
    IBUS_OBJECT_CLASS(vn_engine_parent_class)->destroy(object);
}

static void vn_engine_init(VnEngine* self) {
    self->props = nullptr;
    self->directActive = false;
    build_props(self);
}

static void vn_engine_class_init(VnEngineClass* klass) {
    IBusObjectClass* obj = IBUS_OBJECT_CLASS(klass);
    IBusEngineClass* eng = IBUS_ENGINE_CLASS(klass);

    obj->destroy = vn_engine_destroy;
    eng->process_key_event = vn_engine_process_key_event;
    eng->focus_in          = vn_engine_focus_in;
    eng->focus_out         = vn_engine_focus_out;
    eng->reset             = vn_engine_reset;
    eng->enable            = vn_engine_enable;
    eng->disable           = vn_engine_disable;
    eng->set_capabilities  = vn_engine_set_capabilities;
    eng->property_activate = vn_engine_property_activate;
}
