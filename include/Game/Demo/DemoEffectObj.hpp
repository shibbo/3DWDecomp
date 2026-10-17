#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Effect actor with a transform relative to a demo scene. */
class DemoEffectObj : public al::LiveActor {
public:
    explicit DemoEffectObj(const char* pName);
    /** @brief Destroys the effect actor. */
    ~DemoEffectObj() override = default;
    void initDemoSceneActor(const al::ActorInitInfo& rInfo, const al::ActorInitInfo& rDemoInfo,
                           const sead::Matrix34f* pBaseMtx);
    void setPlacementBaseMtx(const sead::Matrix34f* pBaseMtx);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void makeActorAppeared() override;
    void control() override;
    void appear() override;
    void kill() override;
    /** @brief Gets the world transform. @return Effect placement matrix. */
    const sead::Matrix34f* getBaseMtx() const override { return &mBaseMtx; }

private:
    sead::Matrix34f mBaseMtx;
    sead::Matrix34f mLocalMtx;
    const sead::Matrix34f* mPlacementBaseMtx = nullptr;
};
