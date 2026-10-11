#include "MapObj/KoopaChaseCar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {
NERVE_DECL(KoopaChaseCar, Wait);
NERVE_DECL(KoopaChaseCar, Reaction);
NERVES_MAKE_NOSTRUCT(KoopaChaseCar, Wait, Reaction)
}

KoopaChaseCar::KoopaChaseCar(const char* name) : al::LiveActor(name) {}
KoopaChaseCar::~KoopaChaseCar() {}

void KoopaChaseCar::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KoopaChaseLv2", "CarObj");
    al::initNerve(this, &NrvKoopaChaseCarWait, 0);
    int count = rc::getControlUserNumMax();
    mReactionTimers.tryAllocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i)
        mReactionTimers[i] = 0;
    makeActorAppeared();
}

bool KoopaChaseCar::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                               al::HitSensor* receiver) {
    if (al::isSensorEnemyBody(receiver)) {
        if (al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerSpinAttack(msg) ||
            al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerBodyAttack(msg)) {
            int id = rc::tryFindRelativeControlUserId(sender);
            if (mReactingPlayers.isOnBit(id) && mReactionTimers[id] > 0)
                return true;
            mReactingPlayers.setBit(id);
            mReactionTimers[id] = 40;
            al::setNerve(this, &NrvKoopaChaseCarReaction);
            return true;
        }
        if (al::isMsgPlayerBoomerangBreak(msg) || al::isMsgPlayerFireBallAttack(msg)) {
            al::setNerve(this, &NrvKoopaChaseCarReaction);
            return true;
        }
    } else {
        if (al::isMsgTouchAssistTrig(msg)) {
            al::setNerve(this, &NrvKoopaChaseCarReaction);
            return true;
        }
        if (al::isMsgPlayerRollingReflect(msg) || al::isMsgPlayerHipDropAll(msg))
            al::setNerve(this, &NrvKoopaChaseCarReaction);
    }
    return false;
}

void KoopaChaseCar::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "KoopaLastWait");
        mReactingPlayers.makeAllZero();
        if (!mIsPlayingMusic)
            al::changeBgmSituation(this, "CancelFocusSe");
    }
    if (mIsPlayingMusic && al::isStep(this, 58)) {
        al::changeBgmSituation(this, "CancelFocusMe");
        mIsPlayingMusic = false;
    }
}

void KoopaChaseCar::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "KoopaLastReaction");
    int count = mReactionTimers.size();
    for (int i = 0; i < count; ++i) {
        if (mReactingPlayers.isOnBit(i)) {
            --mReactionTimers[i];
            if (mReactionTimers[i] <= 0) {
                mReactionTimers[i] = 0;
                mReactingPlayers.resetBit(i);
            }
        }
    }
    if (al::isStep(this, 20)) {
        if (!mIsPlayingMusic)
            ++mReactionCount;
        if (mReactionCount >= 4) {
            float random = al::getRandom(0.0f, 1.0f);
            if (random <= 0.0078125f) {
                al::startSeWithParam(this, "pgCarStereo", 2.0f, nullptr);
                al::changeBgmSituation(this, "FocusMe");
                mIsPlayingMusic = true;
                mReactionCount = 0;
            } else if (random <= 0.0390625f) {
                al::startSeWithParam(this, "pgCarStereo", 1.0f, nullptr);
                al::changeBgmSituation(this, "FocusSe");
                mReactionCount = 0;
            } else if (random <= 0.5f) {
                al::startSeWithParam(this, "pgCarStereo", 0.0f, nullptr);
                if (mIsPlayingMusic)
                    al::changeBgmSituation(this, "CancelFocusMe");
                else
                    al::changeBgmSituation(this, "CancelFocusSe");
            } else {
                al::startSe(this, "LastReactionHorn", nullptr);
                if (mIsPlayingMusic)
                    al::changeBgmSituation(this, "CancelFocusMe");
                else
                    al::changeBgmSituation(this, "CancelFocusSe");
            }
        } else {
            al::startSe(this, "LastReactionHorn", nullptr);
            al::changeBgmSituation(this, "CancelFocusSe");
        }
    }
    if (al::isStep(this, 79))
        al::changeBgmSituation(this, "CancelFocusSe");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvKoopaChaseCarWait);
}

void KoopaChaseCar::startClipped() {
    al::LiveActor::startClipped();
    al::changeBgmSituation(this, "CancelFocusSe");
    mIsPlayingMusic = false;
    mReactionCount = 0;
}
