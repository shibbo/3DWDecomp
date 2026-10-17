#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectMiniature;
class CourseSelectFlag : public al::LiveActor {
public:
    CourseSelectFlag(const char* pName, CourseSelectMiniature* pMiniature);
    ~CourseSelectFlag() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void updateVisibility(bool isVisible);
    void startAppear();
    void startShow();
    void startWaitClearDemo();
    void startClearDemo();
    void exeHide();
    void exeAppear();
    void exeWait();
    void exeWaitClearDemo();
    void exeClearDemo();
private:
    struct FlagState {
        bool visible; bool gold; u8 character; u8 star;
        operator u32() const { u32 value; __builtin_memcpy(&value, this, 4); return value; }
    };
    u32 calcFlagState() const;
    void updateFlagAction(u32 state, bool isClearDemo);
    CourseSelectMiniature* mpMiniature;
    u32 mFlagState = 0;
};
static_assert(sizeof(CourseSelectFlag) == 0x158);
