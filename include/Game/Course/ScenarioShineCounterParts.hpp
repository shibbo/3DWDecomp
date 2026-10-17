#pragma once
#include "Layout/Switch/SingleModeCounterBase.hpp"
#include "Layout/TextBoxTextInfo.hpp"
class ScenarioShineCounterParts : public SingleModeCounterBase {
public:
    ScenarioShineCounterParts(const al::LayoutInitInfo&, const char*, const char*, al::LayoutActor*);
    void control() override;
    void show() override;
    void forceHide();
    void forceEndAppear(int);
    void updateString(int);
    void endDemo();
    void handleIslandWarp();
    void updateCount(int);
    void addShine();
private:
    int mIsland = -1;
    fix::TextBoxTextInfo mShineText;
    fix::TextBoxTextInfo mBaseText;
};

static_assert(sizeof(ScenarioShineCounterParts) == 0x168);
