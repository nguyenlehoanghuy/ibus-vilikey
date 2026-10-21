// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <ibus.h>

G_BEGIN_DECLS

typedef struct _VnEngine VnEngine;
typedef struct _VnEngineClass { IBusEngineClass parent_class; } VnEngineClass;

GType vn_engine_get_type(void);
#define VN_TYPE_ENGINE (vn_engine_get_type())
#define VN_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), VN_TYPE_ENGINE, VnEngine))

// Khởi tạo/giải phóng trạng thái dùng chung (cấu hình, ukengine).
void vn_engine_global_init(void);
void vn_engine_global_cleanup(void);

G_END_DECLS
