#include "ProjectManager.h"

#include <Entity.h>

#include <StdFileLoadManager.h>
#include <ResManager.h>
#include <SpriteRes.h>
#include <GLTextureResFactory.h>
#include <SpriteResFactory.h>
#include <GLRenderElementFactory.h>
#include <GLShaderResFactory.h>
#include <TextureRes.h>
#include <FontRes.h>
#include <FontResFactory.h>
#include <GlyphSheetRes.h>
#include <GlyphSheetResFactory.h>
#include <Sdl2RgbaBitmapResFactory.h>
#include <Sdl2RgbaBitmapRes.h>
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
        std::string tmpStackStr;
        if (argumentInfos.begin()->get_type() == rttr::type::get<int>())
            arguments.push_back(rttr::argument(json::parse(value).get<int>()));
        else if (argumentInfos.begin()->get_type() == rttr::type::get<float>())
            arguments.push_back(rttr::argument(json::parse(value).get<float>()));
        else if (argumentInfos.begin()->get_type() == rttr::type::get<double>())
            arguments.push_back(rttr::argument(json::parse(value).get<double>()));
        else if (argumentInfos.begin()->get_type() == rttr::type::get<bool>())
            arguments.push_back(rttr::argument(json::parse(value).get<bool>()));
        else if (argumentInfos.begin()->get_type() == rttr::type::get<std::string>()) {
            tmpStackStr = json::parse(value).get<std::string>();
            arguments.push_back(rttr::argument(tmpStackStr));
        }
        // TODO: Handle enums
        // TODO: Handle pointers
        // TODO: Handle serializable structures
        m_setter.invoke_variadic(variant, arguments);
    }
    std::string getValue(rttr::variant variant) {
        auto result = m_getter.invoke(variant);
        auto type = result.get_type();
        // FIXME: Spike to represent string in json format
        if (type == rttr::type::get<std::string>())
            return "\"" + result.to_string() + "\"";
        else
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
    , m_projectPath(projectPath)
{
    // TODO: Compose correct path to the lib
    auto libraryPath = projectPath + "/generated/cmake/build/libTestProject.dylib";

    events()->subscribe([this, libraryPath, projectPath](InitEvent) {
        // TODO: Compose correct path to resources
        entity()->createComponent<ResRepositoryManager>(projectPath + "/generated/cmake/resources");
        entity()->createComponent<TextResFactory>();
        entity()->createComponent<GLRenderElementFactory>();
        entity()->createComponent<ResManager<JsonRes>> ();
        entity()->createComponent<DefaultResFactory<JsonRes, JsonRes, TextRes>>();
        entity()->createComponent<GLTextureResFactory>();
        entity()->createComponent<Sdl2RgbaBitmapResFactory> ();
        entity()->createComponent<GLShaderResFactory> ();
        entity()->createComponent<ResManager<ShaderRes>> ();
        entity()->createComponent<ResManager<IRgbaBitmapRes>> ();
        entity()->createComponent<ResManager<TextureRes>> ();
        entity()->createComponent<ResManager<TextRes>> ();
        entity()->createComponent<ResManager<GlyphSheetRes>> ();
        entity()->createComponent<GlyphSheetResFactory>();
        entity()->createComponent<ResManager<FontRes>> ();
        entity()->createComponent<FontResFactory>();
    });

    events()->subscribeAll([this] (const EventHandle& eventHandle) {
        // FIXME: Should check is event UpdateEvent
        if (m_root && eventHandle.id() != GetTypeId<EventHandle, EventHandle>::value()) {
            m_root->events()->post(eventHandle);
        }
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

        try {
            auto componentPointer = RTTRService::instance().toVariant(component);
            auto componentType = componentPointer.get_type();
            auto properties = scanProperties(componentType);
            for (auto property : properties) {
                try {
                    std::string value = property.second.getValue(componentPointer);
                    jsonComponent[property.first] = json::parse(value);
                } catch (const std::exception& e) {
                    LOGE("Can't parse. %s:%s. %s",
                         component->componentId().name().c_str(),
                         property.first.c_str(),
                         e.what());
                }
            }

            jsonComponents.push_back(jsonComponent);
        } catch (const std::exception& e) {
            LOGE("Can't serialize. %s. %s",
                 component->componentId().name().c_str(),
                 e.what());
        }
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
            try {
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
                        try {
                            if (fieldIter.key() != "type") {
                                auto propertyIter = properties.find(fieldIter.key());
                                if (propertyIter != properties.end()) {
                                    propertyIter->second.setValue(componentPointer, fieldIter.value().dump());
                                    LOGI("Property: %s", propertyIter->second.getValue(componentPointer).c_str());
                                }
                            }
                        } catch (const std::exception& e) {
                            LOGE("Can't parse. %s", e.what());
                        }
                    }
                }
            } catch (const std::exception& e) {
                LOGE("Can't create component. %s", e.what());
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
        LOGE("Serialized: %s", m_serializedTree.dump().c_str());
        m_root = nullptr;
    }
    m_loaded = false;
    try {
        RTTRService::instance().loadLibrary(libraryPath);
        loadFromJson(m_serializedTree);
        m_loaded = true;
    } catch (const std::exception& e) {
        LOGE("Can't load. %s", e.what());
    }
}

void ProjectManager::loadFromJson(const json& jsonTree) {
    m_root = loadEntity(jsonTree);

    for (auto managerPair : managers()) {
        LOGI("ProjectManager Try send: %s", managerPair.second->componentId().name().c_str());
        m_root->events()->post(ManagerAddedEvent(managerPair.second));
    }
}

nlohmann::json ProjectManager::saveToJson() {
    return serializeEntity(m_root);
}
