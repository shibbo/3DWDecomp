#include "Layout/MiiverseStateTimeLine.hpp"
#include "Layout/MiiverseBalloonParts.hpp"
#include "Layout/MiiverseStateDetail.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include <heap/seadExpHeap.h>
namespace {
NERVE_DECL(MiiverseStateTimeLine, Wait);
NERVES_MAKE_NOSTRUCT(MiiverseStateTimeLine, Wait)
}
/** @brief Creates six balloon parts and their detail state. */
MiiverseStateTimeLine::MiiverseStateTimeLine(al::LayoutActor* pParent, const al::LayoutInitInfo& rInfo, al::NetworkSystem* pNetwork, al::ErrorViewer* pErrorViewer)
    : al::NerveExecutor("Miiverseタイムライン表示ステート"), mParent(pParent) {
    for (int i = 0; i < 6; ++i)
        mBalloons[i] = new MiiverseBalloonParts(rInfo, "Miiverseバルーン", al::StringTmp<64>("ParBalloonMiiverse%d", i + 1).cstr(), mParent, 240.0f);
    mDetail = new MiiverseStateDetail(rInfo, pNetwork, pErrorViewer);
    mHeap = sead::ExpHeap::create(0x80000, "MiiverseHeap", nullptr, 8, sead::Heap::cHeapDirection_Forward, false);
    mDecompressFunctor = new al::FunctorV0M<MiiverseStateTimeLine*, void (MiiverseStateTimeLine::*)()>(this, &MiiverseStateTimeLine::decompress);
    initNerve(&NrvMiiverseStateTimeLineWait, 0);
}
/** @brief Online post decompression is unavailable in this version. */
void MiiverseStateTimeLine::decompress() {}
/** @brief Clears temporary data and starts the timeline animation. */
void MiiverseStateTimeLine::appear(const char* pTag) {
    mHeap->freeAll();
    mPage = 1;
    if (mBalloonCount >= 6) {
        al::startAction(mParent, "MiiverseWaitMove", "Memo");
        al::setActionFrameRate(mParent, 0.0f, "Memo");
    } else al::startAction(mParent, al::StringTmp<256>("MiiverseWait%d", mBalloonCount).cstr(), "Memo");
    al::setNerve(this, &NrvMiiverseStateTimeLineWait);
}
/** @brief Advances the timeline state. */
void MiiverseStateTimeLine::update() { updateNerve(); }
/** @brief Reports an idle timeline with no balloon being confirmed. */
bool MiiverseStateTimeLine::isWait() const {
    if (!al::isNerve(this, &NrvMiiverseStateTimeLineWait)) return false;
    for (int i = 0; i < mBalloonCount; ++i)
        if (mBalloons[i]->isDecide()) return false;
    return true;
}
/** @brief Advances the rotating balloons while touch actions permit it. */
void MiiverseStateTimeLine::exeWait() {
    if (mPostCount >= 6) {
        bool allWait = true;
        for (int i = 0; i < mBalloonCount; ++i) {
            if (!mBalloons[i]->isWait()) { allWait = false; break; }
        }
        if (allWait) {
            al::setActionFrameRate(mParent, 1.0f, "Memo");
            if (al::isActionEnd(mParent, "Memo")) {
                for (int i = 0; i < mBalloonCount; ++i) mBalloons[i]->reset();
                ++mPage;
                al::startAction(mParent, "MiiverseWaitMove", "Memo");
            }
        } else al::setActionFrameRate(mParent, 0.0f, "Memo");
    }
    for (int i = 0; i < mBalloonCount; ++i) {
        mBalloons[i]->movement();
        if (mBalloons[i]->isDecideEnd()) break;
    }
}
/** @brief Suspends balloon input until the detail view closes. */
void MiiverseStateTimeLine::exeDetail() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mBalloonCount; ++i) {
            mBalloons[i]->isDecideEnd();
            mBalloons[i]->setTouchDisable();
        }
    }
    mDetail->updateNerve();
    if (mDetail->isEnd()) {
        for (int i = 0; i < mBalloonCount; ++i) {
            mBalloons[i]->setTouchEnable();
            mBalloons[i]->reset();
        }
        al::setNerve(this, &NrvMiiverseStateTimeLineWait);
    }
}
