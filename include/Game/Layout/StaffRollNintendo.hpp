#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class StaffRollNintendo : public al::LayoutActor {
public:
    explicit StaffRollNintendo(const al::LayoutInitInfo& rInfo);

    void appear() override;
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    void startEnd();
    bool isEnd() const;
};
