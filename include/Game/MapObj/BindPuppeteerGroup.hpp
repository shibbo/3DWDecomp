#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>

namespace al {
class HitSensor;
}  // namespace al

class BindPuppeteer;
class IUsePlayerPuppet;

/**
 * @brief A fixed-capacity list of bind puppeteers, one per player that can be bound.
 */
class BindPuppeteerGroup {
public:
    using PuppeteerArray = sead::PtrArray<BindPuppeteer>;

    BindPuppeteerGroup(const char* pName, s32 maxNum);

    void update();
    void createAndFillPuppeteer(const char*);
    bool isBindingSameUserId(const al::HitSensor*) const;
    bool isBinding(s32) const;
    s32 getBindingIndex(const BindPuppeteer*) const;
    s32 getBindingIndex(const al::HitSensor*) const;
    BindPuppeteer* getPuppeteer(const al::HitSensor*) const;
    void tryEndBindAll();
    void tryCancelBind(const al::HitSensor*);
    void tryCancelBindAll();
    IUsePlayerPuppet* getPlayerPuppet(s32) const;
    BindPuppeteer* getPuppeteerNoBind() const;
    s32 calcBindingPuppeteerNum() const;
    void setNullPlayerPuppet(s32);
    void registerPuppeteer(BindPuppeteer* pPuppeteer);
    void insertPuppeteer(s32 index, BindPuppeteer* pPuppeteer);
    bool isEndBindAll() const;
    BindPuppeteer* getPuppeteer(s32 index) const;
    BindPuppeteer* getPuppeteerByPlayerIndex(const al::HitSensor* pPlayerSensor) const;
    void erasePuppeteer(BindPuppeteer* pPuppeteer);

    /**
     * @brief Get a puppeteer as its concrete type.
     * @param index Index of the puppeteer in the group.
     * @return The puppeteer.
     */
    template <typename T>
    T* getPuppeteer(s32 index) const {
        return static_cast<T*>(getPuppeteer(index));
    }

    /**
     * @brief Get the puppeteer of a player as its concrete type.
     * @param pPlayerSensor Sensor of the player.
     * @return The puppeteer of the player.
     */
    template <typename T>
    T* getPuppeteerByPlayerIndex(const al::HitSensor* pPlayerSensor) const {
        return static_cast<T*>(getPuppeteerByPlayerIndex(pPlayerSensor));
    }

    /**
     * @brief Count the registered puppeteers.
     * @return Number of puppeteers in the group.
     */
    s32 getPuppeteerNum() const { return mPuppeteers.size(); }

    /**
     * @brief Get the capacity of the group.
     * @return Maximum number of puppeteers.
     */
    s32 getPuppeteerNumMax() const { return mPuppeteers.capacity(); }

    /// Forget every registered puppeteer (the puppeteers themselves are kept alive).
    void clearPuppeteer() { mPuppeteers.clear(); }

private:
    const char* mName;
    PuppeteerArray mPuppeteers;
};

static_assert(sizeof(BindPuppeteerGroup) == 0x18);
