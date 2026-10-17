#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class SingleModeCounterBase : public al::LayoutActor {
public:
    SingleModeCounterBase(const al::LayoutInitInfo&, const char*, const char*, al::LayoutActor*);
    virtual void show();
    virtual void hide();
    void exeAppear();
};
