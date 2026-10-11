#include "MapObj/GeneratorBoxChild.hpp"
#include "MapObj/GeneratorBox.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(GeneratorBoxChild, Appear);
    NERVE_DECL(GeneratorBoxChild, Disappear);
    NERVE_DECL(GeneratorBoxChild, DisappearSign);
    NERVE_DECL(GeneratorBoxChild, Bound);
    NERVE_DECL(GeneratorBoxChild, Wait);
    NERVES_MAKE_NOSTRUCT(GeneratorBoxChild, Appear, Disappear, DisappearSign, Bound, Wait)
}
GeneratorBoxChild::GeneratorBoxChild(const char* name) : al::LiveActor(name) {}
GeneratorBoxChild::~GeneratorBoxChild() {}
void GeneratorBoxChild::initWithArchive(const al::ActorInitInfo& info, const char* archive, const char* suffix) {
    al::initActorWithArchiveName(this, info, archive, suffix);
    al::initNerve(this, &NrvGeneratorBoxChildAppear, 0);
    al::invalidateClipping(this);
    makeActorDead();
}
bool GeneratorBoxChild::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerGiantAttack(msg) && !al::isSingleMode(this)) {
        requestBreak();
    } else if (rc::isMsgForBlockAll(msg, sender, receiver, 100.0f) && al::isSensorPlayer(sender)) {
        if (mReactionTime == 0) {
            al::startHitReactionHit(this);
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            if (al::isMsgPlayerUpperPunch(msg)) mReactionTime = 5;
            else mReactionTime = 30;
        }
    } else if (al::isMsgPlayerHipDropAll(msg) && mHost->getChildCount() - 1 >= 0 && isTop() && !mHost->isReacting()) {
        return mHost->receiveMsg(msg, sender, receiver);
    }
    return false;
}
void GeneratorBoxChild::requestBreak() {
    if (al::isDead(this)) return;
    al::startHitReactionBreak(this);
    if (al::isVisAnimPlaying(this, "Blink")) al::setVisAnimFrameAndStop(this, 0.0f);
    kill();
}
bool GeneratorBoxChild::isTop() { return mHost->getChild(mHost->getChildCount() - 1) == this; }
void GeneratorBoxChild::appear() {
    al::setTrans(this, al::getTrans(mHost));
    al::LiveActor::makeActorAppeared();
    al::setNerve(this, &NrvGeneratorBoxChildAppear);
    if (mHost->isBlinkingTime()) {
        float frame = al::getVisAnimFrame(mHost->getChild(0));
        float maxFrame = al::getVisAnimFrameMax(mHost->getChild(0));
        frame += 1.0f;
        if (frame > maxFrame) frame -= maxFrame;
        al::startVisAnim(this, "Blink");
        al::setVisAnimFrame(this, frame);
    }
}
void GeneratorBoxChild::control() { if (mReactionTime - 1 >= 0) --mReactionTime; }
void GeneratorBoxChild::trySetDisappear() {
    if (!al::isNerve(this, &NrvGeneratorBoxChildDisappear)) al::setNerve(this, &NrvGeneratorBoxChildDisappear);
}
bool GeneratorBoxChild::requestDisappearSign() {
    if (al::isNerve(this, &NrvGeneratorBoxChildDisappearSign)) return false;
    al::setNerve(this, &NrvGeneratorBoxChildDisappearSign);
    return true;
}
void GeneratorBoxChild::requestBound() {
    if (!al::isNerve(this, &NrvGeneratorBoxChildBound)) al::setNerve(this, &NrvGeneratorBoxChildBound);
}
void GeneratorBoxChild::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGeneratorBoxChildWait);
}
void GeneratorBoxChild::exeBound() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGeneratorBoxChildWait);
}
void GeneratorBoxChild::exeDisappear() {
    al::startHitReactionDisappear(this);
    if (al::isVisAnimPlaying(this, "Blink")) al::setVisAnimFrameAndStop(this, 0.0f);
    al::resetPosition(this, al::getTrans(mHost), false);
    kill();
}
void GeneratorBoxChild::exeDisappearSign() {
    if (al::isFirstStep(this)) al::startVisAnim(this, "Blink");
}
void GeneratorBoxChild::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
void GeneratorBoxChild::setChild(GeneratorBoxChild* child) { mChild = child; }
void GeneratorBoxChild::setParent(GeneratorBoxChild* parent) { mParent = parent; }
