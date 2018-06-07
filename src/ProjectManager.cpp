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

// TODO: Move Property to a separate file
class Property {
public:
    Property(rttr::method setter, rttr::method getter)
        : m_setter(setter)
        , m_getter(getter)
    {}

    void setValue(rttr::variant variant, const std::string& value) {
        auto argumentInfos = m_setter.get_parameter_infos();
        std::vector<rttr::argument> arguments;
        if (argumentInfos.begin()->get_type() == rttr::type::get<int>())
            arguments.push_back(rttr::argument(json::parse(value).get<int>()));
        m_setter.invoke_variadic(variant, arguments);
    }
    std::string getValue(rttr::variant variant) {
        auto result = m_getter.invoke(variant);
        return result.to_string();
    }
private:
    rttr::method m_setter;
    rttr::method m_getter;
};

static std::vector<rttr::method> findMethods(rttr::type componentType, std::string name, rttr::type returnType) {
    for (auto method : componentType.get_methods()) {
        if (method.get_return_type() == returnType && method.get_name() == name)
            return {method};
    }
    return {};
}

// TODO: Make a constructor of PropertyList class
static std::unordered_map<std::string, Property> scanProperties(rttr::type componentType) {
    std::unordered_map<std::string, Property> properties;
    for (auto method : componentType.get_methods()) {
        auto args = method.get_parameter_infos();
        auto methodName = method.get_name().to_string();
        if (args.size() == 1 && methodName.substr(0, 3) == "set") {
            auto argType = args.begin()->get_type();
            auto getterName = methodName.substr(3);
            getterName[0] = tolower(getterName[0]);
            auto getterMethods = findMethods(componentType, getterName, argType);
            if (getterMethods.size() == 1)
                properties.emplace(getterName, Property(method, getterMethods[0]));
        }
    }
    return properties;
}

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
        if (m_root && eventHandle.id() != GetTypeId<EventHandle, EventHandle>::value())
            m_root->events()->post(eventHandle);
    });

    events()->subscribe([this, libraryPath] (const UpdateEvent&) {
        if (!m_loaded)
            reload(libraryPath);
    });

    RTTRService::instance().loadLibrary(libraryPath);
    m_loaded = true;
}

static json serializeEntity(const SafePtr<Entity>& entity) {
    if (!entity)
        return {};
    json jsonEntity;
    std::vector<json> jsonComponents;
    auto components = entity->findComponents<ComponentBase>();
    for (auto component : components) {
        json jsonComponent = { { "type", component->componentId().name() } };

        auto componentPointer = RTTRService::instance().toVariant(component);
        auto componentType = componentPointer.get_type();
        auto properties = scanProperties(componentType);
        for (auto property : properties) {
            jsonComponent[property.first] = json::parse(property.second.getValue(componentPointer));
        }

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

                auto componentPointer = RTTRService::instance().toVariant(component);
                auto componentType = componentPointer.get_type();
                auto properties = scanProperties(componentType);
                for (auto propertyPair : properties)
                    LOGI("property: %s", propertyPair.first.c_str());

                for (auto fieldIter = jsonComponent.begin(); fieldIter != jsonComponent.end(); fieldIter++) {
                    if (fieldIter.key() != "type") {
                        auto propertyIter = properties.find(fieldIter.key());
                        if (propertyIter != properties.end()) {
                            propertyIter->second.setValue(componentPointer, fieldIter.value().dump());
                            LOGI("Property: %s", propertyIter->second.getValue(componentPointer).c_str());
                        }
                    }
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
    if (m_root) {
        m_serializedTree = serializeEntity(m_root);
        m_root = nullptr;
    }
    m_loaded = false;
    try {
        RTTRService::instance().loadLibrary(libraryPath);
        m_root = loadEntity(m_serializedTree);
        m_loaded = true;
    } catch (const std::exception&) {

    }
}

void ProjectManager::loadFromJson(const json& jsonTree) {
    m_root = loadEntity(jsonTree);
}

nlohmann::json ProjectManager::saveToJson() {
    return serializeEntity(m_root);
}
