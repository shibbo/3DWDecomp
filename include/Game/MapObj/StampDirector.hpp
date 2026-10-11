#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <container/seadSafeArray.h>
#include <math/seadVector.h>

#include "Library/Scene/ISceneObj.hpp"

class alModelCafe;
class SnapshotLayout;

namespace agl {
class RenderBuffer;
}  // namespace agl
class DrcTouchAssistInfo;

namespace al {
class ActorInitInfo;
class IUseAudioKeeper;
class LayoutActor;
class LayoutResource;
class Resource;
}  // namespace al

namespace nn::ui2d {
class TextureInfo;
}  // namespace nn::ui2d

namespace rc {
class Stamp;

/** @brief Rotation input applied to the held stamp. */
enum StampRotateState {
    StampRotateState_None = 0,
    StampRotateState_Left = 1,
    StampRotateState_Right = 2,
};

/** @brief Stamp selection input (cycling through the unlocked stamps). */
enum StampIncrementState {
    StampIncrementState_None = 0,
    StampIncrementState_Increment = 1,
    StampIncrementState_Decrement = 2,
};

/**
 * @brief Scene object owning the collectible stamps and their resources.
 */
class StampDirector : public al::ISceneObj {
public:
    /** @brief Number of save files; each has its own unlock list, plus one for all files. */
    static constexpr s32 cFileNum = 4;
    static constexpr s32 cFileUnlockInfoNum = cFileNum + 1;

    /**
     * @brief Sorted list (and bit set) of the unlocked stamp ids.
     */
    struct StampUnlockInfo {
        void init(u32* pFlags);
        void addStamp(s32 stampId);
        s32 insertStamp(s32 stampId);
        void insertStamp(s32 index, s32 stampId);

        void setFlag(s32 stampId) {
            if (u32(stampId) < 128) {
                mFlags[stampId >> 5] |= 1 << stampId;
            }
        }

        s32 mCount;                          // 0x00
        u32 mFlags[4] = {};                  // 0x04
        sead::SafeArray<u8, 128> mStampIds;  // 0x14
    };

    static_assert(sizeof(StampUnlockInfo) == 0x94);

    StampDirector(const al::ActorInitInfo& rInfo, const char* pArchiveName,
                  const DrcTouchAssistInfo* pTouchInfo, s32 stampNum, s32 courseId);
    virtual ~StampDirector();

    s32 getAllUnlockedStamps(s32* pOutStampIds, s32 fileIndex);
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    bool updateUnlockedStamps(s32 fileIndex);
    void start();
    void finish();
    bool update(StampRotateState rotateState, StampIncrementState incrementState);
    void incrementStamp();
    void decrementStamp();
    void appearStamp(s32 stampId);
    void sleepStamp();
    s32 getNextStampID(s32 stampId, s32 direction);
    void refreshStampLayoutTexture();
    Stamp* getStamp(const sead::Vector3f& rTargetPos);
    Stamp* getNewStamp();
    void activateStamp(Stamp* pStamp, s32 stampId);
    void releaseStamp(Stamp* pStamp);
    void throwStamp(Stamp* pStamp, const sead::Vector3f& rVelocity);
    void hideAllStamps();
    void setCollectStamp(s32 courseId);
    Stamp* getStampFromModel(alModelCafe* pModel) const;
    Stamp* getActiveStamp() const;
    void draw2D(const agl::RenderBuffer* pRenderBuffer);

    const char* getSceneObjName() const override { return "Stamp Director"; }

    const al::Resource* getStampResource() const { return mStampResource; }

    al::LayoutResource* getLayoutResource() const { return mLayoutResource; }

    s32 getStartingStampId() const { return mStartingStampId; }

    bool isUseNoCodeWallFilter() const { return mIsUseNoCodeWallFilter; }

    void setUseNoCodeWallFilter(bool isUse) { mIsUseNoCodeWallFilter = isUse; }

    const al::IUseAudioKeeper* getAudioKeeperUser() const { return mAudioKeeperUser; }

private:
    al::Resource* mStampResource = nullptr;                       // 0x08
    al::LayoutResource* mLayoutResource = nullptr;                // 0x10
    sead::PtrArray<Stamp>* mStamps = nullptr;                     // 0x18
    const DrcTouchAssistInfo* mTouchAssistInfo = nullptr;         // 0x20
    Stamp* mActiveStamp = nullptr;                                // 0x28
    bool _30 = false;                                             // 0x30
    s32 mStartingStampId = 0;                                     // 0x34
    StampIncrementState mIncrementState = StampIncrementState_None;  // 0x38
    s32 mRepeatTimer = 10;                                        // 0x3C
    s32 mStampNum = 0;                                            // 0x40
    f32 mRotation;                                                // 0x44
    bool mIsHolding = false;                                      // 0x48
    bool mIsUseNoCodeWallFilter = false;                          // 0x49
    al::IUseAudioKeeper* mAudioKeeperUser = nullptr;              // 0x50
    SnapshotLayout* mSnapshotLayout = nullptr;                    // 0x58
    al::LayoutActor* mLayoutActor = nullptr;                      // 0x60
    nn::ui2d::TextureInfo* mTextureInfo;                          // 0x68
    StampUnlockInfo mUnlockInfo;                                  // 0x70
    s32 mCourseId;                                                // 0x104
    s32 mCollectStampId;                                          // 0x108
    s32 mCollectStampIndex;                                       // 0x10C
    StampUnlockInfo mFileUnlockInfos[cFileUnlockInfoNum];         // 0x110
};

static_assert(sizeof(StampDirector) == 0x3f8);
}  // namespace rc
