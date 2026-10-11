#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <prim/seadBitFlag.h>

#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CameraInfo;
}  // namespace al

class CourseSelectDirector;
class CourseSelectPuppeteer;
class CourseSelectPuppeteerGroup;
class CourseSelectSensor;
class RocketController;

/**
 * @brief Rocket of the course-select map, carrying the players to the special worlds. The rocket
 * placed on the map owns a second rocket at its landing point (linked as "LandRocket"); entering
 * one of them launches the players to the other one.
 * @note A stub definition also exists in Scene/ProjectActorFactoryTypes.hpp; never include both.
 */
class CourseSelectRocket : public al::LiveActor {
public:
    /** @brief World the rocket leads to, read from the "RocketType" placement argument. */
    enum RocketType : s32 {
        cRocketType_SpecialWorld = 0,
        cRocketType_ChampionshipWorld = 1,
        cRocketType_ArrangeWorld = 2,
    };

    explicit CourseSelectRocket(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool isDemo() const;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void control() override;
    void startOpenDemo();
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup);
    void endPuppetDemo();
    void startBind(CourseSelectPuppeteer* pPuppeteer);
    bool tryDecide(const CourseSelectDirector* pDirector);

    void exeRock();
    void exeBreakable();
    void exeWait();
    void exeDemoOpen();
    void exeDemoBreak();
    void exeDemoBreakFade();
    void exeDemoBreakFadeWait();
    void exeDemoBreakFadeEnd();
    void exeDemoBreakInfo();
    void exeDemoStart();
    void exeDemoLaunchFirst();
    void exeDemoLaunch();
    void exeDemoLand();
    void exeDemoExit();
    void exeDemoBeforeSave();
    void exeDemoSave();

private:
    typedef sead::PtrArray<al::LiveActor> ActorArray;

    inline bool isOpenWorld() const;
    inline void appearFairies(const char* pActionName);
    inline void appearFairyTools();
    inline void startPuppeteerRocketDemoEnd();

    bool mIsOpen;  // 0x144
    bool mIsKillReserved = false;  // 0x145
    bool mIsLandRocket = false;  // 0x146
    RocketType mRocketType = cRocketType_SpecialWorld;  // 0x148
    sead::BitFlag16 mRidePlayerFlags;  // 0x14c
    CourseSelectRocket* mPairRocket = nullptr;  // 0x150
    al::LiveActor* mRock = nullptr;  // 0x158
    al::LiveActor* mRockBroken = nullptr;  // 0x160
    al::LiveActor* mBase = nullptr;  // 0x168
    RocketController* mController = nullptr;  // 0x170
    CourseSelectDirector* mDirector = nullptr;  // 0x178
    CourseSelectSensor* mSensor = nullptr;  // 0x180
    CourseSelectPuppeteerGroup* mPuppeteerGroup = nullptr;  // 0x188
    void* _190;
    ActorArray mFairies;  // 0x198
    ActorArray mFairyTools;  // 0x1a8
    al::CameraInfo* mCameraAfterEnding = nullptr;  // 0x1b8
    al::CameraInfo* mCameraAppearance = nullptr;  // 0x1c0
    al::CameraInfo* mCameraShot;  // 0x1c8
    sead::Matrix34f mDemoMtx = sead::Matrix34f::ident;  // 0x1d0
    al::LiveActor* mWaveRock = nullptr;  // 0x200
    al::LiveActor* mWaveBase = nullptr;  // 0x208
};

static_assert(sizeof(CourseSelectRocket) == 0x210);

/**
 * @brief Course-select controller of a rocket: lets the puppeteers and the director drive a
 * CourseSelectRocket through the ICourseSelectActorController interface.
 */
class RocketController : public ICourseSelectActorController {
public:
    /**
     * @brief Constructs the controller.
     * @param pRocket Controlled rocket.
     */
    explicit RocketController(CourseSelectRocket* pRocket) : mRocket(pRocket) {}

    /** @brief Gets the controlled actor. @return The rocket. */
    al::LiveActor* getActor() override { return mRocket; }

    /** @brief Gets the cursor layout shown over the rocket. @return Always 5. */
    s32 getCursorLayoutType() const override { return 5; }

    /** @brief Gets the course data of the rocket. @return Always nullptr: rockets have none. */
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }

    /** @brief Gets the branch node placed on the rocket. @return Always nullptr. */
    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }

    /**
     * @brief Starts a puppet demo on the rocket.
     * @param pGroup Puppeteer group playing the demo.
     */
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override {
        mRocket->startPuppetDemo(pGroup);
    }

    /** @brief Ends the puppet demo of the rocket. */
    void endPuppetDemo() override { mRocket->endPuppetDemo(); }

    /**
     * @brief Binds a player to the rocket.
     * @param pPuppeteer Puppeteer of the player.
     */
    void startBind(CourseSelectPuppeteer* pPuppeteer) override { mRocket->startBind(pPuppeteer); }

    /**
     * @brief Launches the rocket when the main player decides on it.
     * @param pDirector Course select director.
     * @return Whether the rocket demo started.
     */
    bool tryDecide(const CourseSelectDirector* pDirector) override {
        return mRocket->tryDecide(pDirector);
    }

    /** @brief Does nothing: the rocket opens with its own demo. */
    void startOpen() override {}

    /** @brief Does nothing: the rocket opens with its own demo. */
    void startOpenImmediately() override {}

    /** @brief Gets whether the open demo ended. @return Always true. */
    bool isEndOpen() const override { return true; }

private:
    CourseSelectRocket* mRocket;
};

static_assert(sizeof(RocketController) == 0x10);
