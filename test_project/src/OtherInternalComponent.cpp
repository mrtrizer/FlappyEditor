#include "OtherInternalComponent.h"

#include <Entity.h>

#include "./InternalComponent.h"

namespace flappy {

OtherInternalComponent::OtherInternalComponent() {

    events()->subscribe([](InitEvent) {
        LOGI("OtherComponent initialized!");
    });

    events()->subscribe([this](UpdateEvent) {
        int value = entityPtr()->component<InternalComponent>()->value();
        LOGI("value() => %d", value);
        entityPtr()->component<InternalComponent>()->setValue(value + 1);
    });

    events()->subscribe([](InitEvent) {
        LOGI("OtherComponent deinitialized!");
    });
}

}
