#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DemoActionList;

/** @brief Animated joint locators used to position players during cutscenes. */
class DemoPlayerLocator : public al::LiveActor {
public:
    explicit DemoPlayerLocator(const char* pName);
    /** @brief Destroys the locator actor. */
    ~DemoPlayerLocator() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    virtual void initDemoSceneActor(const al::ActorInitInfo& rInfo,
        const al::ActorInitInfo& rParentInfo, const sead::Matrix34f* pParentMtx);
    virtual void startDemo(const sead::Matrix34f& rBaseMtx, int playerCount);
    virtual void endDemo();
    virtual void startAction(int index);
    const sead::Matrix34f* getPlayerLocatorMtxPtr(int index) const;
    void exeWait();

private:
    DemoActionList* mActions = nullptr;
    sead::Matrix34f mPlacementMtx;
    int mPlayerCount = 0;
};
