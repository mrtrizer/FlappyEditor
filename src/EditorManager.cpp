#include "EditorManager.h"

#include <Entity.h>
#include <IFileMonitorManager.h>
#include <process.hpp>

#include "./ProjectManager.h"

using namespace flappy;
using namespace TinyProcessLib;

EditorManager::EditorManager(const std::string& projectPath)
{
    addDependency(IFileMonitorManager::id());

    auto libraryPath = projectPath + "/generated/cmake/build/libTestProject.dylib";

    events()->subscribe([this, libraryPath, projectPath](InitEvent) {
        m_projectRoot = std::make_shared<Entity>();
        if (manager<IFileMonitorManager>()->exists(libraryPath)) {
            m_projectRoot->createComponent<ProjectManager>(projectPath);
        } else {
            Process process("flappy build cmake +editor", projectPath, [](const char *bytes, size_t n) {
                    std::cout << std::string(bytes, n);
                }, [](const char *bytes, size_t n) {
                    std::cout << std::string(bytes, n);
                });
        }
    });

    events()->subscribeAll([this] (const EventHandle& eventHandle) {
        if (!isInitialized())
            return;
        if (eventHandle.id() != GetTypeId<EventHandle, EventHandle>::value())
            m_projectRoot->events()->post(eventHandle);
    });

    events()->subscribe([this, libraryPath](UpdateEvent) {
        if (manager<IFileMonitorManager>()->changed(libraryPath)) {
            try {
                if (m_projectRoot->findComponent<ProjectManager>() == nullptr)
                    m_projectRoot->createComponent<ProjectManager>(libraryPath);
                else
                    m_projectRoot->findComponent<ProjectManager>()->reload(libraryPath);
            } catch (const std::exception& e) {
                LOGE("%s", e.what());
            }
        }
    });

    events()->subscribe([this, libraryPath](DeinitEvent) {
        m_projectRoot.reset();
    });
}

flappy::SafePtr<ProjectManager> EditorManager::projectManager() {
    return m_projectRoot != nullptr ? m_projectRoot->manager<ProjectManager>() : nullptr;
}
