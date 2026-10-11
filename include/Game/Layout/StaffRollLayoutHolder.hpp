#pragma once

#include <container/seadRingBuffer.h>

#include "Library/Message/IUseMessageSystem.hpp"
#include "Library/Nerve/NerveExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class LayoutActor;
class LayoutInitInfo;
}  // namespace al

class StaffRollFin;
class StaffRollJob;
class StaffRollName;

/**
 * Scrolls the staff roll (job/name lines and images) shown during the ending.
 */
class StaffRollLayoutHolder : public al::NerveExecutor,
                              public al::IUseMessageSystem,
                              public al::ISceneObj {
public:
    /** Kind of a staff roll message line, decided by its first character. */
    enum LineType : s32 {
        LineType_None = 0,
        LineType_Job = 1,    ///< Line starting with '@'.
        LineType_Name = 2,   ///< Any other non-empty line.
        LineType_Space = 3,  ///< Empty line or a line starting with a space.
    };

    /** Static information about one message line: its kind and how many lines precede it. */
    struct StaffRollLineInfo {
        LineType type = LineType_None;
        s32 jobNum = 0;
        s32 nameNum = 0;
        s32 spaceNum = 0;
    };

    /** A line currently shown on screen with the layout displaying it. */
    struct StaffRollLine {
        al::LayoutActor* actor = nullptr;
        s32 lineIndex = -1;
    };

    typedef sead::RingBuffer<StaffRollLineInfo> LineInfoBuffer;
    typedef sead::RingBuffer<StaffRollLine> LineBuffer;
    typedef sead::RingBuffer<StaffRollJob*> JobBuffer;
    typedef sead::RingBuffer<StaffRollName*> NameBuffer;

    StaffRollLayoutHolder(const al::LayoutInitInfo& rInfo, bool isSingleMode);

    /** @brief Destroys the holder (the layouts are owned by the layout kit). */
    ~StaffRollLayoutHolder() override {}

    /**
     * @brief Gets the message system used to read the staff roll text.
     * @return The message system taken from the layout init info.
     */
    const al::MessageSystem* getMessageSystem() const override { return mMessageSystem; }

    /**
     * @brief Gets the scene object name.
     * @return The debug name of the holder.
     */
    const char* getSceneObjName() const override { return "スタッフロール管理"; }

    bool getJobString(char16_t** pJobString, char16_t* pLine);
    void exePreAppear();
    void exeAppear();
    al::LayoutActor* tryPushStaffRollLine(s32 lineIndex);
    void updateScroll();
    void updateStaffRollTrans();
    bool tryPopStaffRollString(s32 index);
    void updateImageLayoutTrans();
    void exeFin();
    void exeEnd();
    void exeHide();
    void update();
    bool isEnd() const;
    void setup();
    void startAppear();
    void startFin();
    f32 calcLayoutPos(s32 lineIndex);
    bool isOutOfScreen(f32 pos, f32 height);
    f32 getHeight(const StaffRollLineInfo& rInfo);
    bool isInclude(s32 lineIndex);
    StaffRollJob* getVacantStaffRollJob();
    StaffRollName* getVacantStaffRollName();
    f32 calcLocalLayoutPos(s32 jobNum, s32 nameNum, s32 spaceNum, s32 type);

private:
    /** @return The scroll speed of the text lines per step. */
    f32 getScrollSpeed() const { return mIsSingleMode ? 1.36f : 1.7f; }

    /** @return The scroll speed of the image layout per step. */
    f32 getImageScrollSpeed() const { return mIsSingleMode ? 0.92f : 1.7f; }

    LineInfoBuffer* mLineInfos = nullptr;
    s32 mLineNum = 0;
    JobBuffer* mJobs = nullptr;
    NameBuffer* mNames = nullptr;
    StaffRollFin* mFin = nullptr;
    LineBuffer* mLines = nullptr;
    al::LayoutActor* mImageLayout = nullptr;
    const al::MessageSystem* mMessageSystem;
    s32 mJobNum = 0;
    s32 mNameNum = 0;
    s32 mSpaceNum = 0;
    bool mIsSingleMode;
};

static_assert(sizeof(StaffRollLayoutHolder) == 0x70);
