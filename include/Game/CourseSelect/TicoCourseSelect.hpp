#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TicoCourseSelect : public al::LiveActor {
public:
    explicit TicoCourseSelect(const char* pName);
    ~TicoCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
};
