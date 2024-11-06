#define WLR_USE_UNSTABLE

#include <format>
#include <string>

#include <hyprland/src/desktop/Window.hpp>

#include "global.hpp"

// Methods
namespace hook {
    class CWindow final : public ::CWindow {
      public:
        inline static CFunctionHook* fetchTitleHook          = nullptr;
        inline static CFunctionHook* insertWindowToGroupHook = nullptr;

        EXPORT std::string fetchTitle() {
            const auto orig  = method_cast<std::string (*)(::CWindow*)>(fetchTitleHook->m_pOriginal);
            auto       title = orig(this); //(this->*orig)();

            if (m_sGroupData.pNextWindow.expired())
                return title;

            PHLWINDOW head = getGroupHead();
            PHLWINDOW curr = head;

            size_t    groupId     = 1;
            size_t    groupsCount = 1;
            while (true) {
                curr = curr->m_sGroupData.pNextWindow.lock();
                if (curr == head)
                    break;

                groupsCount++;
                if (curr.get() == this)
                    groupId = groupsCount;
            }

            return std::format("[{}/{}] {}", groupId, groupsCount, title);
        }
        EXPORT void insertWindowToGroup(PHLWINDOW pWindow) {
            const auto orig = method_cast<void (*)(::CWindow*, PHLWINDOW)>(insertWindowToGroupHook->m_pOriginal);

            orig(this, pWindow);

            PHLWINDOW curr = pWindow;
            while (true) {
                curr = curr->m_sGroupData.pNextWindow.lock();
                if (curr == pWindow)
                    break;

                curr->onUpdateMeta();
            }
        }
    };
    static_assert(sizeof(CWindow) == sizeof(::CWindow), "The `hook` class is not extend the `CWindow` class, and cannot be have additional fields!");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    hook::CWindow::fetchTitleHook = make_hook(handle, method_cast(&hook::CWindow::fetchTitle), "CWindow");
    hook::CWindow::insertWindowToGroupHook =
        make_hook(handle, method_cast(&hook::CWindow::insertWindowToGroup), "insertWindowToGroupEN9Hyprutils6Memory14CSharedPointerIS_EE", "CWindow");

    enable_hooks(hook::CWindow::fetchTitleHook, hook::CWindow::insertWindowToGroupHook);

    return {"group-in-title", "A plugin to add group id in window title", "SR_team", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    disable_hooks(hook::CWindow::fetchTitleHook, hook::CWindow::insertWindowToGroupHook);
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}