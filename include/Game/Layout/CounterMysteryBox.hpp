#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class CounterMysteryBox : public al::LayoutActor {
public:
    explicit CounterMysteryBox(const al::LayoutInitInfo& rInfo);

    void setCount(s32 count);
    void exeWait();
};
