#pragma once

#include <container/seadRingBuffer.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"

/**
 * @brief Ring buffer of the weapons a Bros enemy throws; the front weapon is the next one held.
 * @tparam T Weapon actor type, which provides a static getWeaponName().
 */
template <typename T>
class BrosWeaponArray : public sead::RingBuffer<T*> {
public:
    /**
     * @brief Creates and initializes the weapons.
     * @param pHost Bros enemy that owns the weapons.
     * @param rInfo Init info of the owner.
     * @param num Number of weapons.
     * @param isInitWithInfo Whether to initialize the weapons with their own init(), instead of as
     * actors without placement info.
     */
    BrosWeaponArray(al::LiveActor* pHost, const al::ActorInitInfo& rInfo, s32 num,
                    bool isInitWithInfo) {
        this->allocBuffer(num, nullptr);
        for (s32 i = 0; i < this->capacity(); i++) {
            T* weapon = new T(T::getWeaponName());
            if (isInitWithInfo) {
                weapon->init(rInfo);
            } else {
                al::initCreateActorNoPlacementInfoNoViewId(weapon, rInfo);
            }

            this->pushBack(weapon);
        }
    }

    /**
     * @brief Gets a weapon.
     * @param index Index from the front of the buffer.
     * @return The weapon.
     */
    T* getWeapon(s32 index) { return (*this)(index); }

    /**
     * @brief Takes the front weapon and moves it to the back of the buffer.
     * @return The weapon to hold next.
     */
    T* rotateNext() {
        T* weapon = this->front();
        this->popFront();
        this->pushBack(weapon);
        return weapon;
    }

    /**
     * @brief Checks whether any weapon is still alive (i.e. still flying).
     * @return Whether a weapon is alive.
     */
    bool isAnyAlive() {
        for (s32 i = 0; i < this->size(); i++) {
            if (al::isAlive(getWeapon(i))) {
                return true;
            }
        }

        return false;
    }
};
