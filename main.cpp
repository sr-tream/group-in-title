#define WLR_USE_UNSTABLE

#include <format>
#include <string>

#include <hyprland/src/desktop/Window.hpp>

#include "global.hpp"

// Methods
static CFunctionHook* g_pFetchWindowHook = nullptr;

namespace hook {
    EXPORT std::string fetchTitle(CWindow* This) {
        const auto orig  = (*(decltype(&fetchTitle))g_pFetchWindowHook->m_pOriginal);
        auto       title = orig(This);

        if (!This->m_sGroupData.pNextWindow)
            return title;

        PHLWINDOW head = This->getGroupHead();
        PHLWINDOW curr = head;

        size_t    groupId     = 0;
        size_t    groupsCount = 0;
        while (true) {
            curr = curr->m_sGroupData.pNextWindow.lock();
            if (curr == head)
                break;

            groupsCount++;
            if (curr.get() == This)
                groupId = groupsCount;
        }

        return std::format("[{}/{}] {}", groupId + 1, groupsCount, title);
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    g_pFetchWindowHook = MAKE_HOOK(handle, fetchTitle, "CWindow");

    if (g_pFetchWindowHook == nullptr) {
        HyprlandAPI::addNotification(handle, "[group-in-title] Failure in initialization: Failed to find `CWindow::fetchTitle` with compatible signature",
                                     CColor{1.0, 0.2, 0.2, 1.0}, 7000);
        throw std::runtime_error("[group-in-title] Hooks fn init failed");
    }

    if (!g_pFetchWindowHook->hook()) {
        HyprlandAPI::addNotification(handle, "[group-in-title] Failure in initialization (hook failed)!", CColor{1.0, 0.2, 0.2, 1.0}, 7000);
        throw std::runtime_error("[group-in-title] Hooks failed");
    }

    return {"group-in-title", "A plugin to add group id in window title", "SR_team", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    if (g_pFetchWindowHook != nullptr)
        g_pFetchWindowHook->unhook();
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}