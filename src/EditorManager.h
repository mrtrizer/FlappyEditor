#pragma once

#include <memory>

#include <json/json.hpp>

#include <Manager.h>

class ProjectManager;

class EditorManager : public flappy::Manager<EditorManager> {
public:
    EditorManager(const std::string& projectPath);

    flappy::SafePtr<ProjectManager> projectManager();

    void selectScene(const std::string& scenePath);

private:
    std::shared_ptr<flappy::Entity> m_projectRoot;
    std::string m_scenePath;
    nlohmann::json m_serializedTree;

    bool m_libraryLoaded = false;
    bool m_sceneLoaded = false;
    bool m_sceneSelected = false;
    bool m_sceneCreated = false;

    bool createScene(const std::string &projectPath, const nlohmann::json& jsonTree);
};
