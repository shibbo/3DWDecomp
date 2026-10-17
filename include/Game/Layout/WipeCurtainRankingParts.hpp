#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al {
class LayoutInitInfo;
}
class WipeCurtainRanking;
/** @brief A player score column in the ranking curtain. */
class WipeCurtainRankingParts : public al::LayoutActor {
public:
  WipeCurtainRankingParts(const al::LayoutInitInfo &rInfo, const char *pName,
                          const char *pPane, int userId,
                          WipeCurtainRanking *pParent);
  void appear() override;
  void sharedAppear();
  void networkInfo();
  void appearKinopioBrigade();
  void startRise();
  bool isStop() const;
  bool isDeactive() const;
  bool isEndRise() const;
  void exeWait();
  void exeRise();
  void exeRanking();
  void exeEnd();
  void exeDeactive();
  /** @brief Updates how many columns have finished counting. */
  void setFinishedCount(int count) { mFinishedCount = count; }

private:
  WipeCurtainRanking *mParent;
  int mUserId;
  int mFinishedCount = 0;
  bool mIsBrigade = false;
};
static_assert(sizeof(WipeCurtainRankingParts) == 0x140);
