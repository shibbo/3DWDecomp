#pragma once

#include "Library/MapObj/FixMapParts.hpp"

class GoalPedestal : public al::FixMapParts {
public:
    explicit GoalPedestal(const char* pName);
    ~GoalPedestal() override;
    void init(const al::ActorInitInfo& rInfo) override;
};
