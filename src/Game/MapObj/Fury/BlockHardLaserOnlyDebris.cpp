#include "MapObj/Fury/BlockHardLaserOnlyDebris.hpp"
#include "MapObj/Fury/BlockHardLaserOnly.hpp"
#include "MapObj/Fury/DisasterBlockDirector.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include <new>
BlockHardLaserOnlyDebris::BlockHardLaserOnlyDebris(const char* name, BlockHardLaserOnly* parent) : al::LiveActor(name), mParent(parent) {}
BlockHardLaserOnlyDebris::~BlockHardLaserOnlyDebris() {
    if (mConnector) {
        mConnector->al::MtxConnector::~MtxConnector();
        ::operator delete(mConnector);
        mConnector = nullptr;
    }
}
void BlockHardLaserOnlyDebris::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, getName(), nullptr);
    mConnector = al::tryCreateMtxConnector(this, info);
    makeActorAppeared();
}
void BlockHardLaserOnlyDebris::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
}
void BlockHardLaserOnlyDebris::tryAppear() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller) {
        auto* director = controller->getBlockDirector();
        if (director) {
            sead::Vector3f average(sead::Vector3f::zero);
            director->averageDisasterBlockPosition(average, mParent->getFileID(), false);
            if (al::getTrans(mParent).y - average.y > 0.01f) return;
        }
    }
    float angle = al::getRandom(0, 4) * 90;
    al::setRotateY(this, al::getRotate(this).y + angle);
    appear();
}
void BlockHardLaserOnlyDebris::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    al::LiveActor::control();
}
