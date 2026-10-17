#include "MapObj/ChikaChikaBlock.hpp"
#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
    class ChikaChikaBlockNrvSwitchOnFirst : public al::Nerve {
    public:
        void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<ChikaChikaBlock>()->exeSwitchOn(); }
    };
    NERVE_DECL(ChikaChikaBlock, SwitchOff);
    NERVE_DECL(ChikaChikaBlock, SwitchOn);
    NERVE_DECL(ChikaChikaBlock, SwitchOffSign);
    NERVE_DECL(ChikaChikaBlock, SyncBgm);
    NERVES_MAKE_NOSTRUCT(ChikaChikaBlock, SwitchOnFirst, SwitchOff, SwitchOn, SwitchOffSign, SyncBgm)
}
ChikaChikaBlock::ChikaChikaBlock(const char* pName) : al::LiveActor(pName) {}
ChikaChikaBlock::~ChikaChikaBlock() {}
void ChikaChikaBlock::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = nullptr;
    alPlacementFunction::tryGetModelName(&modelName, rInfo);
    if (al::isEqualString(modelName, "ChikaChikaBlockA") || al::isEqualString(modelName, "ChikaChikaBlockA5x1") ||
        al::isEqualString(modelName, "ChikaChikaBlockA5x2") || al::isEqualString(modelName, "ChikaChikaBoomerangSliderA"))
        mPhase = 0;
    else if (al::isEqualString(modelName, "ChikaChikaBlockB") || al::isEqualString(modelName, "ChikaChikaBlockB5x1") ||
             al::isEqualString(modelName, "ChikaChikaBlockB5x2") || al::isEqualString(modelName, "ChikaChikaBoomerangSliderB"))
        mPhase = 1;
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvChikaChikaBlockSwitchOnFirst, 0);
    makeActorAppeared();
    al::onDrawClipping(this);
    if (mPhase == 1) {
        al::setNerve(this, &NrvChikaChikaBlockSwitchOff);
        al::invalidateCollisionParts(this);
    }
}
void ChikaChikaBlock::startSwitchOffSign() {
    if (al::isNerve(this, &NrvChikaChikaBlockSwitchOn) || al::isNerve(this, &NrvChikaChikaBlockSwitchOnFirst))
        al::setNerve(this, &NrvChikaChikaBlockSwitchOffSign);
}
void ChikaChikaBlock::startSwitch() {
    if (al::isNerve(this, &NrvChikaChikaBlockSwitchOffSign))
        al::setNerve(this, &NrvChikaChikaBlockSwitchOff);
    else if (al::isNerve(this, &NrvChikaChikaBlockSwitchOff))
        al::setNerve(this, &NrvChikaChikaBlockSwitchOn);
}
void ChikaChikaBlock::startSwitchOn() {
    if (al::isNerve(this, &NrvChikaChikaBlockSwitchOffSign) || al::isNerve(this, &NrvChikaChikaBlockSwitchOff))
        al::setNerve(this, &NrvChikaChikaBlockSwitchOn);
}
void ChikaChikaBlock::exeSwitchOn() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
        if (al::isNerve(this, &NrvChikaChikaBlockSwitchOnFirst)) {
            if (mIsSpeedModeDisabled)
                al::startAction(this, "AppearSignQuickFirst");
            else
                al::startAction(this, "AppearSignFirst");
        } else {
            if (mIsSpeedModeDisabled)
                al::startAction(this, "AppearSignQuick");
            else
                al::startAction(this, "AppearSign");
        }
        al::validateCollisionParts(this);
    }
}
void ChikaChikaBlock::exeSwitchOffSign() {
    if (al::isFirstStep(this)) {
        if (mIsSpeedModeDisabled)
            al::startAction(this, "DisappearSignQuick");
        else
            al::startAction(this, "DisappearSign");
    }
    if (al::isActionEnd(this))
        al::invalidateCollisionParts(this);
}
void ChikaChikaBlock::exeSwitchOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
        al::killPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
    }
}
void ChikaChikaBlock::exeSyncBgm() {
    int frame = mSynchronizer->getFrame();
    if (mLastFrame == frame)
        return;
    int phase = mSynchronizer->getPhase();
    bool first = mSynchronizer->isFirstMeasure();
    if (frame < getSwitchTime()) {
        if (mPhase == phase) {
            if (first) {
                if (mIsSpeedModeDisabled)
                    al::startAction(this, "AppearSignQuickFirst");
                else
                    al::startAction(this, "AppearSignFirst");
            } else {
                if (mIsSpeedModeDisabled)
                    al::startAction(this, "AppearSignQuick");
                else
                    al::startAction(this, "AppearSign");
            }
            al::validateCollisionParts(this);
            al::appearPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
        } else {
            al::startAction(this, "Disappear");
            al::invalidateCollisionParts(this);
            al::killPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
        }
        al::setMclAnimFrame(this, frame);
        al::setMtsAnimFrame(this, frame);
    } else if (frame >= getSwitchTime()) {
        if (mPhase == phase) {
            if (mIsSpeedModeDisabled)
                al::startAction(this, "DisappearSignQuick");
            else
                al::startAction(this, "DisappearSign");
            al::validateCollisionParts(this);
            al::appearPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
        } else {
            al::startAction(this, "Disappear");
            al::invalidateCollisionParts(this);
            al::killPrePassLight(this, mPhase == 0 ? "BluePointLight" : "RedPointLight", -1);
        }
        al::setMclAnimFrame(this, frame - getSwitchTime());
        al::setMtsAnimFrame(this, frame - getSwitchTime());
    }
    mLastFrame = frame;
}
int ChikaChikaBlock::getSwitchTime() const {
    return al::getActionFrameMax(this, mIsSpeedModeDisabled ? "DisappearSignQuick" : "DisappearSign");
}
void ChikaChikaBlock::onBgmSync() {
    mIsBgmSync = true;
    al::setIgnoreUpdateDrawClipping(this, true);
    al::setNerve(this, &NrvChikaChikaBlockSyncBgm);
    mSynchronizer = al::getSceneObj<ChikaChikaBlockSynchronizer>(this, 29);
}
