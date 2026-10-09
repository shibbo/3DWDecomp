#pragma once

#include <common/aglShaderLocation.h>
#include <common/aglTextureData.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Project/Collision/TriangleFilterBase.hpp"

class DrcTouchAssistInfo;

namespace al {
class TextureReplacer;
}  // namespace al

namespace nn::gfx {
class ResTextureFile;
}  // namespace nn::gfx

namespace rc {
class StampDirector;

/**
 * @brief Triangle filter for the stamp ray: rejects "NoCode" material triangles.
 */
class TriangleWallFilter : public al::TriangleFilterBase {
public:
    bool isInvalidTriangle(const al::Triangle& rTriangle) const override;
};

/**
 * @brief Like TriangleWallFilter, but also rejects "NoAction" walls.
 */
class TriangleWallNoCodeFilter : public al::TriangleFilterBase {
public:
    bool isInvalidTriangle(const al::Triangle& rTriangle) const override;
};

/**
 * @brief A collectible stamp the player can drag over the touch screen and stick onto the stage.
 */
class Stamp : public al::LiveActor {
public:
    static const char* getStampActorName();
    static bool isStampActor(const al::LiveActor* pActor);
    static void draw(void* pUserData, void* pModel);
    static f32 getDefaultWidth();

    Stamp(const al::ActorInitInfo& rInfo, const char* pArchiveName,
          const DrcTouchAssistInfo* pTouchInfo, StampDirector* pDirector, s32 depth);

    void initAfterPlacement() override;
    void movement() override;
    void draw() const override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    virtual void appear(s32 stampId);

    void setDrcTouchAssistInfo(const DrcTouchAssistInfo* pTouchInfo);
    f32 calculateCameraRotate();
    void setStamp();
    void setStamp(s32 stampId);
    void updatePos(bool isForce);
    bool canGrab() const;
    void forcePlace();
    void sleep();
    void startGrab(bool isKeepRotation);
    void startHover();
    void startPlaced();
    void startErase(const sead::Vector3f& rVelocity);
    void doRotate(f32 degree);
    void rotateStamp(f32 amount);
    bool calcHitPosAndNormal(bool isForce);
    void setRotation(f32 degree);
    const sead::Vector2f& getTextureScale() const;
    sead::Vector2f get2DPos() const;
    f32 getUIRotation() const;
    void trySetTexture(const char* pTextureName);
    bool initializeResTextureFile(nn::gfx::ResTextureFile* pTextureFile);

    void exeAppear();
    void exeDisappear();
    void exeSleep();
    void exeHeld();
    void exeHovered();
    void exePlaced();
    void exeErase();

    s32 getStampId() const { return mStampId; }

    s32 getDepth() const { return mDepth; }

    const char* getTextureName() const { return mTextureName; }

    bool isHidden() const { return mIsHidden; }

    al::TextureReplacer* getTextureReplacer() const { return mTextureReplacer; }

    const sead::Vector2f& getCurrentTextureScale() const { return mTextureScale; }

    f32 getRotation() const { return mRotation; }

    const agl::TextureData& getTextureData() const { return mTextureData; }

    static nn::gfx::ResTextureFile* spTextureFile[100];

private:
    sead::Vector3f mHitPos;                          // 0x144
    sead::Vector3f _150;                             // 0x150
    sead::Vector3f mHitNormal;                       // 0x15C
    sead::Vector3f _168;                             // 0x168
    f32 mCheckLength = 5000.0f;                      // 0x174
    f32 mPlacedRotation;                             // 0x178
    f32 mRotation;                                   // 0x17C
    f32 mOffsetRate;                                 // 0x180
    sead::Vector3f mOffsetDir;                       // 0x184
    f32 mCameraRotate;                               // 0x190
    u8 _194[0x14];                                   // 0x194
    sead::Vector3f mEraseVelocity;                   // 0x1A8
    agl::TextureData mTextureData;                   // 0x1B8
    agl::SamplerLocation mSamplerLocation;           // 0x2E0
    const DrcTouchAssistInfo* mDrcTouchAssistInfo;   // 0x2F8
    s32 mStampId = 0;                                // 0x300
    StampDirector* mStampDirector;                   // 0x308
    sead::Vector2f mTextureScale = {1.0f, 1.0f};     // 0x310
    al::TextureReplacer* mTextureReplacer = nullptr; // 0x318
    s32 mDepth;                                      // 0x320
    bool mIsFirstUpdate = false;                     // 0x324
    bool mIsHidden = false;                          // 0x325
    const char* mTextureName = nullptr;              // 0x328
};

static_assert(sizeof(Stamp) == 0x330);
}  // namespace rc
