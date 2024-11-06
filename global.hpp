#pragma once

#include <cassert>

#include <dlfcn.h>

#include <hyprland/src/plugins/PluginAPI.hpp>

inline CFunctionHook* make_hook(HANDLE plugin_handle, void* hook_address, const char* function_name, const char* namespace_name = "") {
    Dl_info info;
    dladdr(hook_address, &info);
    assert(info.dli_sname != nullptr && "Destination symbol is hidden");

    std::vector<SFunctionMatch> FNS;
    const auto hookName        = std::string_view{info.dli_sname};
    const auto                  functionNamePos = hookName.find(function_name);

    if (functionNamePos != std::string_view::npos) {
        const auto functionNmeWithArgs = hookName.substr(functionNamePos);
        FNS                            = HyprlandAPI::findFunctionsByName(plugin_handle, std::string(functionNmeWithArgs));
    } else
        FNS = HyprlandAPI::findFunctionsByName(plugin_handle, function_name);

    CFunctionHook* hook = nullptr;
    for (auto& fn : FNS) {
        if (!fn.demangled.contains(namespace_name))
            continue;

        hook = HyprlandAPI::createFunctionHook(plugin_handle, fn.address, hook_address);
        break;
    }

    return hook;
}

template <typename R, class C, typename... Args>
inline void* method_cast(R (C::*fn)(Args...)) {
    struct f {
        decltype(fn) fn_ptr;
    } _{fn};
    return *reinterpret_cast<void**>(&_);
}
template <typename To>
inline To method_cast(void* ptr) {
    return reinterpret_cast<To>(ptr);
}

inline void enable_hook(CFunctionHook* hook, int id = 0) {
    if (hook == nullptr)
        throw std::runtime_error("[group-in-title] Hook method id:" + std::to_string(id) + " is not found");

    if (!hook->hook())
        throw std::runtime_error("[group-in-title] Hook id:" + std::to_string(id) + " installation failed");
}
inline void disable_hook(CFunctionHook* hook) {
    if (hook != nullptr)
        hook->unhook();
}

template <class... Hooks>
    requires(std::same_as<Hooks, CFunctionHook*> && ...)
inline void enable_hooks(Hooks... hooks) try {
    int i = 0;
    (enable_hook(hooks, i++), ...);
} catch (std::exception& e) {
    (disable_hook(hooks), ...);
    std::rethrow_exception(std::current_exception());
}
template <class... Hooks>
    requires(std::same_as<Hooks, CFunctionHook*> && ...)
inline void disable_hooks(Hooks... hooks) try {
    (disable_hook(hooks), ...);
} catch (std::exception& e) {
    // Silent fail
}
