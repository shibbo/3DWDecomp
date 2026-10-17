#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class GameDataHolder;
class ButtonGroup;

class GameOverMenu : public al::LayoutActor {
public:
    explicit GameOverMenu(const GameDataHolder* pGameData);
    virtual void init(const al::LayoutInitInfo& rInfo);
    void appear() override;
    bool isDecideContinue() const;
    bool isDecideQuit() const;
    void exeAppear();
    void exeWait();
    void exeEnd();
private:
    const GameDataHolder* mGameData;
    ButtonGroup* mButtonGroup = nullptr;
    int mPadPort = -1;
};
