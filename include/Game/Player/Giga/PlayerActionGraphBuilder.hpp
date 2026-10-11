#pragma once

class Player;
class PlayerActionGraph;

class IUsePlayerActionGraphBuilder {
public:
    virtual PlayerActionGraph* create(Player* pPlayer) = 0;
};

class PlayerActionGraphBuilder : public IUsePlayerActionGraphBuilder {
public:
    explicit PlayerActionGraphBuilder(bool isSingleMode);
    PlayerActionGraph* create(Player* pPlayer) override;

private:
    bool mIsSingleMode;
};
static_assert(sizeof(PlayerActionGraphBuilder) == 0x10);
