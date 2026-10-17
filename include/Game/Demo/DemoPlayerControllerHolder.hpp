#pragma once

namespace al {
class LiveActor;
class PlayerHolder;
}
class PlayerActor;
class DemoPlayerController;

/** @brief Collects living players and coordinates their demo controllers. */
class DemoPlayerControllerHolder {
public:
    explicit DemoPlayerControllerHolder(al::PlayerHolder* pPlayerHolder);
    bool requestStartDemo();
    void requestEndDemo();
    bool isStartDemo() const;
    void endDemo();
    int getDemoPlayerNum() const;
    DemoPlayerController* getDemoPlayer(int index) const;
    DemoPlayerController* getDemoPlayerByCharacter(int character) const;
    DemoPlayerController* getDemoPlayerByActor(const al::LiveActor* pActor) const;

private:
    al::PlayerHolder* mPlayerHolder;
    DemoPlayerController* mControllers;
    PlayerActor** mPlayers;
    int mPlayerCount;
    bool mIsStarted;
    bool mIsRequested;
};
