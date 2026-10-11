#pragma once

#include "Library/Scene/ISceneObj.hpp"
#include "Library/Execute/IUseExecutor.hpp"
#include <container/seadBuffer.h>

namespace al { class IUseSceneObjHolder; }
namespace rc { class IUseTimer; }
class CheckPointStar;
class Rabbit;

class TimerManager : public al::ISceneObj, public al::IUseExecutor {
public:
    static bool isTimerManagerActive(al::IUseSceneObjHolder* pUser);
    static TimerManager* tryGetTimerManager(al::IUseSceneObjHolder* pUser);

    TimerManager();
    virtual const char* getSceneObjName() const;
    virtual void initSceneObj();
    virtual void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo);
    virtual void execute();

    bool isActive();
    void forceCancelCurrent(bool isCancelCheckPoint, bool isCancelRabbit);
    void cancelCheckPoint();
    void cancelRabbit();
    bool canCancel() const;
    bool activateTimer(rc::IUseTimer* pTimer);
    bool deactivateTimer(rc::IUseTimer* pTimer);
    void setCurrentCheckPoint(CheckPointStar* pCheckPoint);
    void resetCurrentCheckPoint();
    void setCurrentRabbit(Rabbit* pRabbit);
    void resetCurrentRabbit(Rabbit* pRabbit);

    rc::IUseTimer* mCurrentTimer = nullptr; // 0x10
    CheckPointStar* mCurrentCheckPoint = nullptr; // 0x18
    sead::Buffer<Rabbit*> mRabbits; // 0x20
};
