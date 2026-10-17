#pragma once
namespace al {
class HitSensor;
class LiveActor;
} // namespace al
class NpcTargetFinder;
class NpcTargetFinderParam;
namespace npc {
enum NpcFindTargetType : unsigned int {
    None = 0,
    PlayerRegular = 1,
    PlayerClimb = 2,
    KoopaJr = 4,
    Cursor = 8,
    Ball = 16,
    Koura = 32,
    Bird = 64,
    Neko = 128,
    Player = PlayerRegular | PlayerClimb,
    ChaseObj = Ball | Koura | Bird,
    NpcFindTargetType_All = 0xffffffff,
    NpcFindTargetType_None = None,
    NpcFindTargetType_PlayerRegular = PlayerRegular,
    NpcFindTargetType_PlayerClimb = PlayerClimb,
    NpcFindTargetType_KoopaJr = KoopaJr,
    NpcFindTargetType_Cursor = Cursor,
    NpcFindTargetType_Ball = Ball,
    NpcFindTargetType_Koura = Koura,
    NpcFindTargetType_Bird = Bird,
    NpcFindTargetType_Neko = Neko,
    NpcFindTargetType_Player = Player,

};
bool isSensorBall(const al::HitSensor* pSensor);
bool isSensorBird(const al::HitSensor* pSensor);
bool isSensorNeko(const al::HitSensor* pSensor);
bool isSensorNekoDisaster(const al::HitSensor* pSensor);
const char* NpcFindTargetTypeToString(const NpcFindTargetType& rType);
bool calcIsTargetInSight(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                         const NpcTargetFinderParam* pParam);
bool calcIsTargetInChaseRange(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                              const NpcTargetFinderParam* pParam);
} // namespace npc
