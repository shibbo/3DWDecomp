#pragma once

#include "Project/Block/BlockRailParts.hpp"
#include <math/seadQuat.h>
#include <prim/seadSafeString.h>

namespace al { class BlockRailRider; }
class CourseSelectDirector;
class RouteDokanController;
class CourseSelectSensor;

class CourseSelectRouteDokanEntrance : public al::BlockRailParts {
public:
    CourseSelectRouteDokanEntrance(const char* pName, const char* pSuffix);
    ~CourseSelectRouteDokanEntrance() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void createStopper(const al::ActorInitInfo& rInfo);
    void setDisableInput();
    void appearStopper(bool isEffect);
    void setInitQT(const sead::Quatf& rQuat, const sead::Vector3f& rTrans);
    void startRouteDokanRider(al::BlockRailRider* pRider) const;
    bool tryDecide(const CourseSelectDirector* pDirector);
    void setEnableInput();

private:
    CourseSelectDirector* mpDirector = nullptr;
    RouteDokanController* mpController = nullptr;
    CourseSelectSensor* mpSensor = nullptr;
    sead::Quatf mInitQuat = sead::Quatf::unit;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    al::LiveActor* mpStopper = nullptr;
    bool mIsEnableInput = true;
    sead::FixedSafeString<64> mArchiveCategory;
};

static_assert(sizeof(CourseSelectRouteDokanEntrance) == 0x220);
