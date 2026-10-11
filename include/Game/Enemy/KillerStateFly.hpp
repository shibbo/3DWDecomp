#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class Killer;
struct KillerStateFlyParam;

class KillerStateFly : public al::NerveStateBase {
public:
    KillerStateFly(Killer* pHost);
    /** @brief Releases the flight state. */
    ~KillerStateFly() override = default;
    void appear() override;
    void kill() override;
    void reset();
    void exeFlyWaitStart();
    void exeFlyWait();
    int getStepFly() const;

private:
    const KillerStateFlyParam& getParam() const;
    Killer* mHost;
    int mStepFly = 0;
};
