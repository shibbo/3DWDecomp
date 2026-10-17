#include "CourseSelect/CoinChameleonCourseSelect.hpp"
#include "CourseSelect/CoinCourseSelect.hpp"
#include "MapObj/ChameleonStateMic.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/CourseSelectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "math/seadMatrix.hpp"

namespace {
NERVE_DECL(CoinChameleonCourseSelect, Wait);
NERVE_DECL(CoinChameleonCourseSelect, MicReaction);
NERVE_DECL(CoinChameleonCourseSelect, ContactAppear);
NERVE_DECL(CoinChameleonCourseSelect, Interpolate);
NERVES_MAKE_STRUCT(CoinChameleonCourseSelect, Wait, MicReaction, ContactAppear, Interpolate)
const struct { sead::Vector4f absorb, reflection; } sReveal = {{1,1,1,1}, {1,1,1,0.4f}};
const struct { sead::Vector4f absorb, reflection, light; } sInitial = {{1,1,1,1}, {1,1,0,0}, {1,1,1,1}};
const RenderMaterialIndirectParam sWaitParam = {0.06f, 0.0f, {1,1,1,1}, {1,1,0,0}, {1,1,1,1}};
const RenderMaterialIndirectParam sMicParam = {0.2f, 10.0f, {1,1,1,1}, {1,1,1,0.2f}, {1,1,1,1}};
}

/** @brief Create a hidden coin that reacts to contact and microphone input. */
CoinChameleonCourseSelect::CoinChameleonCourseSelect(const char* pName) : al::LiveActor(pName) {}
CoinChameleonCourseSelect::~CoinChameleonCourseSelect() = default;

/** @brief Initialize the transparent material and its visible replacement coin. */
void CoinChameleonCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "CoinChameleon", "CourseSelect");
    al::setMaterialProgrammable(this);
    mpIndirect = new RenderMaterialIndirectParam{0.06f, 0.0f, sInitial.absorb, sInitial.reflection, sInitial.light};
    al::initNerve(this, &NrvCoinChameleonCourseSelect.Wait, 1);
    mpMicState = new ChameleonStateMic(this, mpIndirect, &sMicParam);
    al::initNerveState(this, mpMicState, &NrvCoinChameleonCourseSelect.MicReaction, "[state]カメレオンマイク反応");
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    mBaseQuat.set(al::getQuat(this));
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0,0,1)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0,0,1));
    mpCoin = new CoinCourseSelect("コース選択コイン[コース選択透明コイン出現後]");
    al::initCreateActorWithPlacementInfo(mpCoin, rInfo);
    mpCoin->getName();
    al::setTrans(mpCoin, al::getTrans(this));
    mpCoin->makeActorDead();
    mObjectId = mpCoin->getObjectId();
    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        makeActorDead();
    } else {
        mBaseTrans.set(al::getTrans(this));
        makeActorAppeared();
    }
}

/** @brief Apply the current transparency and shared coin rotation. */
void CoinChameleonCourseSelect::control() {
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mpIndirect);
    al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
}

/** @brief Restore the hidden coin only while it remains uncollected. */
void CoinChameleonCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId))
        al::LiveActor::appear();
}

/** @brief Start the contact reveal unless it is already underway. */
bool CoinChameleonCourseSelect::tryContactAppear(al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvCoinChameleonCourseSelect.ContactAppear)) return false;
    if (al::isNerve(this, &NrvCoinChameleonCourseSelect.Interpolate)) return false;
    al::invalidateHitSensor(this, "ContactAppear");
    al::setNerve(this, &NrvCoinChameleonCourseSelect.ContactAppear);
    return true;
}

/** @brief Test whether contact has started revealing the coin. */
bool CoinChameleonCourseSelect::isNerveAppear() {
    return al::isNerve(this, &NrvCoinChameleonCourseSelect.ContactAppear) || al::isNerve(this, &NrvCoinChameleonCourseSelect.Interpolate);
}

/** @brief Reveal the hidden coin when a player touches its contact sensor. */
void CoinChameleonCourseSelect::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (!isNerveAppear() && al::isSensorName(pSender, "ContactAppear") && al::isSensorPlayer(pReceiver))
        tryContactAppear(pReceiver);
}

/** @brief Award and persist collection once the reveal animation allows it. */
bool CoinChameleonCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (!al::isSensorName(pReceiver, "Get")) return false;
    if ((al::isNerve(this, &NrvCoinChameleonCourseSelect.ContactAppear) && al::isGreaterStep(this, 19)) ||
        (al::isNerve(this, &NrvCoinChameleonCourseSelect.Interpolate) && al::getNerveStep(this) >= 15)) {
        if (rc::isMsgCoinGet(pMsg)) {
            rc::acquirerItemCoin(this, pSender);
            rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
            al::startHitReactionGet(this);
            kill();
            return true;
        }
    }
    return false;
}

/** @brief Return toward the hidden material, or react to microphone input. */
void CoinChameleonCourseSelect::exeWait() {
    if (al::isMicInputOn(this)) al::setNerve(this, &NrvCoinChameleonCourseSelect.MicReaction);
    else ChameleonStateUtil::updateIndirectParam(mpIndirect, &sWaitParam);
}

/** @brief Run the microphone material reaction before returning to waiting. */
void CoinChameleonCourseSelect::exeMicReaction() {
    al::updateNerveStateAndNextNerve(this, &NrvCoinChameleonCourseSelect.Wait);
}

/** @brief Launch the coin upward and accelerate its rotation as it becomes visible. */
void CoinChameleonCourseSelect::exeContactAppear() {
    ChameleonStateUtil::updateIndirectParam(mpIndirect, 0.05f, 2.0f, sReveal.absorb, sReveal.reflection, sead::Vector4f::ones);
    if (al::isFirstStep(this)) {
        al::validateHitSensor(this, "Get");
        al::setVelocity(this, sead::Vector3f(0,19,0));
        mRotate = rc::getCoinRotateY(this);
        al::startSe(this, "PgTransparentAppear", nullptr);
    }
    updateVelocity();
    rotate(rc::getCoinRotateYByFrame(this) * 5.0f);
    if (al::isGreaterStep(this, 5)) al::setNerve(this, &NrvCoinChameleonCourseSelect.Interpolate);
}

/** @brief Integrate the reveal jump without allowing a negative height. */
void CoinChameleonCourseSelect::updateVelocity() {
    al::getVelocityPtr(this)->y -= 1.2f;
    mHeight = sead::Mathf::max(0.0f, al::getVelocity(this).y + mHeight);
}

/** @brief Advance the reveal rotation by the supplied angle. */
void CoinChameleonCourseSelect::rotate(float delta) {
    mRotate += delta;
    setRotate(mRotate);
}

/** @brief Settle the reveal rotation and exchange this actor for the normal coin. */
void CoinChameleonCourseSelect::exeInterpolate() {
    ChameleonStateUtil::updateIndirectParam(mpIndirect, 0.05f, 2.0f, sReveal.absorb, sReveal.reflection, sead::Vector4f::ones);
    if (al::isFirstStep(this)) {
        mTargetRotate = rc::getCoinRotateY(this) + rc::getCoinRotateYByFrame(this) * 13.0f;
        mTargetRotate = al::modf(mTargetRotate + 720.0f - 1.0f + 360.0f, 360.0f) + 1.0f;
        mRotate = al::modf(mRotate + 720.0f - 1.0f + 360.0f, 360.0f) + 1.0f;
        float diff = mTargetRotate - mRotate;
        if (diff < 0.0f) mTargetRotate += (static_cast<u32>(diff / -180.0f) + 1) * 180.0f;
        else if (diff < 180.0f) mTargetRotate += 180.0f;
        mRotateFactor = al::calcMultValueToDestination(13, mRotate, mTargetRotate);
    }
    updateVelocity();
    if (al::isLessStep(this, 13)) {
        mRotate *= mRotateFactor;
        setRotate(mRotate);
    }
    if (al::isGreaterEqualStep(this, 13)) {
        setRotate(rc::getCoinRotateY(this));
        if (mHeight == 0.0f) {
            kill();
            al::hideModelIfShow(this);
            al::setTrans(mpCoin, al::getTrans(this));
            al::copyPose(mpCoin, this);
            mpCoin->makeActorAppeared();
            al::startHitReaction(this, "可視");
        }
    }
}

/** @brief Rotate about the placement axis and apply the reveal jump offset. */
void CoinChameleonCourseSelect::setRotate(float angle) {
    sead::Quatf quat;
    al::rotateQuatYDirDegree(&quat, mBaseQuat, angle);
    sead::Matrix34f matrix;
    matrix.makeQT(quat, mBaseTrans);
    matrix.toQuat(*al::getQuatPtr(this));
    matrix.getTranslation(*al::getTransPtr(this));
    if (mHeight > 0.0f) {
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        al::setTrans(this, al::getTrans(this) + up * mHeight);
    }
}
