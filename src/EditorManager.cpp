#include "EditorManager.h"

#include <atomic>
#include <Entity.h>
#include <IFileMonitorManager.h>
#include <process.hpp>

#include "./ProjectManager.h"

using namespace flappy;
using namespace TinyProcessLib;

std::atomic<bool> flappyBuildStarted;

static std::string bashify(const std::string command) {
    std::stringstream ss;
    ss << "/bin/bash -c \"source ~/.bashrc;" << command << '\"';
    return ss.str();
}

static void runFlappyBuild(const std::string& projectPath) {
    std::thread flappyBuildThread ([projectPath]() {
        flappyBuildStarted = true;
        Process process(bashify("flappy build cmake +editor"), projectPath, [](const char *bytes, size_t n) {
                std::cout << std::string(bytes, n);
            }, [](const char *bytes, size_t n) {
                std::cout << std::string(bytes, n);
            });
        flappyBuildStarted = false;
        auto exit_status=process.get_exit_status();
        std::cout << "flappy build returned: " << exit_status << " (" << (exit_status==0?"success":"failure") << ")" << std::endl;
    });
    flappyBuildThread.detach();
}

EditorManager::EditorManager(const std::string& projectPath)
{
    addDependency(IFileMonitorManager::id());

    auto libraryPath = projectPath + "/generated/cmake/build/libTestProject.dylib";

    events()->subscribe([this, libraryPath, projectPath](InitEvent) {
        m_projectRoot = std::make_shared<Entity>();
        if (manager<IFileMonitorManager>()->exists(libraryPath)) {
            m_projectRoot->createComponent<ProjectManager>(projectPath);
        } else {
            runFlappyBuild(projectPath);
        }
        std::thread fsWatchThread ([projectPath]() {
            Process process(bashify("fswatch ./src ./res_src"), projectPath, [projectPath](const char *bytes, size_t n) {
                    if (!flappyBuildStarted) {
                        runFlappyBuild(projectPath);
                    }
                    std::cout << std::string(bytes, n);
                }, [](const char *bytes, size_t n) {
                    std::cout << std::string(bytes, n);
                });
            auto exit_status=process.get_exit_status();
            std::cout << "fswatch returned: " << exit_status << " (" << (exit_status==0?"success":"failure") << ")" << std::endl;
        });
        fsWatchThread.detach();
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
