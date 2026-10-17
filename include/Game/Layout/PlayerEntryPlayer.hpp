#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class PlayerEntryPlayerHolder;
class PlayerModelHolder;
class PlayerKeyConfig;

/**
 * @brief 3D player model that jumps in when a player decides a character in the
 * entry window.
 */
class PlayerEntryPlayer : public al::LiveActor {
public:
  PlayerEntryPlayer(const char *pName, PlayerEntryPlayerHolder *pHolder);
  void init(const al::ActorInitInfo &rInfo) override;
  void appear() override;
  void kill() override;
  void control() override;
  void exeFall();
  void exeLand();
  void exeWait();
  void exeTurn();
  void exeJump();
  void exeJumpOut();
  void exeDecision();
  void startPlayerAction(const char *pActionName);
  bool isPlayerActionEnd() const;
  bool tryStartJump(const PlayerKeyConfig &rKeyConfig);
  void setPlayerModel(PlayerModelHolder *pModel);
  void appearPlayer(s32 userId);
  void startDecision();
  void startJumpOut();
  bool isLand() const;
  bool isEndDecision() const;
  bool isDead() const;

  /** @brief Releases the model held back from its start-decision animation. */
  void requestStartDecision() { mIsWaitStartDecision = false; }

private:
  PlayerEntryPlayerHolder *mHolder;
  PlayerModelHolder *mPlayerModel = nullptr;
  s32 mUserId = 0;
  f32 mRotateY = 0.0f;
  bool mIsWaitStartDecision = true;
};
