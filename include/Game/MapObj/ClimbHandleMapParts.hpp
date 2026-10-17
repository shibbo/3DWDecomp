#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>

class ClimbHandleMapParts : public al::LiveActor {
public:
    ClimbHandleMapParts(const char* pName);
    ~ClimbHandleMapParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
    void exeStop();
    void exeUp();
    void exeDown();
    void setNerveToStop();
    void setNerveToUp();
    void setNerveToDown();

private:
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    bool mIsSingleMode = false;
};
