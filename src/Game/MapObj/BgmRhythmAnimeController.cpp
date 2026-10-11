#include "MapObj/BgmRhythmAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Bgm/BgmUtil.hpp"

struct BgmRhythmAnimeInfo {
    int type;
    const char* action;
    bool isLoop;
};

namespace {
    NERVE_DECL(BgmRhythmAnimeController, Wait);
    NERVE_DECL(BgmRhythmAnimeController, SyncRhythm);
    NERVES_MAKE_NOSTRUCT(BgmRhythmAnimeController, Wait, SyncRhythm)

    const BgmRhythmAnimeInfo sNormal[] = {
        {0, "NormalRight", true}, {1, "NormalLeft", true},
        {2, "NormalCenterHight", true}, {3, "NormalCenterLow", true},
        {4, "FillInCenter", true}, {5, "FillInRight", true},
        {6, "FillInLeft", true}, {7, "HalfDown", true},
        {8, "HalfUp", true}, {9, "FillInAll", true},
        {10, "HurryUp", false}, {11, "Goal", false},
        {12, "SuperGoal", false}, {13, "Wait", true},
    };
    const BgmRhythmAnimeInfo sReverse[] = {
        {0, "NormalLeft", true}, {1, "NormalRight", true},
        {2, "NormalCenterLow", true}, {3, "NormalCenterHight", true},
        {4, "FillInCenter", true}, {5, "FillInLeft", true},
        {6, "FillInRight", true}, {7, "HalfUp", true},
        {8, "HalfDown", true}, {9, "FillInAll", true},
        {10, "HurryUp", false}, {11, "Goal", false},
        {12, "SuperGoal", false}, {13, "Wait", true},
    };
    const BgmRhythmAnimeInfo sDefault = {-1, "Wait", true};
}

BgmRhythmAnimeController::BgmRhythmAnimeController(al::LiveActor* pActor, bool isNormal)
    : al::NerveExecutor("BGM リズム同期アニメコントローラ"), mActor(pActor), mIsNormal(isNormal) {
    initNerve(&NrvBgmRhythmAnimeControllerWait, 0);
    al::startAction(mActor, "Wait");
}

void BgmRhythmAnimeController::update() {
    updateNerve();
}

void BgmRhythmAnimeController::exeWait() {
    if (al::isEnableRhythmAnim(mActor, nullptr) && al::isTriggerRhythmAnimChange(mActor)) {
        updateRhythmInfo();
        if (mInfo != nullptr) {
            updateAnimeFrame();
            al::setNerve(this, &NrvBgmRhythmAnimeControllerSyncRhythm);
        }
    }
}

void BgmRhythmAnimeController::updateRhythmInfo() {
    int type = al::getRhythmAnimType(mActor);
    const BgmRhythmAnimeInfo* info;
    if (mIsNormal) {
        if (type == 0) {
            info = &sNormal[0];
        } else switch (type) {
        case 1: info = &sNormal[1]; break;
        case 2: info = &sNormal[2]; break;
        case 3: info = &sNormal[3]; break;
        case 4: info = &sNormal[4]; break;
        case 5: info = &sNormal[5]; break;
        case 6: info = &sNormal[6]; break;
        case 7: info = &sNormal[7]; break;
        case 8: info = &sNormal[8]; break;
        case 9: info = &sNormal[9]; break;
        case 10: info = &sNormal[10]; break;
        case 11: info = &sNormal[11]; break;
        case 12: info = &sNormal[12]; break;
        case 13: info = &sNormal[13]; break;
        default: info = &sDefault; break;
        }
    } else {
        if (type == 0) {
            info = &sReverse[0];
        } else switch (type) {
        case 1: info = &sReverse[1]; break;
        case 2: info = &sReverse[2]; break;
        case 3: info = &sReverse[3]; break;
        case 4: info = &sReverse[4]; break;
        case 5: info = &sReverse[5]; break;
        case 6: info = &sReverse[6]; break;
        case 7: info = &sReverse[7]; break;
        case 8: info = &sReverse[8]; break;
        case 9: info = &sReverse[9]; break;
        case 10: info = &sReverse[10]; break;
        case 11: info = &sReverse[11]; break;
        case 12: info = &sReverse[12]; break;
        case 13: info = &sReverse[13]; break;
        default: info = &sDefault; break;
        }
    }
    mInfo = info;
    al::startAction(mActor, mInfo->action);
    mFrameMax = al::getSklAnimFrameMax(mActor, 0);
    mLastFrame = 0.0f;
}

// One-shot animations cannot rewind when the rhythm frame wraps.
void BgmRhythmAnimeController::updateAnimeFrame() {
    float frame = al::getRhythmAnimFrame(mActor);
    if (mInfo->isLoop) {
        frame = al::wrapValue(frame, mFrameMax);
    } else {
        mLastFrame = frame < mLastFrame ? mLastFrame : frame;
        frame = sead::Mathf::clamp(mLastFrame, 0.0f, mFrameMax);
    }
    al::setSklAnimFrameNoUpdate(mActor, frame, 0);
}

void BgmRhythmAnimeController::exePrepareWait() {
}

void BgmRhythmAnimeController::exeSyncRhythm() {
    if (!al::isEnableRhythmAnim(mActor, nullptr)) {
        al::startAction(mActor, "Wait");
        al::setNerve(this, &NrvBgmRhythmAnimeControllerWait);
        return;
    }
    if (al::isTriggerRhythmAnimChange(mActor)) {
        updateRhythmInfo();
    }
    if (mInfo != nullptr) {
        updateAnimeFrame();
    }
}
