#include "editor.h"

#include <engine/engine.h>
#include <engine/events.h>
#include <engine/rendering/renderer.h>

#include <rttr/registration>

#include "assets/asset_watcher.h"
#include "editing/editing_manager.h"
#include "editing/picking_manager.h"
#include "editing/thumbnail_manager.h"
#include "events.h"
#include "hub/hub.h"
#include "imgui/imgui_interface.h"
#include "system/project_manager.h"

namespace ace
{
RTTR_REGISTRATION
{
    rttr::registration::class_<editor>("editor")
        .constructor<>()
        .method("create", &editor::create)
        .method("init", &editor::init)
        .method("deinit", &editor::deinit)
        .method("destroy", &editor::destroy)
        .method("process", &editor::process)
        .method("interrupt", &editor::interrupt);

}

auto editor::create(rtti::context& ctx, cmd_line::parser& parser) -> bool
{
    if(!engine::create(ctx, parser))
    {
        return false;
    }

    fs::path binary_path = fs::resolve_protocol("binary:/");
    fs::path editor_data = binary_path / "data" / "editor";
    fs::add_path_protocol("editor", editor_data);

    ctx.add<ui_events>();
    ctx.add<project_manager>(ctx);
    ctx.add<imgui_interface>(ctx);
    ctx.add<hub>(ctx);
    ctx.add<editing_manager>();
    ctx.add<picking_manager>();
    ctx.add<thumbnail_manager>();
    ctx.add<asset_watcher>();

    return true;
}

auto editor::init(const cmd_line::parser& parser) -> bool
{
    if(!engine::init_core(parser))
    {
        return false;
    }

    auto& ctx = engine::context();

    if(!init_window(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<asset_watcher>().init(ctx))
    {
        return false;
    }

    if(!engine::init_systems(parser))
    {
        return false;
    }

    if(!ctx.get_cached<project_manager>().init(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<imgui_interface>().init(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<hub>().init(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<editing_manager>().init(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<picking_manager>().init(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<thumbnail_manager>().init(ctx))
    {
        return false;
    }

    return true;
}

auto editor::init_window(rtti::context& ctx) -> bool
{
    auto title = fmt::format("Ace Editor <{}>", gfx::get_renderer_name(gfx::get_renderer_type()));
    uint32_t flags = os::window::resizable | os::window::maximized;
    auto primary_display = os::display::get_primary_display_index();

    auto& rend = ctx.get_cached<renderer>();
    rend.create_window_for_display(primary_display, title, flags);
    return true;
}

auto editor::deinit() -> bool
{
    auto& ctx = engine::context();

    if(!ctx.get_cached<asset_watcher>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<thumbnail_manager>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<picking_manager>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<editing_manager>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<hub>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<imgui_interface>().deinit(ctx))
    {
        return false;
    }

    if(!ctx.get_cached<project_manager>().deinit(ctx))
    {
        return false;
    }

    return engine::deinit();
}

auto editor::destroy() -> bool
{
    auto& ctx = engine::context();

    ctx.remove<asset_watcher>();
    ctx.remove<thumbnail_manager>();
    ctx.remove<picking_manager>();
    ctx.remove<editing_manager>();

    ctx.remove<hub>();
    ctx.remove<imgui_interface>();

    ctx.remove<project_manager>();

    ctx.remove<ui_events>();

    return engine::destroy();
}

auto editor::process() -> bool
{
    return engine::process();
}
auto editor::interrupt() -> bool
{
    return engine::interrupt();
}

} // namespace ace
