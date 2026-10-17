#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadMatrix.h>
namespace al {
class LayoutInitInfo;
}
class GameDataHolder;
class WipeCurtainRankingParts;
/** @brief Coordinates the result ranking curtain and its player score columns.
 */
class WipeCurtainRanking : public al::LayoutActor {
public:
  /** @brief Creates the ranking curtain and four score columns. */
  WipeCurtainRanking(const al::LayoutInitInfo &rInfo,
                     const GameDataHolder *pHolder);
  /** @brief Finds the highest active score and starts the result sequence. */
  void appear() override;
  /** @brief Returns a player score. */
  int getScore(int userId) const;
  /** @brief Ranks a player against the active users. */
  int calcRank(int userId) const;
  /** @brief Checks for a shared first place or a brigade stage. */
  bool isExistMultiPlayer1st() const;
  /** @brief Starts the curtain exit. */
  void startEnd();
  /** @brief Shows the skip button. */
  void startSkipAppear();
  /** @brief Checks whether the curtain is ready for its owner to end it. */
  bool isEnableEnd() const;
  /** @brief Checks whether every score column has stopped. */
  bool isSkipReady() const;
  /** @brief Emits the crown effect at the requested pane when enabled. */
  bool tryEmitCrownEffect(const WipeCurtainRankingParts *pParts,
                          const char *pPane);
  /** @brief Disables and removes the curtain effects. */
  void killEffects();
  /** @brief Shows score columns and starts the counting sound. */
  void exeAppear();
  /** @brief Coordinates score counting and detects completion. */
  void exeRise();
  /** @brief Waits for the owner to request the skip button. */
  void exeSkipReady();
  /** @brief Shows the skip button and its entrance sound. */
  void exeSkipAppear();
  /** @brief Accepts confirmation from an active player or the main controller.
   */
  void exeSkipWait();
  /** @brief Finishes the skip button press animation. */
  void exeSkipEnd();
  /** @brief Waits for the owner to request the curtain exit. */
  void exeEndInit();
  /** @brief Plays the curtain exit and kills the layout. */
  void exeEnd();
  /** @brief Returns the highest score found when the curtain appeared. */
  int getMaxScore() const { return mMaxScore; }
  /** @brief Returns the game data used for ranking. */
  const GameDataHolder *getGameDataHolder() const { return mGameDataHolder; }

private:
  const GameDataHolder *mGameDataHolder;
  WipeCurtainRankingParts **mParts = nullptr;
  int mMaxScore = 0;
  sead::Matrix34f mCrownMtx = sead::Matrix34f::ident;
  bool mIsEffectEnabled = true;
};
static_assert(sizeof(WipeCurtainRanking) == 0x170);
