#include "MapObj/StepScrollParts.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/Function/alModelFunction.hpp"
#include <math/seadBoundBox.h>

StepScrollParts::StepScrollParts(const char* pName) : al::LiveActor(pName) {}
StepScrollParts::~StepScrollParts() {}

void StepScrollParts::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    float radius = al::getClippingRadius(this);
    sead::BoundBox3f bounds;
    alModelFunction::calcBoundingBox(&bounds, getModelKeeper()->getModelCafe());
    mClippingCenter = bounds.getCenter();
    mClippingRadius = radius;
    al::setClippingInfo(this, radius, &mClippingCenter);
    mLocalClippingCenter = mClippingCenter;
    makeActorAppeared();
}

void StepScrollParts::control() {
    mClippingCenter = al::getTrans(this) + mLocalClippingCenter;
}

bool StepScrollParts::isOverBound(const sead::Vector3f& direction, const sead::Vector3f& origin) {
    return (al::getTrans(this) - origin).dot(direction) < 0.0f;
}

void StepScrollParts::scroll(const sead::Vector3f& direction, float distance) {
    al::setTrans(this, al::getTrans(this) - direction * distance);
}
