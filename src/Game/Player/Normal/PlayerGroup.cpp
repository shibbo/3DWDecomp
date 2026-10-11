#include "Player/Normal/PlayerGroup.hpp"

#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Player/Normal/IUsePlayerActorAccessor.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

/**
 * @brief Creates an empty group; init() allocates the per-character arrays.
 */
PlayerGroup::PlayerGroup() : mCharaPlayers(nullptr), mAccessor(nullptr) {}

/**
 * @brief Allocates one player array per character.
 * @param playerNum Capacity of each character's array.
 * @param pAccessor Accessor used to query and change the players.
 */
void PlayerGroup::init(int playerNum, IUsePlayerActorAccessor* pAccessor) {
    mCharaPlayers = new PlayerArray[EPlayerChara::size()];

    for (s32 i = 0; i < EPlayerChara::size(); i++) {
        mCharaPlayers[i].allocBuffer(playerNum, nullptr);
    }

    mAccessor = pAccessor;
}

/**
 * @brief Adds a player to the array of its character.
 * @param pPlayer The player to add.
 */
void PlayerGroup::append(PlayerActor* pPlayer) {
    mCharaPlayers[mAccessor->getChara(pPlayer)].pushBack(pPlayer);
}

/**
 * @brief Changes the figure of every player of the same character.
 * @param pPlayer A player of the character.
 * @param figure The new figure.
 */
void PlayerGroup::changeFigure(PlayerActor* pPlayer, EPlayerFigure figure) {
    EPlayerChara chara = mAccessor->getChara(pPlayer);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        mAccessor->change(mCharaPlayers[chara][i], figure);
    }
}

/**
 * @brief Forces the figure of every player of the same character.
 * @param pPlayer A player of the character.
 * @param figure The new figure.
 */
void PlayerGroup::changeFigureForce(PlayerActor* pPlayer, EPlayerFigure figure) {
    EPlayerChara chara = mAccessor->getChara(pPlayer);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        mAccessor->changeForce(mCharaPlayers[chara][i], figure);
    }
}

/**
 * @brief Calculates the average position of the active players of a character.
 * @param pPos Output position.
 * @param pPlayer A player of the character.
 */
void PlayerGroup::calcGroupCenterPos(sead::Vector3f* pPos, const PlayerActor* pPlayer) const {
    EPlayerChara chara = mAccessor->getChara(pPlayer);
    pPos->set(0.0f, 0.0f, 0.0f);
    s32 activeNum = 0;

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mAccessor->isActive(mCharaPlayers[chara][i])) {
            *pPos += mAccessor->getTrans(mCharaPlayers[chara][i]);
            activeNum++;
        }
    }

    *pPos *= 1.0f / activeNum;
}

/**
 * @brief Finds the position of the active player of a character furthest along a direction.
 * @param pPos Output position; the first player's position if no player is active.
 * @param pPlayer A player of the character.
 * @param rOffset Direction to measure along.
 */
void PlayerGroup::calcGroupHeadPos(sead::Vector3f* pPos, const PlayerActor* pPlayer,
                                   const sead::Vector3f& rOffset) const {
    EPlayerChara chara = mAccessor->getChara(pPlayer);
    pPos->set(mAccessor->getTrans(mCharaPlayers[chara][0]));
    f32 maxDot = 0.0f;
    bool isFirst = true;

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mAccessor->isActive(mCharaPlayers[chara][i])) {
            sead::Vector3f trans = mAccessor->getTrans(mCharaPlayers[chara][i]);
            f32 dot = trans.dot(rOffset);

            if (isFirst || dot > maxDot) {
                maxDot = dot;
                pPos->set(trans);
            }

            isFirst = false;
        }
    }
}

/**
 * @brief Checks whether a player is the only active one of its character.
 * @param pPlayer The player.
 * @return True if the player is active and no other player of its character is.
 */
bool PlayerGroup::isLast(const PlayerActor* pPlayer) const {
    if (!mAccessor->isActive(pPlayer)) {
        return false;
    }

    EPlayerChara chara = mAccessor->getChara(pPlayer);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mCharaPlayers[chara][i] == pPlayer) {
            continue;
        }

        if (mAccessor->isActive(mCharaPlayers[chara][i])) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Removes every other active player of the same character.
 * @param pPlayer The player to keep.
 */
void PlayerGroup::killAllExcept(PlayerActor* pPlayer) {
    EPlayerChara chara = mAccessor->getChara(pPlayer);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mCharaPlayers[chara][i] != pPlayer &&
            mAccessor->isActive(mCharaPlayers[chara][i])) {
            al::startHitReaction(mCharaPlayers[chara][i], "消滅");
            rc::deactivatePlayer(mCharaPlayers[chara][i]);
        }
    }
}

/**
 * @brief Turns every other active player of the sensor host's character into score.
 * @param pSensor Sensor of the player to keep.
 */
void PlayerGroup::killAllExceptWithScore(al::HitSensor* pSensor) {
    auto* pHost = static_cast<PlayerActor*>(al::getSensorHost(pSensor));
    EPlayerChara chara = mAccessor->getChara(pHost);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mCharaPlayers[chara][i] != pHost && mAccessor->isActive(mCharaPlayers[chara][i])) {
            al::startHitReaction(mCharaPlayers[chara][i], "スコア化");
            rc::deactivatePlayer(mCharaPlayers[chara][i]);
            rc::addScoreBySystem(mCharaPlayers[chara][i], pSensor, "ダブルマリオ", 0);
        }
    }
}

/**
 * @brief Removes the equipment of every other active player of the sensor host's character.
 * @param pSensor Sensor of the player to keep.
 */
void PlayerGroup::removeAllEquipOfAllDoubleMarioExceptWithScore(al::HitSensor* pSensor) {
    auto* pHost = static_cast<PlayerActor*>(al::getSensorHost(pSensor));
    EPlayerChara chara = mAccessor->getChara(pHost);

    for (s32 i = 0; i < mCharaPlayers[chara].size(); i++) {
        if (mCharaPlayers[chara][i] != pHost && mAccessor->isActive(mCharaPlayers[chara][i])) {
            rc::removeAllEquipFromPlayerGoalPole(mCharaPlayers[chara][i]);
        }
    }
}

/**
 * @brief Counts the active double cherry copies, i.e. the active players beyond the first of
 * each character.
 * @return The number of copies.
 */
int PlayerGroup::calcDoubleMarioTotalNum() const {
    s32 totalNum = 0;

    for (s32 c = 0; c < EPlayerChara::size(); c++) {
        u32 activeNum = 0;

        for (s32 i = 0; i < mCharaPlayers[c].size(); i++) {
            if (mAccessor->isActive(mCharaPlayers[c][i])) {
                activeNum++;
            }
        }

        if (activeNum > 1) {
            totalNum += activeNum - 1;
        }
    }

    return totalNum;
}
