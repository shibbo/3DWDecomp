#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
namespace al { class PadRumbleKeeper; }
namespace sead { class IDelegate; }
class HeadgearPoseBuilder;
class HeadgearStateBlow;
struct HeadgearStateBlowParam;
struct BoxCoinReleaseParam {
    BoxCoinReleaseParam(const char* name, int level, float height, bool direction);
    bool isEqualActionName(const char*) const;
    int getReleaseStep() const;
    const char* mAction;
    int mLevel;
    float mHeight;
    bool mDirection;
};
class BoxCoin : public al::LiveActor {
public:
    explicit BoxCoin(const char*);
    ~BoxCoin() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void applyVisDark();
    void appearHipDrop(bool);
    void disappear(bool);
    bool isCarried() const;
    bool tryShiftCarry(al::HitSensor*, al::HitSensor*);
    bool updateCarryPose();
    void exeAppear();
    void exeWait();
    void exeAttach();
    void exeCarry();
    void exeBlow();
private:
    HeadgearPoseBuilder* mPoseBuilder = nullptr;
    const BoxCoinReleaseParam* mReleaseParam = nullptr;
    al::LiveActor* mItem;
    HeadgearStateBlow* mBlowState = nullptr;
    HeadgearStateBlowParam* mBlowParam;
    int mCoinCount = 0;
    sead::Vector3f mReleaseDir{0.0f, 0.0f, 1.0f};
    bool mHipDrop = false;
    bool mFloorTouch = false;
    bool mSuperJump = false;
    bool mRouteJump = false;
    int mUnused = 0;
    al::PadRumbleKeeper* mRumble = nullptr;
    sead::IDelegate* mDelegate = nullptr;
    bool mSingle = false;
    bool mDrawRestored = false;
    al::ModelDrawerArray mDrawers;
};
static_assert(sizeof(BoxCoin) == 0x1b0);
