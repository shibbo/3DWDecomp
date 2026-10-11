#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class KarakuriCastleDoor;
class KarakuriCastleDoorWatcher : public al::LiveActor {
public:
    KarakuriCastleDoorWatcher(const char*);
    ~KarakuriCastleDoorWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWait();
    void exeOpen();
private:
    KarakuriCastleDoor** mDoors = nullptr;
    int mDoorCount = 0;
    int mOpenIndex = 0;
};
