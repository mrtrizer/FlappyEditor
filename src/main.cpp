#include <memory>
#include <dlfcn.h>
#include <json/json.hpp>

#include <Entity.h>
#include <AppManager.h>
#include <TransformComponent.h>
#include <ResManager.h>
#include <SpriteRes.h>
#include <SpriteResFactory.h>
#include <TextureRes.h>
#include <StdFileMonitorManager.h>
#include <ResRepositoryManager.h>
#include <StdFileLoadManager.h>
#include <DefaultResFactory.h>
#include <TextResFactory.h>
#include <DesktopThread.h>
#include <PosixApplication.h>
#include <AtlasResFactory.h>
#include <SceneManager.h>
#include <Sdl2Manager.h>
#include <GLRenderManager.h>
#include <GLRenderElementFactory.h>
#include <ScreenManager.h>
#include <ThreadManager.h>
#include <ProjectManager.h>

#include "./EditorManager.h"

using namespace flappy;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Pass dynamic library of a project as argument." << std::endl;
        return 10;
    }

    PosixApplication application;
    auto currentThread = std::make_shared<DesktopThread>([argc, argv](SafePtr<Entity> rootEntity) {

        rootEntity->createComponent<Sdl2Manager>();
        rootEntity->createComponent<ScreenManager>(600, 600);

        rootEntity->createComponent<ResRepositoryManager>("./resources");
        rootEntity->createComponent<StdFileMonitorManager>();
        rootEntity->createComponent<StdFileLoadManager>();
        rootEntity->createComponent<TextResFactory>();
        rootEntity->createComponent<ResManager<TextRes>> ();

        // Scene
        auto sceneEntity = rootEntity->createEntity();
        sceneEntity->component<SceneManager>()->setMainCamera(sceneEntity->component<CameraComponent>());
        sceneEntity->component<CameraComponent>()->setSize({600, 600});
        sceneEntity->component<GLRenderManager>();
        sceneEntity->component<GLRenderElementFactory>();

        // Editor initialization
        auto editor = sceneEntity->createComponent<EditorManager>(argv[1]);
        auto jsonTree = nlohmann::json::parse("{\"components\":[{\"type\":\"flappy::InternalComponent\"}, {\"type\":\"flappy::OtherInternalComponent\"}]}");
        editor->projectManager()->loadFromJson(jsonTree);
    });
    return application.runThread(currentThread);
}
