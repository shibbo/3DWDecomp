#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
template <class T>
class DeriveActorGroup;
}  // namespace al
class AssistLeaf;
class Ball;
class Bomb;
class BoomerangFlower;
class CoinBlow;
class CoinCountUp;
class DoorKey;
class DoubleMario;
class FireFlower;
class GigaBell;
class KinokoBig;
class KinokoGiga;
class KinokoOneUp;
class KinokoSuper;
class KinokoTreasure;
class SuperBell;
class SuperBellSpecial;
class SuperLeaf;
class SuperStar;
class WhiteBell;

/**
 * @brief Pool of the items the item director can spawn at runtime.
 *
 * Every item kind has its own actor group that is created on demand by the matching declare
 * function. The getters hand out a dead actor of the pool, recycling the first one when all of
 * them are in use.
 */
class ItemHolder {
public:
    ItemHolder(const al::ActorInitInfo& rInfo, bool isSingleMode);

    void initGroupCoinCountUp(const al::ActorInitInfo& rInfo);
    void initGroupCoinBlow(const al::ActorInitInfo& rInfo);
    void declareStockItem(const al::ActorInitInfo& rInfo);
    void declareItem(const char* pName, const al::ActorInitInfo& rInfo);
    void declareKinokoOneUp(const al::ActorInitInfo& rInfo);
    void declareKinokoSuper(const al::ActorInitInfo& rInfo);
    void declareKinokoTreasure(const al::ActorInitInfo& rInfo);
    void declareSuperBellSpecial(const al::ActorInitInfo& rInfo);
    void declareSuperBell(const al::ActorInitInfo& rInfo);
    void declareFireFlower(const al::ActorInitInfo& rInfo);
    void declareSuperLeaf(const al::ActorInitInfo& rInfo);
    void declareBoomerangFlower(const al::ActorInitInfo& rInfo);
    void declareSuperStar(const al::ActorInitInfo& rInfo);
    void declareBall(const al::ActorInitInfo& rInfo);
    void declareBomb(const al::ActorInitInfo& rInfo);
    void declareDoubleMario(const al::ActorInitInfo& rInfo);
    void declareKinokoBig(const al::ActorInitInfo& rInfo);
    void declareAssistLeaf(const al::ActorInitInfo& rInfo);
    void declareDoorKey(const al::ActorInitInfo& rInfo);
    void declareGigaBell(const al::ActorInitInfo& rInfo);
    void declareKinokoGiga(const al::ActorInitInfo& rInfo);
    void declareWhiteBell(const al::ActorInitInfo& rInfo);

    CoinBlow* getCoinBlow() const;
    CoinCountUp* getCoinCountUp() const;
    KinokoOneUp* getKinokoOneUp() const;
    KinokoSuper* getKinokoSuper() const;
    KinokoTreasure* getKinokoTreasure() const;
    SuperBell* getSuperBell() const;
    FireFlower* getFireFlower() const;
    BoomerangFlower* getBoomerangFlower() const;
    SuperLeaf* getSuperLeaf() const;
    SuperStar* getSuperStar() const;
    Ball* getBall() const;
    Bomb* getBomb() const;
    DoubleMario* getDoubleMario() const;
    KinokoBig* getKinokoBig() const;
    AssistLeaf* getAssistLeaf() const;
    SuperBellSpecial* getSuperBellSpecial() const;
    DoorKey* getDoorKey() const;
    GigaBell* getGigaBell() const;
    KinokoGiga* getKinokoGiga() const;
    WhiteBell* getWhiteBell() const;

private:
    al::DeriveActorGroup<CoinBlow>* mCoinBlowGroup = nullptr;                  // 0x00
    al::DeriveActorGroup<CoinCountUp>* mCoinCountUpGroup = nullptr;            // 0x08
    al::DeriveActorGroup<KinokoOneUp>* mKinokoOneUpGroup = nullptr;            // 0x10
    al::DeriveActorGroup<KinokoSuper>* mKinokoSuperGroup = nullptr;            // 0x18
    al::DeriveActorGroup<KinokoTreasure>* mKinokoTreasureGroup = nullptr;      // 0x20
    al::DeriveActorGroup<SuperBell>* mSuperBellGroup = nullptr;                // 0x28
    al::DeriveActorGroup<FireFlower>* mFireFlowerGroup = nullptr;              // 0x30
    al::DeriveActorGroup<SuperLeaf>* mSuperLeafGroup = nullptr;                // 0x38
    al::DeriveActorGroup<BoomerangFlower>* mBoomerangFlowerGroup = nullptr;    // 0x40
    al::DeriveActorGroup<SuperStar>* mSuperStarGroup = nullptr;                // 0x48
    al::DeriveActorGroup<Ball>* mBallGroup = nullptr;                          // 0x50
    al::DeriveActorGroup<Bomb>* mBombGroup = nullptr;                          // 0x58
    al::DeriveActorGroup<DoubleMario>* mDoubleMarioGroup = nullptr;            // 0x60
    al::DeriveActorGroup<KinokoBig>* mKinokoBigGroup = nullptr;                // 0x68
    al::DeriveActorGroup<AssistLeaf>* mAssistLeafGroup = nullptr;              // 0x70
    al::DeriveActorGroup<SuperBellSpecial>* mSuperBellSpecialGroup = nullptr;  // 0x78
    al::DeriveActorGroup<DoorKey>* mDoorKeyGroup = nullptr;                    // 0x80
    al::DeriveActorGroup<GigaBell>* mGigaBellGroup = nullptr;                  // 0x88
    al::DeriveActorGroup<KinokoGiga>* mKinokoGigaGroup = nullptr;              // 0x90
    al::DeriveActorGroup<WhiteBell>* mWhiteBellGroup = nullptr;                // 0x98
    bool mIsSingleMode;                                                        // 0xa0
};

static_assert(sizeof(ItemHolder) == 0xa8);
