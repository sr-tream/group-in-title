#define WLR_USE_UNSTABLE

#include <format>
#include <string>
#include <tuple>

#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>

#include "global.hpp"

// Methods
namespace hook {
    class CWindow final : public ::CWindow {
      public:
        inline static CFunctionHook* fetchTitleHook          = nullptr;
        inline static CFunctionHook* insertWindowToGroupHook = nullptr;
        inline static CFunctionHook* createGroupHook         = nullptr;
        inline static CFunctionHook* destroyGroupHook        = nullptr;

        inline static ::CWindow*     ignoredWindow = nullptr;
        EXPORT std::string fetchTitle() {
            const auto orig  = method_cast<std::string (*)(::CWindow*)>(fetchTitleHook->m_pOriginal);
            auto       title = orig(this); //(this->*orig)();

            if (m_sGroupData.pNextWindow.expired() || this == ignoredWindow)
                return title;

            PHLWINDOW head = getGroupHead();
            PHLWINDOW curr = head;

            size_t    groupId     = 1;
            size_t    groupsCount = 1;
            while (true) {
                curr = curr->m_sGroupData.pNextWindow.lock();
                if (curr == head)
                    break;

                if (curr.get() == ignoredWindow)
                    continue;

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
        EXPORT void createGroup() {
            const auto orig = method_cast<void (*)(::CWindow*)>(createGroupHook->m_pOriginal);

            orig(this);
            onUpdateMeta();
        }
        EXPORT void destroyGroup() {
            const auto orig = method_cast<void (*)(::CWindow*)>(destroyGroupHook->m_pOriginal);

            orig(this);
            onUpdateMeta();
        }
    };
    static_assert(sizeof(CWindow) == sizeof(::CWindow), "The `hook` class is not extend the `CWindow` class, and cannot be have additional fields!");

    class CKeybindManager final : public ::CKeybindManager {
      public:
        inline static CFunctionHook* moveWindowOutOfGroupHook = nullptr;

        EXPORT static void           moveWindowOutOfGroup(PHLWINDOW pWindow, const std::string& dir) {
            const auto orig       = (*(decltype(&moveWindowOutOfGroup))moveWindowOutOfGroupHook->m_pOriginal);
            auto       nextWindow = pWindow->m_sGroupData.pNextWindow.lock();

            orig(pWindow, dir);

            CWindow::ignoredWindow = pWindow.get();
            pWindow->onUpdateMeta();
            CWindow::ignoredWindow = nullptr;

            if (!nextWindow || nextWindow == pWindow)
                return;

            PHLWINDOW curr = nextWindow;
            while (true) {
                curr->onUpdateMeta();

                curr = curr->m_sGroupData.pNextWindow.lock();
                if (curr == nextWindow)
                    break;
            }
        }
    };
    static_assert(sizeof(CKeybindManager) == sizeof(::CKeybindManager), "The `hook` class is not extend the `CKeybindManager` class, and cannot be have additional fields!");

    class CCompositor final : public ::CCompositor {
      public:
        inline static CFunctionHook* closeWindowHook = nullptr;

        void                         closeWindow(PHLWINDOW pWindow) {
            const auto orig       = method_cast<void (*)(::CCompositor*, PHLWINDOW)>(closeWindowHook->m_pOriginal);
            auto       nextWindow = pWindow->m_sGroupData.pNextWindow.lock();

            if (!nextWindow || nextWindow == pWindow)
                return orig(this, pWindow);

            CWindow::ignoredWindow = pWindow.get();
            PHLWINDOW curr         = nextWindow;
            while (true) {
                curr->onUpdateMeta();

                curr = curr->m_sGroupData.pNextWindow.lock();
                if (curr == nextWindow)
                    break;
            }
            CWindow::ignoredWindow = nullptr;

            orig(this, pWindow);
        }
    };
    static_assert(sizeof(CCompositor) == sizeof(::CCompositor), "The `hook` class is not extend the `CCompositor` class, and cannot be have additional fields!");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    const std::string HASH = __hyprland_api_get_hash();
    if (HASH != GIT_COMMIT_HASH)
        HyprlandAPI::addNotification(handle, "[group-in-title] This plugin builded for other version of hyprland, please update it!", CColor{1.0, 0.2, 0.2, 1.0}, 10000);

    hook::CWindow::fetchTitleHook = make_hook(handle, method_cast(&hook::CWindow::fetchTitle), "CWindow");
    hook::CWindow::insertWindowToGroupHook =
        make_hook(handle, method_cast(&hook::CWindow::insertWindowToGroup), "insertWindowToGroupEN9Hyprutils6Memory14CSharedPointerIS_EE", "CWindow");
    hook::CWindow::createGroupHook                  = make_hook(handle, method_cast(&hook::CWindow::createGroup), "CWindow");
    hook::CWindow::destroyGroupHook                 = make_hook(handle, method_cast(&hook::CWindow::destroyGroup), "CWindow");
    hook::CKeybindManager::moveWindowOutOfGroupHook = make_hook(handle, (void*)&hook::CKeybindManager::moveWindowOutOfGroup, "CKeybindManager");
    hook::CCompositor::closeWindowHook              = make_hook(handle, method_cast(&hook::CCompositor::closeWindow), "CCompositor");

    enable_hooks(hook::CWindow::fetchTitleHook, hook::CWindow::insertWindowToGroupHook, hook::CWindow::createGroupHook, hook::CWindow::destroyGroupHook,
                 hook::CKeybindManager::moveWindowOutOfGroupHook, hook::CCompositor::closeWindowHook);

    return {"group-in-title", "A plugin to add group id in window title", "SR_team", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    disable_hooks(hook::CWindow::fetchTitleHook, hook::CWindow::insertWindowToGroupHook, hook::CWindow::createGroupHook, hook::CWindow::destroyGroupHook,
                  hook::CKeybindManager::moveWindowOutOfGroupHook, hook::CCompositor::closeWindowHook);
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}