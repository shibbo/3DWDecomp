#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class StaffRollFin : public al::LayoutActor {
public:
    explicit StaffRollFin(const al::LayoutInitInfo& rInfo);

    void appear() override;
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    void startEnd();
    bool isEnd() const;
};
