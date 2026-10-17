#pragma once

class IUsePlayerActionObserver;

class PlayerBgmStopper {
public:
    explicit PlayerBgmStopper(IUsePlayerActionObserver* pObserver);
    ~PlayerBgmStopper();
    void update();

private:
    IUsePlayerActionObserver* mObserver;
    bool mIsStopped = false;
};

static_assert(sizeof(PlayerBgmStopper) == 0x10);
