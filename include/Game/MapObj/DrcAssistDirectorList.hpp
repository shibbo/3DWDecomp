#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include <container/seadObjList.h>

class DrcAssistDirectorList : public al::ISceneObj {
public:
    explicit DrcAssistDirectorList(s32 maxNum);
    const char* getSceneObjName() const override;
    sead::ObjList<DrcAssistDirector>& getDirectors() { return mDirectors; }
    void setEnable(bool enable);
    void validate();
    void invalidateImmediate();
    void checkPreStartPose();
    void update();
    bool isTouch();
    bool isTouch(const sead::Vector3f& position, float radius) const;
    bool isTouchByPointer(const al::LiveActor* actor) const;
    bool isTouchTrigger(const sead::Vector3f& position, float radius) const;
    bool isEnableTouchPointer() const;
    bool isEnableTouchPointerGrabItem(const al::LiveActor* actor) const;
    void releaseTouchPointerHoldItem(const al::LiveActor* pointer);
    void releaseTouchPointerAllHoldItem();
    al::LiveActor* findDrcAssistDirectorPlayer(const al::ScreenPointer* pointer);
    al::LiveActor* findDrcAssistDirectorPlayer(const al::HitSensor* pointer);
    al::LiveActor* findDrcAssistDirectorTouchPointer(const al::ScreenPointer* pointer);
    DrcTouchPointer* getTouchPointer() const;
    al::LiveActor* getFirstTouchingTouchPointer() const;
    const sead::Vector3f& getTouchPointerPosition(const al::LiveActor* actor) const;
    const sead::Vector3f& getTouchedPointerPosition(const al::LiveActor* actor) const;
    const sead::Vector3f& getTouchPointerPosition(const al::IUseSceneObjHolder* actor) const;
    const sead::Vector3f& getTouchPointerNormal(const al::LiveActor* actor) const;
    void calcTouchPointerUpDir(sead::Vector3f* out);
    bool tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* out, const al::LiveActor* actor);
    bool tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* out, int port);
    bool tryCalcTouchPointerSlideDirOnWorldByPointer(sead::Vector3f* out,
                                                     const al::LiveActor* actor);
    void setStampDirector(rc::StampDirector* director);
    void setPlayerAliveWatcher(PlayerAliveWatcher* watcher);
    void setPlayerHolder(al::PlayerHolder* holder);
    void setIgnoreTriggerFrames(int frames);
    void setKinopioBrigadeFlag(bool brigade);
    void addTouchAssist(int port, bool isEnable);
    void initAfterPlacementSceneObj(const al::ActorInitInfo& info) override;
    void removeTouchAssist(int port);
    void setTouchDisabled(bool disabled);
    void disappearTouchPointer();
    void disappearTouchPointerImmediately();
    void disappearTouchPointerEffect();
    void invalidate(bool force);
    bool tryCalcTouchPointerSlideDirOnWorld(sead::Vector3f* out, const al::IUseCamera* camera);
    void keepAppearPointer();
    void startSnapshotMode();
    void endSnapshotMode();
    bool isPlayerHoldingStamp();
    DrcAssistDirector* getDrcAssist(int port);

private:
    sead::ObjList<DrcAssistDirector> mDirectors;
    DrcTouchEffectTraceTracker* mTraceTracker;
};
static_assert(sizeof(DrcAssistDirectorList) == 0x40);

namespace rc {
void updateDrcAssistDirector(const al::IUseSceneObjHolder* pHolder);
}
