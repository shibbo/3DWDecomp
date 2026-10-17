#pragma once

namespace al {
class ActorInitInfo;
class LiveActor;
}

/** @brief The three optional action commands assigned to a demo actor. */
class DemoActionList {
public:
    DemoActionList(const al::ActorInitInfo& rInfo, const char* pArgPrefix);
    const char* getActionName(int index) const;
    void startAction(al::LiveActor* pActor, int index);
    /** @brief Gets the action slot count. @return Number of available action slots. */
    int getActionCount() const { return mActionCount; }

private:
    const char** mActionNames;
    int mActionCount;
};
