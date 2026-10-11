#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class TentackHill : public al::LiveActor {
public:
    explicit TentackHill(const char* pName);
    ~TentackHill() override;
    void initActorWithModelName(const al::ActorInitInfo& rInfo, const char* pModelName,
                                const char* pSuffix);
    void appear() override;
    void exeAppear();
    void exeAppearWait();
    void exeWait();
    void exeDisappearWait();
    void exeDisappear();
    bool trySetWaitIfNotPlaying();
    void setReverse();
    void setDisappear();
};
static_assert(sizeof(TentackHill) == 0x148);
