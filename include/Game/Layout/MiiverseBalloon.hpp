#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>
namespace al { class LayoutInitInfo; class NetworkSystem; class FunctorBase; }
class CourseSelectDirector;
class ICourseSelectActorController;
class CourseSelectInfoDecideButton;
class MiiverseBalloonController;
/** @brief Positions a Miiverse balloon above the selected world-map actor. */
class MiiverseBalloon : public al::LayoutActor {
public:
    MiiverseBalloon(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector, const al::NetworkSystem* pNetwork);
    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }
    void control() override;
    void decompress();
    void setTouchSelect(bool isTouch);
    void startAppear(ICourseSelectActorController* pController);
    void startDecide();
    void startDisappear();
    bool isAppear() const;
    bool isDisappear() const;
    bool isEnableDecide() const;
    bool isTouchSelect() const { return mIsTouchSelect; }
    void exeDecomp();
    void exeAppear();
    void exeWait();
    void exeDecide();
    void exeDisappear();
private:
    CourseSelectDirector* mDirector;
    ICourseSelectActorController* mActorController = nullptr;
    al::SceneCameraInfo* mSceneCameraInfo;
    MiiverseBalloonController* mController = nullptr;
    const sead::Vector3f* mActorTrans = nullptr;
    CourseSelectInfoDecideButton* mDecideButton = nullptr;
    bool mIsTouchSelect = false;
    al::FunctorBase* mDecompressFunctor = nullptr;
    volatile bool mIsDecompressed = false;
    const al::NetworkSystem* mNetwork;
};
static_assert(sizeof(MiiverseBalloon) == 0x178);
