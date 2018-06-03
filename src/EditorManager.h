#pragma once

#include <memory>

#include <Manager.h>

class ProjectManager;

class EditorManager : public flappy::Manager<EditorManager> {
public:
    EditorManager(const std::string& projectPath);

    flappy::SafePtr<ProjectManager> projectManager();

private:
    std::shared_ptr<flappy::Entity> m_projectRoot;
};
