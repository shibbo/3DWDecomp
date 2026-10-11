#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Util/AreaObjUtil.hpp"

RouteDokanInOutEffect::RouteDokanInOutEffect(const char* pName) : al::LiveActor(pName) {}
RouteDokanInOutEffect::~RouteDokanInOutEffect() {}

void RouteDokanInOutEffect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, "RouteDokanInOut", nullptr);
    makeActorDead();
}

void RouteDokanInOutEffect::startIn(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection) {
    start(rPosition, rDirection, "ルート土管イン");
    appear();
}

void RouteDokanInOutEffect::start(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection, const char* pReaction) {
    if (rc::isInWaterArea(this, rPosition))
        al::setMaterialCode(this, "InWater");
    else
        al::setMaterialCode(this, "");
    sead::Matrix34f matrix;
    al::makeMtxFrontNoSupportPos(&matrix, rDirection, rPosition);
    al::updatePoseMtx(this, &matrix);
    al::startHitReaction(this, pReaction);
}

void RouteDokanInOutEffect::startOut(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection) {
    start(rPosition, rDirection, "ルート土管アウト");
    kill();
}
