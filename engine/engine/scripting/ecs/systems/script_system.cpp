#include "script_system.h"
#include <engine/ecs/ecs.h>
#include <engine/engine.h>
#include <engine/events.h>
#include <engine/meta/ecs/entity.hpp>
#include <engine/scripting/ecs/components/script_component.h>
#include <engine/scripting/script.h>
#include <monopp/mono_exception.h>
#include <monopp/mono_internal_call.h>
#include <monopp/mono_method_invoker.h>
#include <monort/monort.h>

#include <filesystem/filesystem.h>
#include <logging/logging.h>

#include <engine/assets/impl/asset_compiler.h>

namespace ace
{
namespace
{
std::chrono::seconds check_interval(2);
std::atomic_bool initted{};

std::atomic_bool needs_recompile{};
std::mutex container_mutex;
std::set<std::string> needs_to_recompile;

auto print_assembly_info(const mono::mono_assembly& assembly)
{
    std::stringstream ss;
    auto refs = assembly.dump_references();

    ss << fmt::format(" ----- References -----");

    for(const auto& ref : refs)
    {
        ss << fmt::format("\n{}", ref);
    }

    APPLOG_TRACE("\n{}", ss.str());

    auto types = assembly.get_types();

    ss = {};
    ss << fmt::format(" ----- Types -----");

    for(const auto& type : types)
    {
        ss << fmt::format("\n{}", type.get_fullname());

        {
            auto attribs = type.get_attributes();
            for(const auto& attrib : attribs)
            {
                ss << fmt::format("\n - Attribute : {}", attrib.get_fullname());
            }
        }

        auto fields = type.get_fields();
        for(const auto& field : fields)
        {
            ss << fmt::format("\n - Field : {}", field.get_fullname());

            auto attribs = field.get_attributes();
            for(const auto& attrib : attribs)
            {
                ss << fmt::format("\n -- Attribute : {}", attrib.get_fullname());
            }
        }

        auto properties = type.get_properties();
        for(const auto& prop : properties)
        {
            ss << fmt::format("\n - Property : {}", prop.get_fullname());

            auto attribs = prop.get_attributes();
            for(const auto& attrib : attribs)
            {
                ss << fmt::format("\n -- Attribute : {}", attrib.get_fullname());
            }
        }
    }
    APPLOG_TRACE("\n{}", ss.str());
}

} // namespace

auto script_system::find_mono(const rtti::context& ctx) -> mono::compiler_paths
{
    bool is_deploy_mode = ctx.has<deploy>();

    mono::compiler_paths result;

    if(is_deploy_mode)
    {
        auto mono_dir = fs::resolve_protocol("engine:/mono");
        result.assembly_dir = fs::absolute(mono_dir / "lib").string();
        result.config_dir = fs::absolute(mono_dir / "etc").string();
    }
    else
    {
        const auto& names = mono::get_common_library_names();
        const auto& paths = mono::get_common_library_paths();

        auto found_library = fs::find_library(names, paths);

        result.assembly_dir = fs::absolute(found_library.parent_path() / ".." / "lib").string();
        result.config_dir = fs::absolute(fs::path(result.assembly_dir) / ".." / "etc").string();
    }

    {
        const auto& names = mono::get_common_executable_names();
        const auto& paths = mono::get_common_executable_paths();

        result.msc_executable = fs::find_program(names, paths).string();
    }

    APPLOG_TRACE("MONO_PATHS:");
    APPLOG_TRACE("Assembly path - {}", result.assembly_dir);
    APPLOG_TRACE("Config path - {}", result.config_dir);

    return result;
}

auto script_system::init(rtti::context& ctx) -> bool
{
    APPLOG_INFO("{}::{}", hpp::type_name_str(*this), __func__);

    auto& ev = ctx.get<events>();
    ev.on_frame_update.connect(sentinel_, this, &script_system::on_frame_update);
    ev.on_play_begin.connect(sentinel_, -1000, this, &script_system::on_play_begin);
    ev.on_play_end.connect(sentinel_, 1000, this, &script_system::on_play_end);
    ev.on_pause.connect(sentinel_, 100, this, &script_system::on_pause);
    ev.on_resume.connect(sentinel_, -100, this, &script_system::on_resume);
    ev.on_skip_next_frame.connect(sentinel_, -100, this, &script_system::on_skip_next_frame);

    if(mono::init(find_mono(ctx), true))
    {
        bind_internal_calls(ctx);

        mono::mono_domain::set_assemblies_path(fs::resolve_protocol("engine:/compiled").string());

        try
        {
            load_engine_domain(ctx);
        }
        catch(const mono::mono_exception& e)
        {
            APPLOG_ERROR("{}", e.what());
            return false;
        }

        initted = true;
        return true;
    }

    return false;
}

auto script_system::deinit(rtti::context& ctx) -> bool
{
    APPLOG_INFO("{}::{}", hpp::type_name_str(*this), __func__);

    unload_app_domain();
    unload_engine_domain();

    mono::shutdown();

    return true;
}

void script_system::load_engine_domain(rtti::context& ctx)
{
    if(!ctx.has<deploy>())
    {
        create_compilation_job(ctx, "engine").get();
    }

    domain_ = std::make_unique<mono::mono_domain>("Ace.Engine");
    mono::mono_domain::set_current_domain(domain_.get());

    auto engine_script_lib = fs::resolve_protocol(get_lib_compiled_key("engine"));

    auto assembly = domain_->get_assembly(engine_script_lib.string());
    print_assembly_info(assembly);

    cache_.update_manager_type = assembly.get_type("Ace.Core", "SystemManager");
}
void script_system::unload_engine_domain()
{
    cache_ = {};
    domain_.reset();
    mono::mono_domain::set_current_domain(nullptr);
}

auto script_system::load_app_domain(rtti::context& ctx, bool recompile) -> bool
{
    bool result = true;

    if(!ctx.has<deploy>() && recompile)
    {
        result &= create_compilation_job(ctx, "app").get();
    }

    app_domain_ = std::make_unique<mono::mono_domain>("Ace.App");
    mono::mono_domain::set_current_domain(app_domain_.get());

    auto app_script_lib = fs::resolve_protocol(get_lib_compiled_key("app"));

    if(!ctx.has<deploy>())
    {
        auto& am = ctx.get<asset_manager>();
        auto assets = am.get_assets<script>("app");
        // assets include the empty asset
        if(assets.size() <= 1)
        {
            return result;
        }
    }

    try
    {
        auto assembly = app_domain_->get_assembly(app_script_lib.string());
        print_assembly_info(assembly);

        auto engine_script_lib = fs::resolve_protocol(get_lib_compiled_key("engine"));
        auto engine_assembly = domain_->get_assembly(engine_script_lib.string());

        auto system_type = engine_assembly.get_type("Ace.Core", "ScriptSystem");
        app_cache_.scriptable_system_types = assembly.get_types_derived_from(system_type);

        auto comp_type = engine_assembly.get_type("Ace.Core", "ScriptComponent");
        app_cache_.scriptable_component_types = assembly.get_types_derived_from(comp_type);
    }
    catch(const mono::mono_exception& e)
    {
        APPLOG_ERROR("{}", e.what());
        result = false;
    }

    return result;
}
void script_system::unload_app_domain()
{
    app_cache_ = {};
    app_domain_.reset();
    mono::mono_domain::set_current_domain(domain_.get());
}

void script_system::on_create_component(entt::registry& r, const entt::entity e)
{
    // auto& comp = r.get<script_component>(e);
    // comp.create();
    // comp.start();
}
void script_system::on_destroy_component(entt::registry& r, const entt::entity e)
{
    auto& comp = r.get<script_component>(e);
    comp.destroy();
}

void script_system::on_play_begin(rtti::context& ctx)
{
    APPLOG_INFO("{}::{}", hpp::type_name_str(*this), __func__);

    if(!app_domain_ || !domain_)
    {
        return;
    }
    try
    {
        {
            scriptable_systems_.clear();
            scriptable_systems_.reserve(app_cache_.scriptable_system_types.size());
            for(const auto& type : app_cache_.scriptable_system_types)
            {
                auto obj = type.new_instance();
                scriptable_systems_.emplace_back(obj);
            }

            for(const auto& obj : scriptable_systems_)
            {
                auto method = mono::make_method_invoker<void()>(obj, "internal_n2m_on_create");
                method(obj);
            }

            for(const auto& obj : scriptable_systems_)
            {
                auto method = mono::make_method_invoker<void()>(obj, "internal_n2m_on_start");
                method(obj);
            }
        }

        auto& ec = ctx.get<ecs>();
        auto& scn = ec.get_scene();
        auto& registry = *scn.registry;

        registry.on_construct<script_component>().connect<&on_create_component>();
        registry.on_destroy<script_component>().connect<&on_destroy_component>();

        {
            auto& ec = ctx.get<ecs>();
            auto& scn = ec.get_scene();
            auto& registry = *scn.registry;

            {
                create_call_ = call_progress::started;

                registry.view<script_component>().each(
                    [&](auto e, auto&& comp)
                    {
                        comp.create();
                    });

                create_call_ = call_progress::finished;
            }

            {
                start_call_ = call_progress::started;

                registry.view<script_component>().each(
                    [&](auto e, auto&& comp)
                    {
                        comp.start();
                    });

                start_call_ = call_progress::finished;
            }
        }
    }
    catch(const mono::mono_exception& e)
    {
        APPLOG_ERROR("{}", e.what());
    }
}

void script_system::on_play_end(rtti::context& ctx)
{
    APPLOG_INFO("{}::{}", hpp::type_name_str(*this), __func__);

    auto& ec = ctx.get<ecs>();
    auto& scn = ec.get_scene();
    auto& registry = *scn.registry;

    try
    {
        registry.view<script_component>().each(
            [&](auto e, auto&& comp)
            {
                comp.destroy();
            });

        for(const auto& obj : scriptable_systems_)
        {
            auto method = mono::make_method_invoker<void()>(obj, "internal_n2m_on_destroy");
            method(obj);
        }
    }
    catch(const mono::mono_exception& e)
    {
        APPLOG_ERROR("{}", e.what());
    }

    registry.on_construct<script_component>().disconnect<&on_create_component>();
    registry.on_destroy<script_component>().disconnect<&on_destroy_component>();
}

void script_system::on_pause(rtti::context& ctx)
{
}

void script_system::on_resume(rtti::context& ctx)
{
}

void script_system::on_skip_next_frame(rtti::context& ctx)
{
}
void script_system::on_frame_update(rtti::context& ctx, delta_t dt)
{
    auto& ev = ctx.get<events>();
    if(!ev.is_playing)
    {
        check_for_recompile(ctx, dt);
    }

    try
    {
        if(!app_domain_ || !domain_)
        {
            return;
        }

        auto& ec = ctx.get<ecs>();
        auto& scn = ec.get_scene();
        auto& registry = *scn.registry;

        registry.view<script_component>().each(
            [&](auto e, auto&& comp)
            {
                comp.process_pending_deletions();
            });

        if(ev.is_playing && !ev.is_paused)
        {
            auto method_thunk = mono::make_method_invoker<void()>(cache_.update_manager_type, "internal_n2m_update");
            method_thunk();
        }
    }
    catch(const mono::mono_exception& e)
    {
        APPLOG_ERROR("{}", e.what());
    }
}

auto script_system::get_all_scriptable_components() const -> const std::vector<mono::mono_type>&
{
    return app_cache_.scriptable_component_types;
}

auto script_system::is_create_called() const -> bool
{
    return create_call_ == call_progress::finished;
}
auto script_system::is_start_called() const -> bool
{
    return start_call_ == call_progress::finished;
}

void script_system::check_for_recompile(rtti::context& ctx, delta_t dt)
{
    time_since_last_check_ += dt;

    if(time_since_last_check_ >= check_interval)
    {
        time_since_last_check_ = {};

        bool should_recompile = needs_recompile.exchange(false);

        if(should_recompile)
        {
            auto container = []()
            {
                std::lock_guard<std::mutex> lock(container_mutex);
                auto result = std::move(needs_to_recompile);
                return result;
            }();

            for(const auto& protocol : container)
            {
                create_compilation_job(ctx, protocol)
                    .then(itc::this_thread::get_id(),
                          [this, &ctx, protocol](auto f)
                          {
                              auto& ev = ctx.get<events>();

                              if(ev.is_playing)
                              {
                                  return;
                              }

                              bool result = f.get();
                              if(result)
                              {
                                  ev.on_script_recompile(ctx, protocol);
                              }
                          });
            }
        }
    }
}

auto script_system::create_compilation_job(rtti::context& ctx, const std::string& protocol) -> itc::job_future<bool>
{
    auto& thr = ctx.get<threader>();
    auto& am = ctx.get<asset_manager>();
    return thr.pool->schedule(
        [&am, protocol]()
        {
            auto key = get_lib_data_key(protocol);
            auto output = get_lib_compiled_key(protocol);

            return asset_compiler::compile<script_library>(am, key, fs::resolve_protocol(output));
        });
}
void script_system::set_needs_recompile(const std::string& protocol)
{
    if(!initted)
    {
        return;
    }
    needs_recompile = true;
    std::lock_guard<std::mutex> lock(container_mutex);
    needs_to_recompile.emplace(protocol);
}

auto script_system::get_lib_name(const std::string& protocol) -> std::string
{
    return protocol + "-script.dll";
}

auto script_system::get_lib_data_key(const std::string& protocol) -> std::string
{
    std::string output = get_lib_name(protocol + ":/data/" + protocol);
    return output;
}

auto script_system::get_lib_compiled_key(const std::string& protocol) -> std::string
{
    std::string output = get_lib_name(protocol + ":/compiled/" + protocol);
    return output;
}

} // namespace ace