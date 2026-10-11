#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/Scene/ISceneObj.hpp"
#include "MapObj/IGoalItemCollectListener.hpp"
#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/Neko.hpp"

namespace al {
class CameraTicket;
}  // namespace al

class ActorStateSupportStroke;
class DummyCameraTarget;
class GoalItem;
class GuideBalloon;
class NekoNormal;
class NpcHeadController;

/**
 * @brief Mode actor of a mother cat that waits for her lost kittens and rewards the player for
 * bringing them back.
 */
class NekoParent : public IUseNekoModeActor, public IGoalItemCollectListener {
public:
    NekoParent(Neko* pHost);

    void init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
              NpcTargetFinder* pTargetFinder) override;
    bool tryStartDefaultBehavior();
    void addSpecialShineLocation();
    void forceHideGuideBalloon();
    void startKillHost();
    bool tryKillAll();
    void startAppearHost();
    void startClipped() override;
    void endClipped() override;
    void startAttach(const NekoAttachReason& rReason) override;
    void startAppearLinks() override;
    void startDisasterAnticipation(bool isEmitEffect) override;
    void startKill(bool isDeleteParticle) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isDemoGoalSequence() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    virtual bool isInteractive() const;
    void goalItemCollectCallback() override;
    bool acceptTarget(const al::LiveActor* pTarget,
                      const npc::NpcFindTargetType& rType) const override;
    bool isOwnKitten(const IUseNekoModeActor* pNeko) const;
    void exeWait();
    void updateTryCollectNeko(bool isShowGuide);
    bool tryStartDemoIntro();
    void exeWaitDisasterAnticipation();
    void exeDemoIntro();
    void exeDemoTargetWait();
    bool tryStartDemoCollect();
    void updateDemoCamera();
    void endDemoCollect();
    void exeDemoCoinGive();
    void exeDemoTakeOut();
    void exeGoalWait();
    void exeAppear();
    void exeHitReact();
    void exeStroke();
    bool tryStartReactToPlayer();
    bool tryCollectNeko(IUseNekoModeActor* pNeko);
    neko::Target* tryGetNearDropTarget(const al::LiveActor* pActor) const;
    s32 getMaxKittens() const;
    s32 getKittensFound() const;

    /** @return Always false, the parent has no disaster demo. */
    bool startDisasterDemo() override { return false; }

    /** @return Always false, the parent never seeks a target. */
    bool startSeekTarget(const neko::Target* pTarget, bool isForce) override { return false; }

    /** @return Always false, the parent cannot be held. */
    bool isHold() const override { return false; }

    /** @return Always false, the parent cannot be ridden. */
    bool isRide() const override { return false; }

    /** @return Unique id of the host cat. */
    s32 getUID() const override { return mHost->getUID(); }

    /** @return Coat color of the cat. */
    neko::ColorType getNekoType() const override { return mColorType; }

    /** @return Range within which the parent reacts to the player. */
    f32 getChaseRange() const override { return mParam->mChaseRange; }

    /** @return Placement parameters of the cat. */
    const neko::Param* getParam() const override { return mParam; }

    /** @brief Does nothing, the parent cannot hide. */
    void onStartHide() override {}

private:
    /**
     * @brief Check whether every kitten was brought back.
     * @return Whether every kitten was brought back.
     */
    bool isAllKittensFound() const { return getKittensFound() >= getMaxKittens(); }

    /**
     * @brief Check whether a kitten already sits on one of the drop targets.
     * @param pKitten Kitten to check.
     * @return Whether the kitten sits on a drop target.
     */
    bool isKittenAtDropTarget(const Neko* pKitten) const {
        for (s32 i = 0; i < mDropTargets.size(); i++) {
            const neko::Target* target = mDropTargets.unsafeAt(i);
            if (!target->mIsActive && target->mId == pKitten->getUID()) {
                return true;
            }
        }

        return false;
    }

    Neko* mHost = nullptr;
    neko::ColorType mColorType;
    neko::Param* mParam = nullptr;
    NpcTargetFinder* mTargetFinder = nullptr;
    NekoNormal* mCollectedNeko = nullptr;
    IUseNekoModeActor* mTouchNeko = nullptr;
    al::LiveActor* mCollectPlayer = nullptr;
    neko::Target* mCollectTarget;
    sead::PtrArray<neko::Target> mDropTargets;
    sead::PtrArray<Neko> mKittens;
    sead::PtrArray<Neko> mFoundKittens;
    GoalItem* mGoalItem = nullptr;
    Neko* mNextParent = nullptr;
    GuideBalloon* mGuideBalloon = nullptr;
    sead::Vector3f* mHeadPos = new sead::Vector3f();
    ActorStateSupportStroke* mStateSupportStroke = nullptr;
    s32 mHitReactCoolTime = 0;
    NpcHeadController* mHeadController = nullptr;
    DummyCameraTarget* mDemoCameraTarget = nullptr;
    al::CameraTicket* mDemoCamera = nullptr;
    bool mIsSeenDemo = false;
    s32 mPackunEatCoolTime = 0;
};

static_assert(sizeof(NekoParent) == 0x220);

/**
 * @brief Scene object that keeps track of every cat parent of the scene.
 */
class NekoParentHolder : public al::ISceneObj {
public:
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;

    /** @return Name of the scene object. */
    const char* getSceneObjName() const override { return "NekoParentHolder"; }

    /**
     * @brief Register a cat parent.
     * @param pParent The cat parent.
     */
    void add(NekoParent* pParent) { mParents.pushBack(pParent); }

    /**
     * @brief Set the cat parent whose kittens are currently searched.
     * @param pParent The cat parent.
     */
    void setCurrentParent(NekoParent* pParent) { mCurrentParent = pParent; }

private:
    sead::FixedPtrArray<NekoParent, 8> mParents;
    NekoParent* mCurrentParent;
};

static_assert(sizeof(NekoParentHolder) == 0x60);
