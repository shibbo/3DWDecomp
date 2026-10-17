#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class SaveDataChecker : public al::LiveActor {
public:
    SaveDataChecker(const char* pName);
    ~SaveDataChecker() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appear() override;
    void onSwitchSetSave();
    void exeDead();
    void exeWait();

private:
    int mCutsceneId = -1;
};
