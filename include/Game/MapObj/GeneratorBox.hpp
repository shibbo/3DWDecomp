#pragma once

#include <math/seadBoundBox.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ComboCounter;
}  // namespace al

class BlockEmpty;
class GeneratorBoxChild;

/**
 * @brief The "?" generator box that stacks up a column of child blocks when it is hit.
 *
 * Every hit appends one child block (a hip drop or an explosion appends all of them) in the
 * placement's appear direction. Once the column is full the box turns into an empty block. When
 * a disappear time is set, the children blink and vanish after the timer runs out.
 */
class GeneratorBox : public al::LiveActor {
public:
    /**
     * @brief The direction the children are stacked in ("AppearDirection" placement arg).
     */
    enum class AppearDirection : s32 { Up, Side, Front, SideReverse };

    explicit GeneratorBox(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    GeneratorBoxChild* getChild(s32 index) const;
    void updateChildPos();
    void offTimer();
    void onTimer();
    void calcAppearDirection(sead::Vector3f* pDir);
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;
    void kill() override;
    void control() override;
    void disappearSignAllChild();
    bool appendChild();
    bool isAliveAllAppearChild(s32* pDeadIndex) const;
    void appendChildAll();
    void disappearAllChild();
    void boundAllChild();
    bool isNerveEnableAppear() const;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void updateSensorFollowPosOffset(const GeneratorBoxChild* pChild, const sead::Vector3f& rPos,
                                     const sead::Vector3f& rBasePos);
    void exeWait();
    void exeEmpty();
    void exeReaction();
    void exeAppearWait();
    void exeDisappear();
    bool isBlinkingTime() const;
    void removeChild(GeneratorBoxChild* pChild);
    ~GeneratorBox() override;

    /**
     * @brief Get the number of children that are currently stacked on the box.
     * @return The number of appeared children.
     */
    s32 getChildCount() const { return mAppearChildNum; }

    /**
     * @brief Check whether the children are not stacked upwards (a hip drop on the top child is
     * then not passed on to the box).
     * @return True if the appear direction is not up.
     */
    bool isReacting() const { return mAppearDirection != AppearDirection::Up; }

private:
    GeneratorBoxChild* mChildren[20];                           // 0x148
    s32 mChildNum = 10;                                         // 0x1e8
    s32 mAppearChildNum = 0;                                    // 0x1ec
    s32 mDisappearTimer = 0;                                    // 0x1f0
    s32 mToDisappearTime = 600;                                 // 0x1f4
    f32 mChildInterval = 100.0f;                                // 0x1f8
    AppearDirection mAppearDirection = AppearDirection::Up;     // 0x1fc
    sead::Vector3f mAppearDir = sead::Vector3f::ey;             // 0x200
    bool mIsTimerOff = false;                                   // 0x20c
    BlockEmpty* mBlockEmpty = nullptr;                          // 0x210
    s32 mReactionStep = 0;                                      // 0x218
    s32 mControlUserId = -1;                                    // 0x21c
    al::ComboCounter* mComboCounter;                            // 0x220
    bool mIs2x2 = false;                                        // 0x228
    sead::Vector3f mPushSensorOffset = sead::Vector3f::zero;    // 0x22c
    f32 mPushLength = 0.0f;                                     // 0x238
};

static_assert(sizeof(GeneratorBox) == 0x240);
