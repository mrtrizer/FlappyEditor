#pragma once

#include <json/json.hpp>

#include <Manager.h>
#include <RTTRService.h>

class ProjectManager : public flappy::Manager<ProjectManager> {
public:
    ProjectManager(const std::string& projectPath);

    void reload(const std::string& libraryPath);

    void loadObjectTreeAsset(const std::string& filePath);

    void loadFromJson(const nlohmann::json& jsonTree);

    nlohmann::json saveToJson();

private:
    std::shared_ptr<flappy::Entity> m_root;
    nlohmann::json m_serializedTree;
    bool m_loaded;
};
