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
#include <V8JSManager.h>
#include <JSComponent.h>
#include <SceneManager.h>
#include <Sdl2Manager.h>
#include <GLRenderManager.h>
#include <GLRenderElementFactory.h>
#include <ScreenManager.h>
#include <ThreadManager.h>
#include <WrapperInitializer.h>

#include "../test_project/src/Wrapper.h"

using namespace flappy;

std::shared_ptr<Entity> rootEntity;

typedef ProjectWrapper* (*InitProjectFunc)();

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Pass dynamic library of a project as argument." << std::endl;
        return 10;
    }

    PosixApplication application;
    auto currentThread = std::make_shared<DesktopThread>([argc, argv](SafePtr<Entity> rootEntity) {

        rootEntity->createComponent<Sdl2Manager>();
        rootEntity->createComponent<ScreenManager>(600, 600);

        void* handle = dlopen(argv[1], RTLD_NOW);
        if (!handle) {
            std::cout << "Could not open the library" << std::endl;
            return 1;
        }

        InitProjectFunc initProject = reinterpret_cast<InitProjectFunc>(dlsym(handle, "initProject"));
        if (!initProject) {
            std::cout << "Could not find symbol SayHello" << std::endl;
            dlclose(handle);
            return 2;
        }

        ProjectWrapper* wrapper = initProject();
        std::cout << wrapper->sum(10, 20) << std::endl;

        std::cout << "Project wrappers" << std::endl;
        auto projectWrappers = wrapper->getWrappers();
        for (auto wrapper : projectWrappers) {
            std::cout << wrapper.first.name() << " : " << wrapper.second.name << std::endl;
        }

        std::cout << "Editor wrappers" << std::endl;
        auto editorWrappers = getV8Wrappers();
        for (auto wrapper : editorWrappers) {
            std::cout << wrapper.first.name() << " : " << wrapper.second.name << std::endl;
        }

        rootEntity->createComponent<ResRepositoryManager>("./resources");
        rootEntity->createComponent<StdFileMonitorManager>();
        rootEntity->createComponent<StdFileLoadManager>();
        rootEntity->createComponent<TextResFactory>();
        rootEntity->createComponent<ResManager<TextRes>> ();

        rootEntity->component<V8JSManager>()->registerWrappers(editorWrappers);
        rootEntity->component<V8JSManager>()->registerWrappers(projectWrappers);

        auto childEntity = rootEntity->createEntity();
        auto jsRes = rootEntity->manager<ResManager<TextRes>>()->getRes("CppAccessComponent", ExecType::SYNC);
        auto jsComponent = childEntity->createComponent<JSComponent>("CppAccessComponent", jsRes);

        jsComponent->call("createInternal");

        {
            auto value = jsComponent->call("getInternalValue");
            LOGI("internalValue: %d", value.as<int>());
            jsComponent->call("setInternalValue", 700);
            // WARNING: You will get crash if you try to set value second time!!
            // I have no idea why yet.
            auto value1 = jsComponent->call("getInternalValue");
            LOGI("internalValue: %d", value1.as<int>());
        }

        // Scene
        auto sceneEntity = rootEntity->createEntity();
        sceneEntity->component<SceneManager>()->setMainCamera(sceneEntity->component<CameraComponent>());
        sceneEntity->component<CameraComponent>()->setSize({600, 600});
        sceneEntity->component<GLRenderManager>();
        sceneEntity->component<GLRenderElementFactory>();
    });
    return application.runThread(currentThread);
}
