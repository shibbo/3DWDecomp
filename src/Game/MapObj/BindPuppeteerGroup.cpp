#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
BindPuppeteerGroup::BindPuppeteerGroup(const char* name, s32 maxNum) : mName(name) { mPuppeteers.allocBuffer(maxNum, nullptr); }
void BindPuppeteerGroup::update() {
    for (int i = 0; i < mPuppeteers.size(); ++i) getPuppeteer(i)->updateNerve();
}
void BindPuppeteerGroup::registerPuppeteer(BindPuppeteer* puppet) { mPuppeteers.pushBack(puppet); }
void BindPuppeteerGroup::insertPuppeteer(s32 index, BindPuppeteer* puppet) { mPuppeteers.insert(index, puppet); }
void BindPuppeteerGroup::createAndFillPuppeteer(const char* name) {
    for (int i = 0; i < mPuppeteers.capacity(); ++i) registerPuppeteer(new BindPuppeteer(name));
}
bool BindPuppeteerGroup::isBindingSameUserId(const al::HitSensor* sensor) const {
    for (int i = 0; i < mPuppeteers.size(); ++i) {
        if (isBinding(i) && getPuppeteer(i)->getControlUserId() == rc::findControlUserId(sensor)) return true;
    }
    return false;
}
bool BindPuppeteerGroup::isEndBindAll() const {
    for (int i = 0; i < mPuppeteers.size(); ++i) if (isBinding(i)) return false;
    return true;
}
bool BindPuppeteerGroup::isBinding(s32 index) const { return mPuppeteers.unsafeAt(index)->isBind(); }
BindPuppeteer* BindPuppeteerGroup::getPuppeteer(s32 index) const { return mPuppeteers[index]; }
s32 BindPuppeteerGroup::getBindingIndex(const BindPuppeteer* puppet) const {
    for (int i = 0; i < mPuppeteers.size(); ++i) if (isBinding(i) && getPuppeteer(i) == puppet) return i;
    return -1;
}
s32 BindPuppeteerGroup::getBindingIndex(const al::HitSensor* sensor) const {
    BindPuppeteer* puppet = getPuppeteer(sensor);
    return puppet ? getBindingIndex(puppet) : -1;
}
BindPuppeteer* BindPuppeteerGroup::getPuppeteer(const al::HitSensor* sensor) const {
    for (int i = 0; i < mPuppeteers.size(); ++i) {
        if (isBinding(i) && rc::isPuppetSensor(getPuppeteer(i)->getPlayerPuppet(), sensor)) return getPuppeteer(i);
    }
    return nullptr;
}
void BindPuppeteerGroup::tryEndBindAll() {
    for (int i = 0; i < mPuppeteers.size(); ++i) if (isBinding(i)) mPuppeteers.unsafeAt(i)->endBind(nullptr);
}
void BindPuppeteerGroup::tryCancelBind(const al::HitSensor* sensor) {
    for (int i = 0; i < mPuppeteers.size(); ++i) {
        if (isBinding(i) && rc::isPuppetSensor(getPuppeteer(i)->getPlayerPuppet(), sensor)) mPuppeteers.unsafeAt(i)->cancelBind();
    }
}
void BindPuppeteerGroup::tryCancelBindAll() {
    for (int i = 0; i < mPuppeteers.size(); ++i) if (isBinding(i)) mPuppeteers.unsafeAt(i)->cancelBind();
}
IUsePlayerPuppet* BindPuppeteerGroup::getPlayerPuppet(s32 index) const { return getPuppeteer(index)->getPlayerPuppet(); }
BindPuppeteer* BindPuppeteerGroup::getPuppeteerByPlayerIndex(const al::HitSensor* sensor) const { return getPuppeteer(alPlayerFunction::findPlayerHolderIndex(sensor)); }
BindPuppeteer* BindPuppeteerGroup::getPuppeteerNoBind() const {
    for (int i = 0; i < mPuppeteers.size(); ++i) if (!isBinding(i)) return getPuppeteer(i);
    return nullptr;
}
s32 BindPuppeteerGroup::calcBindingPuppeteerNum() const {
    int count = 0;
    for (int i = 0; i < mPuppeteers.size(); ++i) if (isBinding(i)) ++count;
    return count;
}
void BindPuppeteerGroup::setNullPlayerPuppet(s32 index) { getPuppeteer(index)->setNullPlayerPuppet(); }
void BindPuppeteerGroup::erasePuppeteer(BindPuppeteer* puppet) { mPuppeteers.erase(mPuppeteers.indexOf(puppet)); }
