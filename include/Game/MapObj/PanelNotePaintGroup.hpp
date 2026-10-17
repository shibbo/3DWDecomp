#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
namespace al {
class AudioGeneralPurposeAreaChecker;
}
class PanelNotePaint;
class PanelNotePaintGroup : public al::LiveActor {
  public:
    explicit PanelNotePaintGroup(const char* name);
    void init(const al::ActorInitInfo& info) override;
    void control() override;
    void exeWait();
    void updatePitch();
    void exeComplete();
    void exeEnd();
    sead::Vector3f* findPosInfo(int id);
    void setPosInfo(int id, sead::Vector3f* pos);

  private:
    struct PosInfo {
        int id = -1;
        sead::Vector3f* pos = nullptr;
    };
    PanelNotePaint** mPanels = nullptr;
    int mPanelCount = 0;
    bool* mWasOn = nullptr;
    int mLastActivatedIndex = -1;
    int mPitch = 0;
    int mPitchOffset = 0;
    PosInfo** mPositions;
    int mSeType = 0;
    int mBgmPhase = 0;
    bool mAlternateOctaves = false;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    float mCompleteRadius = 0.0f;
    bool* mEffectStarted = nullptr;
    al::AudioGeneralPurposeAreaChecker* mAreaChecker;
    bool mBgmStarted = false;
};
static_assert(sizeof(PanelNotePaintGroup) == 0x1d0);
