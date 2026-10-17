#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>
namespace al {
class LayoutInitInfo;
}
/** @brief Displays a score or extra-life popup at a screen or world position.
 */
class ScoreNumber : public al::LayoutActor {
public:
  explicit ScoreNumber(const al::LayoutInitInfo &rInfo);
  al::SceneCameraInfo *getSceneCameraInfo() const override {
    return mSceneCameraInfo;
  }
  void startScore(int score, unsigned int playerFlags,
                  const sead::Vector2f &rPos, int delay);
  void startScore(int score, unsigned int playerFlags, int delay);
  void startScore(int score, unsigned int playerFlags,
                  const sead::Vector3f *pPos, const sead::Vector3f &rOffset,
                  int delay);
  void startScore(int score, unsigned int playerFlags,
                  const sead::Vector3f &rPos, int delay);
  void startPlayerUp(int count, const sead::Vector2f &rPos, int delay);
  void startPlayerUp(int count, int delay);
  void startPlayerUp(int count, const sead::Vector3f *pPos,
                     const sead::Vector3f &rOffset, int delay);
  void startPlayerUp(int count, const sead::Vector3f &rPos, int delay);
  void updateByWorldPosition(const sead::Vector3f &rPos);
  void start(int delay);
  void control() override;
  void exeDelay();
  void exeAppear();

private:
  const sead::Vector3f *mPosition = nullptr;
  sead::Vector3f mFixedPosition = sead::Vector3f::zero;
  sead::Vector3f mOffset = sead::Vector3f::zero;
  int mDelay = 0;
  al::SceneCameraInfo *mSceneCameraInfo;
  bool mIsPlayerUp = false;
};
static_assert(sizeof(ScoreNumber) == 0x160);
