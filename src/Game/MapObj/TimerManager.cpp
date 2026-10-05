#include "MapObj/TimerManager.hpp"
#include "MapObj/IUseTimer.hpp"
#include "MapObj/CheckPointStar.hpp"
#include "NPC/Rabbit.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"

bool TimerManager::isTimerManagerActive(al::IUseSceneObjHolder* pUser) {
    TimerManager* manager = tryGetTimerManager(pUser);
    return manager && manager->isActive();
}

TimerManager* TimerManager::tryGetTimerManager(al::IUseSceneObjHolder* pUser) {
    return static_cast<TimerManager*>(al::tryGetSceneObj(pUser, 57));
}

bool TimerManager::isActive() {
    return mCurrentTimer != nullptr;
}

TimerManager::TimerManager() {
    mRabbits.tryAllocBuffer(4, nullptr, 8);
    mRabbits.fill(nullptr);
}

const char* TimerManager::getSceneObjName() const {
    return "TimerManager";
}

void TimerManager::initSceneObj() {
}

void TimerManager::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    al::registerExecutorUser(this, rInfo.getExecuteDirector(), "TimerManager");
}

void TimerManager::execute() {
}

void TimerManager::forceCancelCurrent(bool isCancelCheckPoint, bool isCancelRabbit) {
    if (mCurrentTimer) {
        mCurrentTimer->forceCancel();
    }
    if (isCancelCheckPoint) {
        cancelCheckPoint();
    }
    if (isCancelRabbit) {
        cancelRabbit();
    }
}

void TimerManager::cancelCheckPoint() {
    if (mCurrentCheckPoint) {
        CheckPointStar* checkPoint = mCurrentCheckPoint;
        mCurrentCheckPoint = nullptr;
        checkPoint->cancel();
        mCurrentCheckPoint = nullptr;
    }
}

void TimerManager::cancelRabbit() {
    for (auto it = mRabbits.begin(); it.getIndex() != mRabbits.size(); ++it) {
        Rabbit*& rabbit = *it;
        if (rabbit) {
            rabbit->cancel();
            rabbit = nullptr;
        }
    }
}

bool TimerManager::canCancel() const {
    return !mCurrentTimer || mCurrentTimer->canCancel();
}

bool TimerManager::activateTimer(rc::IUseTimer* pTimer) {
    if (mCurrentTimer) {
        if (mCurrentTimer == pTimer) {
            return false;
        }
        mCurrentTimer->reset();
    }
    mCurrentTimer = pTimer;
    return true;
}

bool TimerManager::deactivateTimer(rc::IUseTimer* pTimer) {
    bool isCurrent = mCurrentTimer == pTimer && mCurrentTimer;
    mCurrentTimer = nullptr;
    return isCurrent;
}

void TimerManager::setCurrentCheckPoint(CheckPointStar* pCheckPoint) {
    if (pCheckPoint) {
        if (mCurrentCheckPoint != pCheckPoint) {
            cancelCheckPoint();
        }
        mCurrentCheckPoint = pCheckPoint;
    }
}

void TimerManager::resetCurrentCheckPoint() {
    mCurrentCheckPoint = nullptr;
}

void TimerManager::setCurrentRabbit(Rabbit* pRabbit) {
    if (!pRabbit) {
        return;
    }
    for (auto& rabbit : mRabbits) {
        if (rabbit == pRabbit) {
            return;
        }
        if (!rabbit) {
            rabbit = pRabbit;
            return;
        }
    }
}

void TimerManager::resetCurrentRabbit(Rabbit* pRabbit) {
    if (pRabbit) {
        for (auto it = mRabbits.begin(); it.getIndex() != mRabbits.size(); ++it) {
            Rabbit*& rabbit = *it;
            if (rabbit == pRabbit) {
                rabbit = nullptr;
            }
        }
    } else {
        mRabbits.fill(nullptr);
    }
}
