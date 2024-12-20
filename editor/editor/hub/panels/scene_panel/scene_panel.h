#pragma once
#include <editor/imgui/integration/imgui.h>
#include <math/math.h>
#include "../entity_panel.h"

#include "gizmos/gizmos_renderer.h"

namespace ace
{
class scene_panel : public entity_panel

{
public:
    scene_panel(imgui_panels* parent);

    void init(rtti::context& ctx);
    void deinit(rtti::context& ctx);

    void on_frame_update(rtti::context& ctx, delta_t dt);
    void on_frame_render(rtti::context& ctx, delta_t dt);
    void on_frame_ui_render(rtti::context& ctx, const char* name);

    auto get_camera() -> entt::handle;

    void set_visible(bool visible);
    auto is_focused() const -> bool;

private:
    void draw_scene(rtti::context& ctx, delta_t dt);

    void draw_ui(rtti::context& ctx);
    void draw_menubar(rtti::context& ctx);

    bool is_visible_{};
    bool is_focused_{};
    bool is_dragging_{};
    int visualize_passes_{-1};
    scene panel_scene_;
    entt::handle panel_camera_{};

    float acceleration_{};
    math::vec3 move_dir_{};

    gizmos_renderer gizmos_{};
};
} // namespace ace
