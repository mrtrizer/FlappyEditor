#pragma once

#include <TypeMap.h>
#include <V8JSManager.h>

class ProjectWrapper
{
public:
    ProjectWrapper() = default;
    virtual ~ProjectWrapper() = default;

    virtual int sum(int a, int b);
    virtual flappy::TypeMap<void, flappy::Wrapper> getWrappers();
};

extern "C"
{
    ProjectWrapper* initProject();
}
