#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalItemCollectListener.hpp"
namespace al { class AreaObjGroup; }
class GoalItem;
class Shards;
class ShardsWatcher : public al::LiveActor, public IGoalItemCollectListener {
public:
    explicit ShardsWatcher(const char*);
    ~ShardsWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void appear() override;
    void hide();
    void kill() override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    void killObject(bool);
    void goalItemCollectCallback() override;
    void exeWatch();
    void exeWatchLast();
    void exeWait();
    void setGoalitemPos();
    bool isLastShard() const;
    bool isFinalShard() const;
    int getId() const { return mId; }
    void setShardId(int shardId) { mShardId = shardId; }
    static bool sIsFinalShardGetPending;
private:
    struct Piece { sead::Vector3f offset; Shards* actor; };
    Piece* mPieces = nullptr;
    GoalItem* mGoalItem = nullptr;
    int mPieceCount = 0;
    int mUnknown164 = 0;
    int mDelay = 10;
    int mShardId = -1;
    al::AreaObjGroup* mAreaGroup = nullptr;
    int mId = 0;
    int mScenarioId = 0;
    bool mPlayerInIsland = false;
    int mLastPiece = 0;
    sead::Vector3f mGoalPosition;
    sead::Vector3f mGoalFront;
    bool mCollectedBySensor;
    bool mUseFrontAngle = false;
    float mFrontAngle = 0.0f;
    al::HitSensor* mCollectSensor = nullptr;
};
static_assert(sizeof(ShardsWatcher) == 0x1b0);
