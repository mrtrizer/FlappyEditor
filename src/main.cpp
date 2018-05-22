#include <memory>
#include <dlfcn.h>

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
#include <rttr/library.h>

using namespace flappy;
using namespace rttr;

std::shared_ptr<Entity> rootEntity;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Pass dynamic library of a project as argument." << std::endl;
        return 10;
    }

    PosixApplication application;
    auto currentThread = std::make_shared<DesktopThread>([argc, argv](SafePtr<Entity> rootEntity) {

        rootEntity->createComponent<Sdl2Manager>();
        rootEntity->createComponent<ScreenManager>(600, 600);

        library lib(argv[1]); // file suffix is not needed, will be automatically appended
        lib.load();
        std::cout << "Loaded: " << lib.is_loaded() << std::endl;
        for (auto t : lib.get_types()) {
            std::cout << "Name: " <<  t.get_name() << std::endl;
        }

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
    });
    return application.runThread(currentThread);
}
