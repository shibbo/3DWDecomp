#pragma once

#include "CourseSelect/CourseSelectFunction.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "System/Data/StageDatabaseInfo.hpp"

/**
 * @brief Course-select controller of a course miniature: lets the puppeteers and the director
 * drive a CourseSelectMiniature through the ICourseSelectActorController interface.
 */
class MiniatureController : public ICourseSelectActorController {
public:
    /**
     * @brief Constructs the controller.
     * @param pMiniature Controlled miniature.
     * @param pActorInfo Course data of the miniature.
     */
    MiniatureController(CourseSelectMiniature* pMiniature, CourseSelectActorInfo* pActorInfo)
        : mMiniature(pMiniature), mActorInfo(pActorInfo) {}

    /** @brief Gets the controlled actor. @return The miniature. */
    al::LiveActor* getActor() override { return mMiniature; }

    /**
     * @brief Gets the cursor layout shown over the miniature, by the number of green stars of the
     * course.
     * @return The cursor layout type.
     */
    s32 getCursorLayoutType() const override {
        if (!mActorInfo->isUseCourseInfo()) {
            return 5;
        }

        s32 greenStarNum = mActorInfo->getStageDatabaseInfo()->getGreenStarNum();
        if (greenStarNum > 29) {
            return 3;
        }

        return greenStarNum > 9 ? 2 : 1;
    }

    /** @brief Gets the course data of the miniature. @return The course data. */
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return mActorInfo; }

    /** @brief Gets the branch node placed on the miniature. @return The node. */
    CourseSelectNode* getCourseSelectNode() const override { return mMiniature->getNode(); }

    /** @brief Calculates the priority of the miniature among the opened nodes. @return The priority. */
    s32 calcOpenNodePriority() const override { return rc::calcOpenNodePriority(mMiniature); }

    /**
     * @brief Starts a puppet demo on the miniature.
     * @param pGroup Puppeteer group playing the demo.
     */
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override {
        mMiniature->startPuppetDemo(pGroup);
    }

    /** @brief Ends the puppet demo of the miniature. */
    void endPuppetDemo() override { mMiniature->endPuppetDemo(); }

    /**
     * @brief Does nothing: players are not bound to miniatures.
     * @param pPuppeteer Puppeteer of the player.
     */
    void startBind(CourseSelectPuppeteer* pPuppeteer) override {}

    /**
     * @brief Enters the course when the main player decides on the miniature.
     * @param pDirector Course select director.
     * @return Whether the course is entered.
     */
    bool tryDecide(const CourseSelectDirector* pDirector) override {
        return mMiniature->tryDecide(pDirector);
    }

    /** @brief Opens the course with its appear demo. */
    void startOpen() override { mMiniature->startAppear(); }

    /** @brief Does nothing: miniatures always open with a demo. */
    void startOpenImmediately() override {}

    /** @brief Gets whether the open demo ended. @return Always true. */
    bool isEndOpen() const override { return true; }

private:
    CourseSelectMiniature* mMiniature;
    CourseSelectActorInfo* mActorInfo;
};

static_assert(sizeof(MiniatureController) == 0x18);
