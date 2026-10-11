#include "MapObj/BlockStateCoinTen.hpp"
#include "MapObj/BoxCoin.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(BlockStateCoinTen, Wait);
    NERVE_DECL(BlockStateCoinTen, AppearCoin);
    NERVE_DECL(BlockStateCoinTen, AppearCoinWait);
    NERVES_MAKE_NOSTRUCT(BlockStateCoinTen, Wait, AppearCoin, AppearCoinWait)
}
BlockStateCoinTen::BlockStateCoinTen(al::LiveActor* host, const al::ActorInitInfo& info, bool createBox)
    : al::ActorStateBase("10コインステート", host) {
    if (createBox) {
        mBoxCoin = new BoxCoin("コインボックス");
        al::initCreateActorNoPlacementInfo(mBoxCoin, info);
        bool dark = false;
        bool hasDark = al::tryGetArg(&dark, info, "IsPlacedDarkStageBoxCoin");
        if (dark && hasDark) mBoxCoin->applyVisDark();
    }
    initNerve(&NrvBlockStateCoinTenWait, 0);
}
void BlockStateCoinTen::appear() {
    al::NerveStateBase::appear();
    mElapsedFrames = 0;
    mCoinCount = 0;
    al::setNerve(this, &NrvBlockStateCoinTenWait);
}
bool BlockStateCoinTen::receiveMsg(const al::SensorMsg* msg) {
    if (!isValidAppearCoin()) return false;
    mStatueDrop = false;
    if (msg && al::isExistAction(mHostActor, "ReactionHipDrop") && (al::isMsgPlayerHipDropAll(msg) || al::isMsgPlayerStatueDrop(msg))) {
        al::startAction(mHostActor, "ReactionHipDrop");
        if (al::isMsgPlayerStatueDrop(msg)) mStatueDrop = true;
    } else al::startAction(mHostActor, "Reaction");
    mReactionFrames = rc::getReactionCountByMsg(msg);
    appearCoin();
    al::setNerve(this, &NrvBlockStateCoinTenAppearCoin);
    return true;
}
bool BlockStateCoinTen::isValidAppearCoin() const {
    if (!al::isNerve(this, &NrvBlockStateCoinTenAppearCoin)) return true;
    if (!al::isGreaterStep(this, mReactionFrames)) return false;
    return !isTimerEndOrCoinMax();
}
void BlockStateCoinTen::appearCoin() {
    if (mAppearCoinCallback) {
        (*mAppearCoinCallback)();
        ++mCoinCount;
    }
}
void BlockStateCoinTen::setAppearCoinCallBack(const al::FunctorBase& callback) { mAppearCoinCallback = callback.clone(); }
bool BlockStateCoinTen::isValidUpperPunch(int frame) const { return al::isNerve(this, &NrvBlockStateCoinTenAppearCoin) && al::isLessStep(this, frame); }
bool BlockStateCoinTen::isAppearBoxCoin() const { return mBoxCoin && isAppearCoinMax(); }
bool BlockStateCoinTen::isAppearCoinMax() const { return mMaxCoins <= mCoinCount; }
BoxCoin* BlockStateCoinTen::getBoxCoin() const { return mBoxCoin; }
bool BlockStateCoinTen::isTimerEndOrCoinMax() const {
    if (mMaxCoins < 0) return false;
    if (mElapsedFrames > 180) return true;
    return isAppearCoinMax();
}
void BlockStateCoinTen::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "Wait");
}
void BlockStateCoinTen::exeAppearCoin() {
    ++mElapsedFrames;
    if (al::isActionEnd(mHostActor)) {
        if (isTimerEndOrCoinMax()) {
            if (isAppearBoxCoin()) {
                al::copyPose(mBoxCoin, mHostActor);
                if (al::isActionPlaying(mHostActor, "ReactionHipDrop")) mBoxCoin->appearHipDrop(mStatueDrop);
                else mBoxCoin->appear();
            }
            kill();
        } else if (al::isGreaterStep(this, mReactionFrames)) al::setNerve(this, &NrvBlockStateCoinTenAppearCoinWait);
    }
}
void BlockStateCoinTen::exeAppearCoinWait() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "Wait");
    ++mElapsedFrames;
}
