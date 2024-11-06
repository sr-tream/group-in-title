#pragma once

#include <cassert>

#include <dlfcn.h>

#include <hyprland/src/plugins/PluginAPI.hpp>

inline CFunctionHook* make_hook(HANDLE plugin_handle, void* hook_address, const char* function_name, const char* namespace_name = "") {
    Dl_info info;
    dladdr(hook_address, &info);
    assert(info.dli_sname != nullptr && "Destination symbol is hidden");

    const auto hookName        = std::string_view{info.dli_sname};
    const auto functionNamePos = hookName.find(function_name);
    assert(functionNamePos != std::string_view::npos && "name of destination symbol is not match the `function_name`");

    const auto     functionNmeWithArgs = hookName.substr(functionNamePos);

    auto           FNS  = HyprlandAPI::findFunctionsByName(plugin_handle, std::string(functionNmeWithArgs));
    CFunctionHook* hook = nullptr;
    for (auto& fn : FNS) {
        if (!fn.demangled.contains(namespace_name))
            continue;

        hook = HyprlandAPI::createFunctionHook(plugin_handle, fn.address, hook_address);
        break;
    }

    return hook;
}
#define MAKE_HOOK(plugin_handle, destination, namespace_name) make_hook(plugin_handle, (void*)&hook::destination, #destination, namespace_name)