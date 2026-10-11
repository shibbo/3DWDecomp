#include "MapObj/TestAndoDashPanel.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

TestAndoDashPanel::TestAndoDashPanel(const char* pName) : al::LiveActor(pName) {}

TestAndoDashPanel::~TestAndoDashPanel() {}

void TestAndoDashPanel::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    makeActorAppeared();
}

void TestAndoDashPanel::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther) && rc::isPlayerOnGround(pOther))
        rc::sendMsgDashPanel(pOther, pSelf, 180);
}
