#pragma once

#include <Component.h>

namespace flappy {

class OtherInternalComponent: public Component<OtherInternalComponent> {
public:
    OtherInternalComponent();
};

}
