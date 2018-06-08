#include "EditorManager.h"

#include <atomic>
#include <Entity.h>
#include <IFileMonitorManager.h>
#include <IFileLoadManager.h>
#include <process.hpp>

#include "./ProjectManager.h"

using namespace flappy;
using namespace TinyProcessLib;

std::atomic<bool> flappyBuildStarted;

// TODO: Find crossplatform solution
static std::string bashify(const std::string command) {
    std::stringstream ss;
    ss << "/bin/bash -c \"source ~/.bashrc;" << command << '\"';
    return ss.str();
}

static void runFlappyBuild(const std::string& projectPath, const std::string& scriptName) {
    std::thread flappyBuildThread ([projectPath, scriptName]() {
        flappyBuildStarted = true;
        Process process(bashify("flappy " + scriptName + " cmake +editor"), projectPath, [](const char *bytes, size_t n) {
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

// TODO: Implement process as a class with possibility to kill it and set callbacks
static void runProcess(const std::string& projectPath, const std::string& execPath, std::function<void(const char *bytes, size_t n)> callback) {
    std::thread fsWatchThread ([projectPath, execPath, callback]() {
        Process process(bashify(execPath), projectPath, callback,
            [](const char *bytes, size_t n) {
                std::cout << std::string(bytes, n);
            });
        auto exit_status=process.get_exit_status();
        std::cout << "fswatch returned: " << exit_status << " (" << (exit_status==0?"success":"failure") << ")" << std::endl;
    });
    fsWatchThread.detach();
}

EditorManager::EditorManager(const std::string& projectPath)
{
    addDependency(IFileMonitorManager::id());

    // FIXME: Search actual location of library
    auto libraryPath = projectPath + "/generated/cmake/build/libTestProject.dylib";

    events()->subscribe([this, libraryPath, projectPath](InitEvent) {
        m_projectRoot = std::make_shared<Entity>();
        if (manager<IFileMonitorManager>()->exists(libraryPath)) {
            m_projectRoot->createComponent<ProjectManager>(projectPath);
        } else {
            runFlappyBuild(projectPath, "build");
        }
        runProcess(projectPath, "fswatch ./src ./flappy_conf", [projectPath](const char *bytes, size_t n) {
            if (!flappyBuildStarted)
                runFlappyBuild(projectPath, "build");
            std::cout << std::string(bytes, n);
        });
        runProcess(projectPath, "fswatch ./res_src", [projectPath](const char *bytes, size_t n) {
            if (!flappyBuildStarted)
                runFlappyBuild(projectPath, "pack_res");
            std::cout << std::string(bytes, n);
        });
    });

    events()->subscribeAll([this] (const EventHandle& eventHandle) {
        if (!isInitialized())
            return;
        if (eventHandle.id() != GetTypeId<EventHandle, EventHandle>::value())
            m_projectRoot->events()->post(eventHandle);
    });

    events()->subscribe([this, projectPath, libraryPath](UpdateEvent) {
        auto fileMonitor = manager<IFileMonitorManager>();
        if (fileMonitor->exists(libraryPath) && fileMonitor->changed(libraryPath)) {
            try {
                if (m_projectRoot->findComponent<ProjectManager>() == nullptr)
                    m_projectRoot->createComponent<ProjectManager>(libraryPath);
                else
                    m_projectRoot->findComponent<ProjectManager>()->reload(libraryPath);
            } catch (const std::exception& e) {
                LOGE("%s", e.what());
            }
        }

        std::string fullScenePath = projectPath + "/" + m_scenePath;
        if (fileMonitor->exists(fullScenePath) && (fileMonitor->changed(fullScenePath) || m_updateScene)) {
            auto sceneFileText = manager<IFileLoadManager>()->loadTextFile(fullScenePath);
            auto jsonTree = nlohmann::json::parse(sceneFileText);
            projectManager()->loadFromJson(jsonTree);
            m_updateScene = false;
        }
    });

    events()->subscribe([this, libraryPath](DeinitEvent) {
        m_projectRoot.reset();
    });
}

flappy::SafePtr<ProjectManager> EditorManager::projectManager() {
    return m_projectRoot != nullptr ? m_projectRoot->manager<ProjectManager>() : nullptr;
}

void EditorManager::selectScene(const std::string &scenePath) {
    m_scenePath = scenePath;
    m_updateScene = true;
}
