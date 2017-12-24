#pragma once

#include <Component.h>

namespace flappy {

class InternalComponent: public Component<InternalComponent> {
public:
    void setValue(int value) { m_value = value; }
    int getValue() { return m_value; }
private:
    int m_value = 100;
};

}
