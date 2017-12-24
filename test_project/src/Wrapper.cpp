#include "Wrapper.h"
#include <WrapperInitializer.h>

int ProjectWrapper::sum(int a, int b) {
    return a * b;
}

flappy::TypeMap<void, flappy::Wrapper> ProjectWrapper::getWrappers() {
    return flappy::getV8Wrappers();
}

ProjectWrapper* initProject() {
    return new ProjectWrapper();
}
