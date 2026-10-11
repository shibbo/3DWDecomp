#include "MapObj/Fury/GigaBellPedestal.hpp"
#include "MapObj/Fury/GigaBell.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
GigaBellPedestal::GigaBellPedestal(const char* name) : al::LiveActor(name) {}
GigaBellPedestal::~GigaBellPedestal() {}
void GigaBellPedestal::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "GigaBellPedestal", nullptr);
    const char* modelName = nullptr;
    alPlacementFunction::getModelName(&modelName, info);
    if (al::isEqualString(modelName, "GigaBellPedestal01")) setColor(0);
    if (al::isEqualString(modelName, "GigaBellPedestal02")) setColor(1);
    if (al::isEqualString(modelName, "GigaBellPedestal03")) setColor(2);
    makeActorAppeared();
}
void GigaBellPedestal::setColor(int color) {
    al::startMtpAnimAndSetFrameAndStop(this, "Color", color);
    al::startMclAnimAndSetFrameAndStop(this, "Color", color);
}
void GigaBellPedestal::initAfterPlacement() {
    const sead::Matrix34f* mtx = al::getJointMtxPtr(this, "GigaBell_Pos");
    auto* manager = al::tryGetSceneObj<GigaBellManager>(this, 50);
    if (manager) {
        GigaBell* bell = manager->getGigaBellClosestTo(mtx->getTranslation());
        al::setTrans(bell, mtx->getTranslation());
        bell->setBasePosition(mtx->getTranslation());
        bell->setPedestal(this);
    }
}
