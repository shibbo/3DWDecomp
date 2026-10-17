#include "Enemy/ModelHolderActor.hpp"

/** @brief Constructs a model actor that forwards sensor interactions to its owner.
 * @param pName Actor name.
 * @param pOwner Actor that handles the model's sensor interactions; must be valid.
 */
ModelHolderActor::ModelHolderActor(const char* pName, al::LiveActor* pOwner)
    : al::LiveActor(pName), mOwner(pOwner) {}

/** @brief Forwards an outgoing sensor attack to the owner.
 * @param pSelf Sensor belonging to this model.
 * @param pOther Sensor contacted by this model.
 */
void ModelHolderActor::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mOwner->attackSensor(pSelf, pOther);
}

/** @brief Forwards an incoming sensor message to the owner.
 * @param pMsg Message to handle.
 * @param pOther Sending sensor.
 * @param pSelf Receiving sensor.
 * @return Whether the owner accepted the message.
 */
bool ModelHolderActor::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    return mOwner->receiveMsg(pMsg, pOther, pSelf);
}

/** @brief Forwards a screen pointer message to the owner.
 * @param pMsg Message to handle.
 * @param pPointer Pointer interacting with the model.
 * @param pTarget Target receiving the interaction.
 * @return Whether the owner accepted the message.
 */
bool ModelHolderActor::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                             al::ScreenPointer* pPointer,
                                             al::ScreenPointTarget* pTarget) {
    return mOwner->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
}
