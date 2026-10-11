#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class PanelNote;
class PanelNoteGroup : public al::LiveActor {
public:
    PanelNoteGroup(const char*);
    ~PanelNoteGroup() override;
    void init(const al::ActorInitInfo&) override;
    void exeWait();
    void exeComplete();
private:
    PanelNote** mPanels = nullptr;
    int mPanelCount = 0;
    bool* mWasOn = nullptr;
    int mLastActivatedIndex = 0;
};
