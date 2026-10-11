#include "MapObj/MusicalItem.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
    NERVE_ACTION_IMPL(MusicalItem, Wait)
    NERVE_ACTIONS_MAKE_STRUCT(MusicalItem, Wait)
}
MusicalItem::MusicalItem(const char* pName) : al::LiveActor(pName) {
    mInstrumentType = !al::isEqualString(pName, "楽器アイテムA");
}
MusicalItem::~MusicalItem() {}
void MusicalItem::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Wait", &NrvMusicalItem.collector, 0);
    al::initActor(this, rInfo);
    makeActorAppeared();
}
void MusicalItem::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::startNerveAction(this, "Wait");
}
bool MusicalItem::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgItemGetAll(pMsg)) {
        al::startSe(this, "プログラムコール", nullptr);
        kill();
        return true;
    }
    return false;
}
void MusicalItem::exeWait() { al::addRotateAndRepeatY(this, 2.0f); }
