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

        for (auto managerPair : managers()) {
            LOGI("EditorManager Try send: %s", managerPair.second->componentId().name().c_str());
            m_projectRoot->events()->post(ManagerAddedEvent(managerPair.second));
        }

        runFlappyBuild(projectPath, "build");

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
        if (eventHandle.id() != GetTypeId<EventHandle, InitEvent>::value())
            m_projectRoot->events()->post(eventHandle);
    });

    events()->subscribe([this, projectPath, libraryPath](UpdateEvent) {
        auto fileMonitor = manager<IFileMonitorManager>();
        if (fileMonitor->exists(libraryPath) && fileMonitor->changed(libraryPath)) {
            m_libraryLoaded = false;
        }
        std::string fullScenePath = projectPath + "/" + m_scenePath;
        if (m_sceneSelected) {
            if (fileMonitor->exists(fullScenePath) && fileMonitor->changed(fullScenePath)) {
                m_projectRoot.reset();
                m_sceneLoaded = false;
                m_sceneCreated = false;
            }
        }
        if (!m_libraryLoaded) {
            try {
                m_projectRoot.reset();
                m_sceneCreated = false;
                RTTRService::instance().loadLibrary(libraryPath);
                m_libraryLoaded = true;
            } catch (const std::exception& e) {
                LOGE("Can't load library. %s", e.what());
            }
        }
        if (!m_sceneLoaded && m_sceneSelected) {
            try {
                auto sceneFileText = manager<IFileLoadManager>()->loadTextFile(fullScenePath);
                m_serializedTree = nlohmann::json::parse(sceneFileText);
                m_sceneLoaded = true;
            } catch (const std::exception& e) {
                LOGE("Can't load scene. %s", e.what());
            }
        }
        if (m_libraryLoaded && m_sceneLoaded && !m_sceneCreated)
            m_sceneCreated = createScene(projectPath, m_serializedTree);
    });

    events()->subscribe([this, libraryPath](DeinitEvent) {
        m_projectRoot.reset();
        m_libraryLoaded = false;
        m_sceneLoaded = false;
        m_sceneCreated = false;
    });
}

flappy::SafePtr<ProjectManager> EditorManager::projectManager() {
    return m_projectRoot != nullptr ? m_projectRoot->manager<ProjectManager>() : nullptr;
}

void EditorManager::selectScene(const std::string &scenePath) {
    m_scenePath = scenePath;
    m_sceneLoaded = false;
    m_sceneSelected = true;
}

bool EditorManager::createScene(const std::string& projectPath, const nlohmann::json& jsonTree) {
    try {
        m_projectRoot = std::make_shared<Entity>();
        for (auto managerPair : managers())
            m_projectRoot->events()->post(ManagerAddedEvent(managerPair.second));
        auto manager = m_projectRoot->createComponent<ProjectManager>(projectPath);
        manager->loadFromJson(jsonTree);
        return true;
    } catch (const std::exception& e) {
        LOGE("Can't load. %s", e.what());
        return false;
    }
}
