#include "MapObj/LuckyIsland.hpp"
#include "MapObj/LuckyIslandController.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/LiveActorFlag.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
NERVE_DECL(LuckyIsland, Descend);
NERVE_DECL(LuckyIsland, Wait);
NERVE_DECL(LuckyIsland, DemoIntro);
NERVE_DECL(LuckyIsland, Fade);
const struct {
    LuckyIslandNrvDescend Descend;
    LuckyIslandNrvWait Wait;
    LuckyIslandNrvDemoIntro DemoIntro;
    LuckyIslandNrvFade Fade;
} NrvLuckyIsland;
}
LuckyIsland::LuckyIsland(const char* name) : al::LiveActor(name) {}
LuckyIsland::~LuckyIsland() {}
void LuckyIsland::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvLuckyIsland.Wait, 0);
    initAnimPart(this, info);
    mObjects.init(info, true);
    al::getSceneObj<LuckyIslandHolder>(this, 38)->registerIsland(this);
    ProjectActorFactory factory;
    int count = al::calcLinkChildNum(info, "AppearIfCollectedObj");
    if (count > 0) {
        mCollectedObjects.allocBuffer(count, nullptr);
        for (int i = 0; i < count; ++i) {
            al::PlacementInfo placed;
            al::getLinksInfoByIndex(&placed, info, "AppearIfCollectedObj", i);
            al::PlacementId id;
            al::getPlacementId(&id, placed);
            for (int j = 0; j < mObjects.getObjectNum(); ++j) {
                al::PlacementInfo attached;
                al::PlacementId attachedId;
                al::getLinksInfoByIndex(&attached, info, "AttachObject", j);
                al::getPlacementId(&attachedId, attached);
                if (al::isEqualPlacementID(id, attachedId)) {
                    mCollectedObjects.pushBack(mObjects.getActor(j));
                    break;
                }
            }
            if (i == getCollectedObjectNum()) {
                al::PlacementInfo child;
                al::ActorInitInfo childInfo;
                al::getLinksInfoByIndex(&child, info, "AppearIfCollectedObj", i);
                childInfo.initViewIdSelf(&child, info);
                auto* actor = al::createLinksActorFromFactory(factory, info, "AppearIfCollectedObj", i);
                mCollectedObjects.pushBack(actor);
                actor->makeActorDead();
            }
        }
    }
    mCameraAt = al::getCameraAt_RS(this, 0);
    mCameraPos = al::getCameraPos_RS(this, 0);
    mCamera = al::initProgramableCamera_RS(this, info, "LuckyIslandDemoIntro", &mCameraPos, &mCameraAt, nullptr);
    mSkip = new DemoSkipLayout(al::getLayoutInitInfo(info), true);
    mWipe = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack", al::getLayoutInitInfo(info), "Demo");
    _142 = true;
    al::setEffectFollowPosPtr(this, "Wait", al::getTransPtr(this));
    makeActorAppeared();
    al::hideModelIfShow(this);
}
void LuckyIsland::initAfterPlacement() {
    mHome = al::getTrans(this);
    initAnimPartAfterPlacement(this);
    sead::Vector3f pos(mHome.x, mHome.y, mHome.z);
    UpdatePos(pos);
    al::invalidateClipping(this);
}
void LuckyIsland::UpdatePos(const sead::Vector3f& pos) {
    al::setTrans(this, pos);
    mObjects.syncObjectsToPosition(pos);
}
void LuckyIsland::control() {
    if (mInitialHide) {
        al::validateClipping(this);
        al::showModelIfHide(this);
        kill();
        for (int i = 0; i < mObjects.getObjectNum(); ++i) mObjects.getActor(i)->kill();
        mInitialHide = false;
    }
    if (!mFarLodActor->mActorFlags->isClipped)
        for (int i = 0; i < getEffectKeeper()->getEffectNum(); ++i) getEffectKeeper()->getEffect(i)->update();
}
void LuckyIsland::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        for (int i = 0; i < mObjects.getObjectNum(); ++i) al::validateClipping(mObjects.getActor(i));
        for (int i = 0; i < mCollectedObjects.size(); ++i) al::validateClipping(mCollectedObjects.at(i));
    }
}
void LuckyIsland::exeDescend() {
    if (al::isFirstStep(this)) {
        mSkip->kill();
        al::startAction(this, "Wait");
    }
    if (!al::isEffectEmitting(this, "Wait")) al::tryEmitEffect(this, "Wait", nullptr);
    const auto& trans = al::getTrans(this);
    sead::Vector3f pos(trans.x, trans.y + -10000.0f / 480.0f, trans.z);
    UpdatePos(pos);
    if (al::isGreaterEqualStep(this, 480)) {
        al::setNerve(this, &NrvLuckyIsland.Wait);
        al::validateClipping(this);
    }
}
void LuckyIsland::endDemo() {
    if (mInDisasterDemo) {
        auto* controller = DisasterModeController::tryGetController(this);
        if (!controller) return;
        if (controller->setPostCutsceneDisasterFreezeTime()) {
            rc::addDemoActor(DisasterModeController::tryGetController(this));
            return;
        }
    }
    rc::requestEndDemoInGameCutscene(this);
}
void LuckyIsland::exeDemoIntro() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Intro");
        mDemoRequested = false;
        mInDisasterDemo = rc::isActiveDemoInGameCutscene(this);
        if (mInDisasterDemo) rc::changeActiveDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
        else rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
        if (!mInDisasterDemo && !rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvLuckyIsland.DemoIntro);
            return;
        }
        for (int i = 0; i < mObjects.getObjectNum(); ++i) {
            al::invalidateClipping(mObjects.getActor(i));
            rc::addDemoActor(mObjects.getActor(i));
        }
        mCameraAt = al::getTrans(this);
        sead::Vector3f dir = mCameraDir;
        sead::Matrix33f rotation;
        rotation.makeR(sead::Vector3f(0.0f, sead::Mathf::deg2rad(10.0f), 0.0f));
        dir = rotation * dir;
        mCameraPos = mCameraDistance * dir + mCameraAt;
        if (!mInDisasterDemo) rc::addDemoActor(this);
        al::startCamera_RS(this, mCamera, 0);
        mSkip->appear();
    }
    if (al::getCurPlayingBgmPlayName(this)) al::changeBgmVolume(this, 0.5f, 30);
    if (al::isGreaterEqualStep(this, 0) && al::isLessEqualStep(this, 210)) {
        sead::Vector3f dir = mCameraDir;
        float rate = al::easeByType(al::getNerveStep(this) / 210.0f, 2);
        sead::Quatf rotation;
        rotation.setAxisRadian(sead::Vector3f(0.0f, 1.0f, 0.0f), sead::Mathf::deg2rad(rate * 0.0f + (1.0f - rate) * 10.0f));
        sead::Matrix33f matrix;
        // Preserve the original quaternion-to-matrix evaluation order.
        const float yy = 2 * rotation.y * rotation.y;
        const float zz = 2 * rotation.z * rotation.z;
        const float xx = 2 * rotation.x * rotation.x;
        const float xy = 2 * rotation.x * rotation.y;
        const float xz = 2 * rotation.x * rotation.z;
        const float yz = 2 * rotation.y * rotation.z;
        const float wz = 2 * rotation.w * rotation.z;
        const float wx = 2 * rotation.w * rotation.x;
        const float wy = (rotation.w + rotation.w) * rotation.y;

        matrix.m[0][0] = 1 - yy - zz;
        matrix.m[0][1] = xy - wz;
        matrix.m[0][2] = xz + wy;

        matrix.m[1][0] = xy + wz;
        matrix.m[1][1] = 1 - xx - zz;
        matrix.m[1][2] = yz - wx;

        matrix.m[2][0] = xz - wy;
        matrix.m[2][1] = wx + yz;
        matrix.m[2][2] = 1 - xx - yy;
        dir = matrix * dir;
        float distance = mCameraDistance + (rate * 400.0f + (1.0f - rate) * 4500.0f);
        mCameraPos = mCameraAt + distance * dir;
    }
    auto ports = sead::BitFlag<u16>(rc::getActiveInputPortList(GameDataHolderAccessor(this)));
    if (mSkip->isSkip(ports)) {
        mSkip->kill();
        al::changeBgmVolume(this, 1.0f, 30);
        al::setNerve(this, &NrvLuckyIsland.Fade);
    } else if (al::isGreaterEqualStep(this, 240)) {
        al::requestCaptureScreenCover(this, 2);
        al::endCamera_RS(this, mCamera, 0, false);
        al::changeBgmVolume(this, 1.0f, 30);
        endDemo();
        al::validateClipping(this);
        al::setNerve(this, &NrvLuckyIsland.Wait);
    }
}
void LuckyIsland::exeFade() {
    if (al::isFirstStep(this)) mWipe->startClose(-1);
    if (mWipe->isCloseEnd()) {
        al::requestCaptureScreenCover(this, 2);
        mWipe->startOpen(-1);
        endDemo();
        al::endCamera_RS(this, mCamera, 0, false);
        al::validateClipping(this);
        al::setNerve(this, &NrvLuckyIsland.Wait);
    }
}
void LuckyIsland::setController(LuckyIslandController* controller) { mController = controller; }
void LuckyIsland::DisasterAppear() {
    for (int i = 0; i < mObjects.getObjectNum(); ++i) {
        mObjects.getActor(i)->appear();
        al::validateHitSensors(mObjects.getActor(i));
    }
    if (SingleModeDataFunction::getNumRemainingLuckyShines(GameDataHolderAccessor(this))) {
        for (int i = 0; i < mCollectedObjects.size(); ++i) mCollectedObjects.unsafeAt(i)->kill();
    } else {
        for (int i = 0; i < mCollectedObjects.size(); ++i) {
            al::invalidateClipping(mCollectedObjects.at(i));
            mCollectedObjects.unsafeAt(i)->appear();
        }
    }
    appear();
    al::invalidateClipping(this);
    if (mDemoRequested) StartDemo();
    else {
        al::setNerve(this, &NrvLuckyIsland.Descend);
        UpdatePos(mHome + sead::Vector3f(0.0f, 10000.0f, 0.0f));
    }
}
void LuckyIsland::StartDemo() {
    if (al::isDead(this)) appear();
    UpdatePos(mHome);
    if (mController) {
        mCameraPos = *mController->getClosestCamera(al::getTrans(this));
        mCameraDistance = (mCameraPos - al::getTrans(this)).length();
        const auto& trans = al::getTrans(this);
        mCameraDir = (mCameraPos - trans) * (1.0f / mCameraDistance);
    }
    mDemoRequested = true;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvLuckyIsland.DemoIntro);
}
void LuckyIsland::DisasterDisappear() {
    endFarLod();
    for (int i = 0; i < mObjects.getObjectNum(); ++i)
        if (!al::isDead(mObjects.getActor(i))) mObjects.getActor(i)->kill();
    for (int i = 0; i < mCollectedObjects.size(); ++i)
        if (!al::isDead(mCollectedObjects.at(i))) mCollectedObjects.unsafeAt(i)->kill();
    if (!al::isDead(this)) kill();
}
void LuckyIsland::disableCollision() {
    for (int i = 0; i < mObjects.getObjectNum(); ++i) al::invalidateHitSensors(mObjects.getActor(i));
}
void LuckyIsland::onDisasterModeStateChange(DisasterModeController::State state) {
    startDisasterModeAnim(this, state);
}
