#include "Layout/StaffRollLayoutHolder.hpp"

#include <math/seadVector.h>

#include "Layout/StaffRollFin.hpp"
#include "Layout/StaffRollJob.hpp"
#include "Layout/StaffRollName.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(StaffRollLayoutHolder, Hide);
NERVE_DECL(StaffRollLayoutHolder, Appear);
NERVE_DECL(StaffRollLayoutHolder, End);
NERVE_DECL(StaffRollLayoutHolder, PreAppear);
NERVE_DECL(StaffRollLayoutHolder, Fin);
NERVES_MAKE_NOSTRUCT(StaffRollLayoutHolder, Hide, Appear, End, PreAppear, Fin)

/** Number of job heading layouts that can be shown at once. */
constexpr s32 cJobLayoutNum = 6;
/** Number of name layouts that can be shown at once. */
constexpr s32 cNameLayoutNum = 24;
/** Maximum number of lines that can be on screen at once. */
constexpr s32 cLineMax = 30;
/** Maximum number of characters in one staff roll line. */
constexpr s32 cLineLength = 128;
/** Number of steps the staff roll waits before scrolling in multiplayer mode. */
constexpr s32 cPreAppearStep = 1065;
/** Step of the ending title state at which the title is closed. */
constexpr s32 cFinEndStep = 500;
}  // namespace

/**
 * @brief Reads the staff roll text, classifies its lines and creates the line layouts.
 * @param rInfo Layout initialization context.
 * @param isSingleMode Whether the single player ending (with the image layout) is shown.
 */
StaffRollLayoutHolder::StaffRollLayoutHolder(const al::LayoutInitInfo& rInfo, bool isSingleMode)
    : al::NerveExecutor("スタッフロール管理"), mMessageSystem(rInfo.getMessageSystem()),
      mIsSingleMode(isSingleMode) {
    initNerve(&NrvStaffRollLayoutHolderHide, 0);
    mLineNum = al::countMessageLine(al::getSystemMessageString(this, "StaffRoll", "StaffRoll"));

    mLineInfos = new LineInfoBuffer;
    mLineInfos->allocBuffer(mLineNum, nullptr);
    mLines = new LineBuffer;
    mLines->allocBuffer(cLineMax, nullptr);
    mNames = new NameBuffer;
    mNames->allocBuffer(cNameLayoutNum, nullptr);
    mJobs = new JobBuffer;
    mJobs->allocBuffer(cJobLayoutNum, nullptr);

    for (s32 i = 0; i < mLineNum; i++) {
        char16_t line[cLineLength];
        al::getMessageLine(line, cLineLength,
                           al::getSystemMessageString(this, "StaffRoll", "StaffRoll"), i);

        LineType type;
        switch (line[0]) {
        case u'@':
            type = LineType_Job;
            break;
        case u'\0':
        case u'\n':
        case u' ':
            type = LineType_Space;
            break;
        default:
            type = LineType_Name;
            break;
        }

        mLineInfos->pushBack({type, mJobNum, mNameNum, mSpaceNum});

        switch (type) {
        case LineType_Job:
            mJobNum++;
            break;
        case LineType_Name:
            mNameNum++;
            break;
        case LineType_Space:
            mSpaceNum++;
            break;
        default:
            break;
        }
    }

    for (s32 i = 0; i < cJobLayoutNum; i++) {
        mJobs->pushBack(new StaffRollJob(rInfo));
    }

    for (s32 i = 0; i < cNameLayoutNum; i++) {
        mNames->pushBack(new StaffRollName(rInfo));
    }

    if (mIsSingleMode) {
        mImageLayout = new al::LayoutActor("StaffRollImage");
        al::initLayoutActor(mImageLayout, rInfo, "StaffRollImage", nullptr);
    }

    mFin = new StaffRollFin(rInfo);
}

/**
 * @brief Splits off the job marker of a staff roll line.
 * @param pJobString Receives the displayed text (the line without its leading '@' for jobs).
 * @param pLine Staff roll line.
 * @return Whether the line is a job heading.
 */
bool StaffRollLayoutHolder::getJobString(char16_t** pJobString, char16_t* pLine) {
    bool isJob = pLine[0] == u'@';
    *pJobString = isJob ? pLine + 1 : pLine;
    return isJob;
}

/** @brief Waits for the music intro (skipped in single mode) before starting to scroll. */
void StaffRollLayoutHolder::exePreAppear() {
    if (mIsSingleMode || al::isGreaterEqualStep(this, cPreAppearStep)) {
        al::setNerve(this, &NrvStaffRollLayoutHolderAppear);
    }
}

/** @brief Shows the lines entering the screen and scrolls everything. */
void StaffRollLayoutHolder::exeAppear() {
    for (s32 i = 0; i < mLineNum; i++) {
        al::LayoutActor* actor = tryPushStaffRollLine(i);
        if (actor != nullptr) {
            actor->appear();
        }
    }

    if (al::isFirstStep(this) && mIsSingleMode) {
        al::setLocalTrans(mImageLayout, sead::Vector2f::zero);
        s32 imageNum = al::getPaneChildNum(mImageLayout, "Image");
        f32 posY = -720.0f;
        for (s32 i = 0; i < imageNum; i++) {
            s32 imageNo = i + 1;
            if (imageNo == 8) {
                posY += -170.0f;
            }

            const char* paneName = al::StringTmp<32>("PicImage%02d", imageNo).cstr();
            sead::Vector3f trans = al::getPaneLocalTrans(mImageLayout, paneName);
            al::setPaneLocalTrans(mImageLayout, paneName, {trans.x, posY, 0.0f});
            posY += -528.0f;
        }

        mImageLayout->appear();
    }

    updateScroll();
}

/**
 * @brief Shows a staff roll line if it is on screen and not shown yet.
 * @param lineIndex Index of the message line.
 * @return The layout now showing the line, or nullptr if nothing was shown.
 */
al::LayoutActor* StaffRollLayoutHolder::tryPushStaffRollLine(s32 lineIndex) {
    StaffRollLineInfo info = (*mLineInfos)[lineIndex];
    if (lineIndex < 0 || lineIndex >= mLineNum) {
        return nullptr;
    }

    if (isOutOfScreen(calcLayoutPos(lineIndex), getHeight(info))) {
        return nullptr;
    }

    if (isInclude(lineIndex)) {
        return nullptr;
    }

    char16_t line[cLineLength];
    al::getMessageLine(line, cLineLength, al::getSystemMessageString(this, "StaffRoll", "StaffRoll"),
                       lineIndex);

    char16_t* text;
    al::LayoutActor* actor;
    if (getJobString(&text, line)) {
        StaffRollJob* job = getVacantStaffRollJob();
        if (job == nullptr) {
            return nullptr;
        }

        job->setStringW(text);
        actor = job;
    } else if (line[0] == u'\0' || line[0] == u'\n' || line[0] == u' ') {
        StaffRollName* name = getVacantStaffRollName();
        if (name == nullptr) {
            return nullptr;
        }

        name->setStringW(u" ");
        actor = name;
    } else {
        StaffRollName* name = getVacantStaffRollName();
        if (name == nullptr) {
            return nullptr;
        }

        name->setStringW(text);
        actor = name;
    }

    mLines->pushBack({actor, lineIndex});
    actor->appear();
    return actor;
}

/** @brief Moves the shown lines and hides the ones that left the screen. */
void StaffRollLayoutHolder::updateScroll() {
    updateStaffRollTrans();
    for (s32 i = 0; i < mLines->size(); i++) {
        tryPopStaffRollString(i);
    }

    if (mIsSingleMode) {
        updateImageLayoutTrans();
    }
}

/** @brief Places every shown line at its current scroll position. */
void StaffRollLayoutHolder::updateStaffRollTrans() {
    for (s32 i = 0; i < mLines->size(); i++) {
        const StaffRollLine& line = (*mLines)[i];
        al::LayoutActor* actor = line.actor;
        sead::Vector2f trans(0.0f, calcLayoutPos(line.lineIndex));
        al::setLocalTrans(actor, trans);
    }
}

/**
 * @brief Hides a shown line if it left the screen or its layout was killed.
 * @param index Index in the shown line buffer.
 * @return Whether the line was removed.
 */
bool StaffRollLayoutHolder::tryPopStaffRollString(s32 index) {
    if (index < 0 || index >= mLines->size()) {
        return false;
    }

    const StaffRollLine& line = (*mLines)[index];
    al::LayoutActor* actor = line.actor;
    s32 lineIndex = line.lineIndex;
    f32 height = getHeight((*mLineInfos)[lineIndex]);
    if (!isOutOfScreen(calcLayoutPos(lineIndex), height) && actor->isAlive()) {
        return false;
    }

    actor->kill();
    mLines->remove(index);
    return true;
}

/** @brief Scrolls the single mode image layout. */
void StaffRollLayoutHolder::updateImageLayoutTrans() {
    sead::Vector3f trans = al::getLocalTrans(mImageLayout);
    trans.y += getImageScrollSpeed();
    al::setLocalTrans(mImageLayout, trans);
}

/** @brief Shows the ending title and closes it after a while. */
void StaffRollLayoutHolder::exeFin() {
    if (al::isFirstStep(this)) {
        mFin->appear();
    }

    if (al::isStep(this, cFinEndStep)) {
        mFin->startEnd();
    }
}

/** @brief Does nothing; the staff roll has ended. */
void StaffRollLayoutHolder::exeEnd() {}

/** @brief Does nothing; the staff roll is hidden. */
void StaffRollLayoutHolder::exeHide() {}

/** @brief Updates the current state. */
void StaffRollLayoutHolder::update() {
    updateNerve();
}

/**
 * @brief Checks whether the staff roll has ended.
 * @return Whether the end state is active.
 */
bool StaffRollLayoutHolder::isEnd() const {
    return al::isNerve(this, &NrvStaffRollLayoutHolderEnd);
}

/** @brief Hides every shown line and the ending title. */
void StaffRollLayoutHolder::setup() {
    while (mLines->size() > 0) {
        mLines->front().actor->kill();
        mLines->popFront();
    }

    mFin->kill();
    updateStaffRollTrans();
}

/** @brief Resets the staff roll and starts it. */
void StaffRollLayoutHolder::startAppear() {
    setup();
    al::setNerve(this, &NrvStaffRollLayoutHolderPreAppear);
}

/** @brief Starts showing the ending title. */
void StaffRollLayoutHolder::startFin() {
    al::setNerve(this, &NrvStaffRollLayoutHolderFin);
}

/**
 * @brief Calculates the current vertical position of a staff roll line.
 * @param lineIndex Index of the message line.
 * @return The scrolled layout position.
 */
f32 StaffRollLayoutHolder::calcLayoutPos(s32 lineIndex) {
    const StaffRollLineInfo& info = (*mLineInfos)[lineIndex];
    f32 basePos =
        -360.0f - calcLocalLayoutPos(info.jobNum, info.nameNum, info.spaceNum, info.type) + 0.0f;
    return basePos + al::getNerveStep(this) * getScrollSpeed();
}

/**
 * @brief Checks whether a line is outside of the screen.
 * @param pos Vertical position of the line.
 * @param height Height of the line.
 * @return Whether the line is fully above or below the screen.
 */
bool StaffRollLayoutHolder::isOutOfScreen(f32 pos, f32 height) {
    return pos > height + 359.0f || pos < -359.0f - height;
}

/**
 * @brief Gets the height of a staff roll line.
 * @param rInfo Line information.
 * @return The line height.
 */
f32 StaffRollLayoutHolder::getHeight(const StaffRollLineInfo& rInfo) {
    if (rInfo.type == LineType_Job || rInfo.type == LineType_Name) {
        return 30.0f;
    }

    return rInfo.type == LineType_Space ? 50.0f : 0.0f;
}

/**
 * @brief Checks whether a message line is currently shown.
 * @param lineIndex Index of the message line.
 * @return Whether a layout shows the line.
 */
bool StaffRollLayoutHolder::isInclude(s32 lineIndex) {
    for (s32 i = 0; i < mLines->size(); i++) {
        if ((*mLines)[i].lineIndex == lineIndex) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Finds a job heading layout that is not shown.
 * @return A hidden job layout, or nullptr if all are in use.
 */
StaffRollJob* StaffRollLayoutHolder::getVacantStaffRollJob() {
    for (s32 i = 0; i < mJobs->size(); i++) {
        StaffRollJob* job = (*mJobs)[i];
        if (!job->isAlive()) {
            return job;
        }
    }

    return nullptr;
}

/**
 * @brief Finds a name layout that is not shown.
 * @return A hidden name layout, or nullptr if all are in use.
 */
StaffRollName* StaffRollLayoutHolder::getVacantStaffRollName() {
    for (s32 i = 0; i < mNames->size(); i++) {
        StaffRollName* name = (*mNames)[i];
        if (!name->isAlive()) {
            return name;
        }
    }

    return nullptr;
}

/**
 * @brief Calculates the unscrolled position of a line from the lines above it.
 * @param jobNum Number of job lines above the line.
 * @param nameNum Number of name lines above the line.
 * @param spaceNum Number of empty lines above the line.
 * @param type LineType of the line.
 * @return The distance from the top of the staff roll.
 */
f32 StaffRollLayoutHolder::calcLocalLayoutPos(s32 jobNum, s32 nameNum, s32 spaceNum, s32 type) {
    f32 pos = jobNum * 50.0f + nameNum * 36.0f + spaceNum * 50.0f;
    if (type == LineType_Job) {
        pos += 15.0f;
    } else if (type == LineType_Name) {
        pos += 15.0f;
    }

    return pos;
}
