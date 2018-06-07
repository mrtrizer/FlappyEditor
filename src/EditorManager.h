#pragma once

#include <memory>

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
    // FIXME: Get rid of this flag
    bool m_updateScene = false;
};
