#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <gfx/seadColor.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class JointLookAtController;
class JointRumbler;
class JointSpringController;
}  // namespace al

class FurKeeper;
class IUsePlayerRetargettingSelector;
class PlayerModelIK;
class PlayerModelHair;

/// The model actor of one of the player's figures.
class PlayerModel : public al::LiveActor {
public:
    typedef sead::PtrArray<al::JointSpringController> SpringControllerArray;

    PlayerModel(const char* pName, const char* pArchiveName, const al::ActorInitInfo& rInfo,
                const char* pSuffix);

    void draw() const override;
    void movement() override;
    void calcAnim() override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;

    void startAction(const char* pActionName, IUsePlayerRetargettingSelector* pSelector);
    void setFrame(s32 frame);
    void initJointController(const sead::Vector2f& rPitchRange, const bool* pIsMash,
                             const sead::Quatf* pSpineQuat, f32 lookAtRate);
    void startRumble(u32 index);
    void setInvincibleColor(const sead::Color4f& rColor);
    void createFur();
    void hideFur();
    void showFur();
    void createIK();
    void createHairCtrl();
    void setShadowLength(f32 length);
    void createSkirtDynamics();
    void validateSkirtDynamics();
    void invalidateSkirtDynamics();
    bool isExistSkirtDynamics() const;
    bool isValidSkirtDynamics() const;
    void resetSkirtDynamics();
    void createTailJointController();
    void validateTailDynamics();
    void invalidateTailDynamics();
    bool isExistTailDynamics() const;
    bool isValidTailDynamics() const;
    void resetTailDynamics();
    void createHairJointController();
    void validateHairDynamics();
    void invalidateHairDynamics();
    bool isExistHairDynamics() const;
    bool isValidHairDynamics() const;
    void resetHairDynamics();

    static void createStandardCtrl(PlayerModel* pModel);

    /** @brief Gets the index used to pick this model's retargetting info. @return Index. */
    s32 getRetargettingIndex() const { return mRetargettingIndex; }

    /** @brief Gets the name of this model's animation set (e.g. "Climb"). @return Name. */
    const char* getAnimSetName() const { return mAnimSetName; }

    /** @brief Gets the foot IK (created by createIK). @return IK, or nullptr. */
    PlayerModelIK* getIK() const { return mIK; }

    /** @brief Gets the hair controller (created by createHairCtrl). @return Controller, or nullptr. */
    PlayerModelHair* getHairCtrl() const { return mHairCtrl; }

private:
    s32 mRetargettingIndex = 0;                       // 0x144
    s32 _148 = 0;                                     // 0x148
    void* _150 = nullptr;                             // 0x150
    const char* mAnimSetName = nullptr;               // 0x158
    void* _160 = nullptr;                             // 0x160
    al::JointRumbler* mRumblers[2] = {};              // 0x168
    al::JointLookAtController* mLookAtCtrl = nullptr;  // 0x178
    sead::PtrArray<FurKeeper> mFurKeepers;            // 0x180
    PlayerModelIK* mIK = nullptr;                     // 0x190
    PlayerModelHair* mHairCtrl = nullptr;             // 0x198
    SpringControllerArray mSkirtSprings;              // 0x1a0
    bool mIsValidSkirtDynamics = false;               // 0x1b0
    SpringControllerArray mTailSprings;               // 0x1b8
    SpringControllerArray mHairSprings;               // 0x1c8
    bool mIsValidTailDynamics = false;                // 0x1d8
    bool mIsValidHairDynamics = false;                // 0x1d9
};

static_assert(sizeof(PlayerModel) == 0x1e0);
