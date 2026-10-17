#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GigaBellPedestal : public al::LiveActor {
public:
    explicit GigaBellPedestal(const char*);
    ~GigaBellPedestal() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void setColor(int);
};
