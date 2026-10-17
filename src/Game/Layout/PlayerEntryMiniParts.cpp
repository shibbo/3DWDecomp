#include "Layout/PlayerEntryMiniParts.hpp"

#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(PlayerEntryMiniParts, Wait);
NERVES_MAKE_NOSTRUCT(PlayerEntryMiniParts, Wait)
}

/** @brief Creates compact player-entry parts with an initially empty left icon.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent layout.
 * @param port Controller port. */
PlayerEntryMiniParts::PlayerEntryMiniParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                         const char* pPartsName, al::LayoutActor* pParent, int port)
    : al::LayoutActor(pName), mPort(port) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvPlayerEntryMiniPartsWait, 0);
    al::setPaneString(this, "TxtCharacterL", u"", 0, -1);
}

/** @brief Holds the current player-entry appearance. */
void PlayerEntryMiniParts::exeWait() {}

/** @brief Selects controller, character-font and color animations.
 * @param characterId Character index, or minus one for the special color. */
void PlayerEntryMiniParts::setupController(int characterId) {
    switch (mPort) {
    case 1: al::startAction(this, "RemoconNum1", "Controller"); break;
    case 2: al::startAction(this, "RemoconNum2", "Controller"); break;
    case 3: al::startAction(this, "RemoconNum3", "Controller"); break;
    case 5: al::startAction(this, "RemoconNum5", "Controller"); break;
    }
    if (mIsKinopioBrigade) {
        al::startAction(this, "ColorMario", "Color");
        al::setPaneString(this, "TxtCharacterL", sead::WFormatFixedSafeString<6>(
            u"%s", LayoutFontUtil::getPictureFontKinopioBrigade(mPort - 1)).cstr(), 0, -1);
        al::setPaneString(this, "TxtCharacterR", sead::WFormatFixedSafeString<6>(
            u"%s", LayoutFontUtil::getPictureFontKinopioBrigade(mPort - 1)).cstr(), 0, -1);
    } else {
        al::setPaneString(this, "TxtCharacterL", sead::WFormatFixedSafeString<6>(
            u"%s", LayoutFontUtil::getPictureFontPlayer(characterId)).cstr(), 0, -1);
        al::setPaneString(this, "TxtCharacterR", sead::WFormatFixedSafeString<6>(
            u"%s", LayoutFontUtil::getPictureFontPlayer(characterId)).cstr(), 0, -1);
        switch (characterId) {
        case -1: al::startAction(this, "ColorSpecial", "Color"); break;
        case 0: al::startAction(this, "ColorMario", "Color"); break;
        case 1: al::startAction(this, "ColorLuigi", "Color"); break;
        case 2: al::startAction(this, "ColorPeach", "Color"); break;
        case 3: al::startAction(this, "ColorKinopio", "Color"); break;
        case 4: al::startAction(this, "ColorRosetta", "Color"); break;
        }
    }
}

/** @brief Uses Toad Brigade character icons for this entry. */
void PlayerEntryMiniParts::setStageKinopioBrigade() { mIsKinopioBrigade = true; }

/** @brief Configures the character and plays the requested main animation.
 * @param characterId Character index.
 * @param pAnimName Main animation name. */
void PlayerEntryMiniParts::appearWithAnimName(int characterId, const char* pAnimName) {
    setupController(characterId);
    al::startAction(this, pAnimName, "Main");
}
