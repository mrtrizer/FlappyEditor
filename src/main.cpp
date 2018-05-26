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
#include <RTTRLibManager.h>

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

        auto rttrLibManager = rootEntity->createComponent<RTTRLibManager>();
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

        // Library playground
        auto& library = rttrLibManager->getLibrary(argv[1]);
        sceneEntity->addComponent(library.createComponent("InternalComponent"));

        auto componentVariant = library.toVariant(sceneEntity->componentById(TypeId<ComponentBase>("flappy::InternalComponent")));
        for (auto m: componentVariant.get_type().get_methods()) {
            std::cout << "Method: " <<  m.get_name() << " (";
            for (auto p: m.get_parameter_infos()) {
                std::cout <<  p.get_type().get_name() << " ";
            }
            std::cout << ") => " << m.get_return_type().get_name();
            std::cout << std::endl;
        }
        componentVariant.get_type().get_method("setValue").invoke(componentVariant, 200);
        int value = componentVariant.get_type().get_method("getValue").invoke(componentVariant).to_int();
        std::cout << "Result: " << value << std::endl;
    });
    return application.runThread(currentThread);
}
