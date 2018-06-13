#include "OtherInternalComponent.h"

#include <Entity.h>
#include <TextComponent.h>

#include "./InternalComponent.h"

namespace flappy {

OtherInternalComponent::OtherInternalComponent() {

    events()->subscribe([](InitEvent) {
        LOGI("OtherComponent initialized!");
    });

    events()->subscribe([this](UpdateEvent) {
        int value = entityPtr()->component<InternalComponent>()->value();
        //LOGI("value() => %d", value);
        entityPtr()->component<InternalComponent>()->setValue(value - 100);
        entityPtr()->component<TextComponent>()->setText(std::to_string(entityPtr()->component<InternalComponent>()->value()));
    });

    events()->subscribe([](InitEvent) {
        LOGI("OtherComponent deinitialized!");
    });
}

}
