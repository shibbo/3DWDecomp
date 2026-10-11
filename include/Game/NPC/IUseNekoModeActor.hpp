#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "NPC/IUseNpcPuppet.hpp"
#include "NPC/IUseTargetFinderFilter.hpp"

class NpcTargetFinder;

namespace neko {

class Target;

/**
 * @brief Coat color (kind) of a cat.
 * @note The values have not been reconstructed yet; kinds up to 4 are regular cats.
 */
enum ColorType : s32 {};

/**
 * @brief Placement parameters shared by every cat mode actor.
 */
struct Param {
    const char* mComment = nullptr;
    bool mIsDisabledPR = false;
    bool mIsDisablePlessieChase = false;
    f32 mChaseRangeOverride = -1.0f;
    bool mIsEnableCliffCheck = true;
    bool mIsEnableShoreCheck = true;
    f32 mChaseRange = 1000.0f;
};

}  // namespace neko

/**
 * @brief Why a cat mode actor gets attached to its host cat.
 */
struct NekoAttachReason {
    /**
     * @brief Kinds of attach reasons.
     * @note Only the values used by reconstructed code are named.
     */
    enum Type : s32 {
        Type_StartDisaster = 1,
        Type_Hide = 2,
        Type_AppearAtHost = 3,
    };

    Type mType;
};

/**
 * @brief Base of the actors that implement one behavior mode of a cat (Neko).
 */
class IUseNekoModeActor : public al::LiveActor,
                          public IUseTargetFinderFilter,
                          public IUseNpcPuppet {
public:
    /**
     * @brief Construct the mode actor.
     * @param pName Name of the actor.
     */
    IUseNekoModeActor(const char* pName) : al::LiveActor(pName) {}

    using al::LiveActor::init;
    virtual void init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
                      NpcTargetFinder* pTargetFinder) = 0;
    virtual void startAttach(const NekoAttachReason& rReason) = 0;
    virtual void startKill(bool isDeleteParticle) = 0;
    virtual void startDisasterAnticipation(bool isEmitEffect) = 0;
    virtual bool startDisasterDemo() = 0;
    virtual bool startSeekTarget(const neko::Target* pTarget, bool isForce) = 0;
    virtual void startAppearLinks() = 0;
    virtual bool isHold() const = 0;
    virtual bool isRide() const = 0;
    virtual s32 getUID() const = 0;
    virtual neko::ColorType getNekoType() const = 0;
    virtual f32 getChaseRange() const = 0;
    virtual const neko::Param* getParam() const = 0;

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void startBindNpc(al::HitSensor* pSelf, al::HitSensor* pOther) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void endBindNpc(NpcPuppetBindEndType type) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void setTransVec(const sead::Vector3f& rTrans) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void setFrontVec(const sead::Vector3f& rFront) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void setUpVec(const sead::Vector3f& rUp) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void setMtx(const sead::Matrix34f* pMtx) override {}

    /** @brief Does nothing, cat modes are never bound as NPC puppets. */
    void setPlayerPuppetInputTurnStick(f32 x, f32 y) override {}

    /** @return Always the zero vector. */
    const sead::Vector3f& getTransVec() const override { return sead::Vector3f::zero; }

    /** @return Always the zero vector. */
    const sead::Vector3f& getFrontVec() const override { return sead::Vector3f::zero; }

    /** @return Always the zero vector. */
    const sead::Vector3f& getUpVec() const override { return sead::Vector3f::zero; }

    virtual void onStartHide() = 0;
};

static_assert(sizeof(IUseNekoModeActor) == 0x158);
