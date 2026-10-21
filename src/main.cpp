// SPDX-License-Identifier: GPL-3.0-or-later
#include <clocale>
#include <cstdio>

#include <ibus.h>

#include "engine.h"

static gboolean opt_ibus = FALSE;
static gboolean opt_version = FALSE;

static const GOptionEntry kEntries[] = {
    {"ibus", 'i', 0, G_OPTION_ARG_NONE, &opt_ibus, "Được khởi chạy bởi ibus-daemon", nullptr},
    {"version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, "In phiên bản", nullptr},
    {nullptr, 0, 0, G_OPTION_ARG_NONE, nullptr, nullptr, nullptr},
};

static void on_disconnected(IBusBus*, gpointer) {
    ibus_quit();
}

static IBusComponent* make_component() {
    IBusComponent* component = ibus_component_new(
        "org.freedesktop.IBus.ViliKey", "ViliKey — bộ gõ tiếng Việt (engine UniKey)", VN_VERSION,
        "GPL-3.0-or-later", "ibus-vilikey", "", "", "ibus-vilikey");
    IBusEngineDesc* desc = ibus_engine_desc_new_varargs(
        "name", "vilikey",
        "longname", "ViliKey",
        "description", "ViliKey — bộ gõ tiếng Việt dùng engine gốc của UniKey",
        "language", "vi",
        "license", "GPL-3.0-or-later",
        "author", "ViliKey contributors",
        "icon", VN_DATADIR "/icons/ibus-vilikey.svg",
        "layout", "default",
        "symbol", "vi",
        "rank", 99,
        nullptr);
    ibus_component_add_engine(component, desc);
    return component;
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");

    GError* error = nullptr;
    GOptionContext* ctx = g_option_context_new("- bộ gõ tiếng Việt cho IBus");
    g_option_context_add_main_entries(ctx, kEntries, nullptr);
    if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
        std::fprintf(stderr, "%s\n", error->message);
        return 1;
    }
    g_option_context_free(ctx);

    if (opt_version) {
        std::printf("ibus-vilikey %s\n", VN_VERSION);
        return 0;
    }

    ibus_init();
    IBusBus* bus = ibus_bus_new();
    if (!ibus_bus_is_connected(bus)) {
        std::fprintf(stderr, "Không kết nối được tới ibus-daemon.\n");
        return 1;
    }
    g_signal_connect(bus, "disconnected", G_CALLBACK(on_disconnected), nullptr);

    vn_engine_global_init();

    IBusFactory* factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "vilikey", VN_TYPE_ENGINE);

    if (opt_ibus) {
        ibus_bus_request_name(bus, "org.freedesktop.IBus.ViliKey", 0);
    } else {
        // Chạy tay (để gỡ lỗi): tự đăng ký component với ibus-daemon.
        IBusComponent* component = make_component();
        ibus_bus_register_component(bus, component);
        g_object_unref(component);
    }

    ibus_main();

    vn_engine_global_cleanup();
    g_object_unref(factory);
    g_object_unref(bus);
    return 0;
}
