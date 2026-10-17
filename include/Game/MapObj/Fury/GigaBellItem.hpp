#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class GigaBellItem : public al::LiveActor {
public:
    explicit GigaBellItem(const char* pName);
    ~GigaBellItem() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appearHidden();
    void appearPopUpAbove();
    void appearPopUpAboveSilent();
    void exeWait();
    void exePopUpAbove();
    void exeHidden();
    void stopForCutscene();
    /** @brief Sets whether the bell respawns after being collected. */
    void setIsRespawn(bool isRespawn) { mIsRespawn = isRespawn; }
private:
    bool mIsRespawn = false;
};
static_assert(sizeof(GigaBellItem) == 0x148);
