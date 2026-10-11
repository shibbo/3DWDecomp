#pragma once

#include "Library/HostIO/IUseHioNode.hpp"

namespace al {
class ActorInitInfo;
}
class DemoScenePlayerModel;

/** @brief Collects figure requests and creates the corresponding demo player models. */
class DemoPlayerModelDirector : public al::IUseHioNode {
public:
    DemoPlayerModelDirector();
    void requestCreateAllPlayer(const char* pSuffix);
    void setPlayerSuffix(const char* pSuffix);
    void requestCreateAllFigure(int character, const char* pSuffix, bool keepSuffix);
    void requestCreateFigureSuper(int character, const char* pSuffix);
    void requestCreateFigureClimb(int character, const char* pSuffix);
    void requestCreateFigureClimbGiga(int character, const char* pSuffix);
    void endInit(const al::ActorInitInfo& rInfo);
    DemoScenePlayerModel* getDemoPlayerModel(int character) const;

private:
    enum Request { None, Super, Climb, ClimbGiga, All };
    DemoScenePlayerModel** mModels;
    Request* mRequests;
    const char* mSuffix;
};
