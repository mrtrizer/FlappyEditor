#include "ProjectManager.h"

#include <Entity.h>
#include <IFileMonitorManager.h>

#include <StdFileLoadManager.h>
#include <ResManager.h>
#include <SpriteRes.h>
#include <SpriteResFactory.h>
#include <TextureRes.h>
#include <StdFileMonitorManager.h>
#include <ResRepositoryManager.h>
#include <DefaultResFactory.h>
#include <TextResFactory.h>

using namespace flappy;
using json = nlohmann::json;

ProjectManager::ProjectManager(const std::string& projectPath)
    : m_root(std::make_shared<Entity>())
{
    auto libraryPath = projectPath + "/generated/cmake/build/libTestProject.dylib";

    addDependency(IFileMonitorManager::id());

    events()->subscribe([this, libraryPath, projectPath](InitEvent) {
        entity()->createComponent<ResRepositoryManager>(projectPath + "/generated/cmake/resources");
        entity()->createComponent<StdFileMonitorManager>();
        entity()->createComponent<StdFileLoadManager>();
        entity()->createComponent<TextResFactory>();
        entity()->createComponent<ResManager<TextRes>> ();
    });

    events()->subscribeAll([this] (const EventHandle& eventHandle) {
        if (eventHandle.id() != GetTypeId<EventHandle, EventHandle>::value())
            m_root->events()->post(eventHandle);
    });

    RTTRService::instance().loadLibrary(libraryPath);
}

static json serializeEntity(const SafePtr<Entity>& entity) {
    json jsonEntity;
    std::vector<json> jsonComponents;
    auto components = entity->findComponents<ComponentBase>();
    for (auto component : components) {
        json jsonComponent = { { "type", component->componentId().name() } };
        jsonComponents.push_back(jsonComponent);
    }
    jsonEntity["components"] = jsonComponents;
    auto nextEntities = entity->findEntities([](const auto&){ return true; });
    std::vector<json> jsonEntities;
    for (auto nextEntity : nextEntities) {
        jsonEntity = serializeEntity(nextEntity);
        jsonEntities.push_back(jsonEntity);
    }
    jsonEntity["entities"] = jsonEntities;
    return jsonEntity;
}

static std::shared_ptr<Entity> loadEntity(const json& jsonEntity) {
    auto entity = std::make_shared<Entity>();
    auto componentsIter = jsonEntity.find("components");
    if (componentsIter != jsonEntity.end() && componentsIter->is_array()) {
        for (auto jsonComponent : *componentsIter) {
            auto typeName = jsonComponent["type"].get<std::string>();
            LOGI("type: %s", typeName.c_str());
            auto typeId = TypeId<ComponentBase>(typeName);
            auto component = RTTRService::instance().createComponent(typeId);
            if (component != nullptr) {
                // TODO: Setup properties
                entity->addComponent(component);
            }
            for (auto fieldIter = jsonComponent.begin(); fieldIter != jsonComponent.end(); fieldIter++) {
                if (fieldIter.key() != "type") {
                    LOGI("method: %s", fieldIter.key().c_str());
                    int value = fieldIter.value().get<int>();

                    auto componentPointer = RTTRService::instance().toVariant(component);
                    auto componentType = componentPointer.get_type();

                    auto method = componentType.get_method(fieldIter.key());
                    auto args = method.get_parameter_infos();
                    for (auto arg : args) {
                        LOGI("arg: %s", arg.get_type().get_name().to_string().c_str());
                    }

                    std::vector<rttr::argument> arguments;
                    arguments.push_back(rttr::argument(value));
                    method.invoke_variadic(componentPointer, arguments);
                }
            }
        }
    }
    auto entitiesIter = jsonEntity.find("entities");
    if (entitiesIter != jsonEntity.end() && entitiesIter->is_array()) {
        for (auto nextJsonEntity : *entitiesIter) {
            entity->addEntity(loadEntity(nextJsonEntity));
        }
    }
    return entity;
}

void ProjectManager::reload(const std::string& libraryPath) {
    auto serializedTree = serializeEntity(m_root);
    m_root = nullptr;
    RTTRService::instance().loadLibrary(libraryPath);
    m_root = loadEntity(serializedTree);
}

void ProjectManager::loadFromJson(const json& jsonTree) {
    m_root = loadEntity(jsonTree);
}

nlohmann::json ProjectManager::saveToJson() {
    return serializeEntity(m_root);
}
