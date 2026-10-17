#include "Layout/WindowMessage.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_DECL(WindowMessage, Appear);
NERVE_DECL(WindowMessage, Wait);
NERVE_DECL(WindowMessage, End);
NERVE_DECL(WindowMessage, PageEnd);
NERVE_DECL(WindowMessage, PageStart);
NERVES_MAKE_NOSTRUCT(WindowMessage, Appear, Wait, End, PageEnd, PageStart)
}
/** @brief Creates the message window and detects the active game mode. */
WindowMessage::WindowMessage(const al::LayoutInitInfo& rInfo, const char* pLayoutName,
                             const char* pName, const char* pGroupName)
    : al::LayoutActor(pName), mPort(al::getMainControllerPort()) {
    al::initLayoutActor(this, rInfo, pLayoutName, pGroupName);
    initNerve(&NrvWindowMessageAppear, 0);
    if (getSceneObjHolder() && al::tryGetSceneObj(this, 8))
        mIsSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(this));
    else mIsSingleMode = false;
    kill();
}
/** @brief Shows the text already stored in the message pane. */
void WindowMessage::appearWithCurrentMessage(int port) {
    al::startAction(this, "Appear", "Main");
    al::LayoutActor::appear();
    mMessage = al::getPaneStringBuffer(this, "TxtMessage");
    mPort = port;
    al::setNerve(this, &NrvWindowMessageAppear);
}
/** @brief Shows a message beginning with its first page. */
void WindowMessage::appearWithMessage(const char16_t* pMessage, int port) {
    al::startAction(this, "Appear", "Main");
    al::LayoutActor::appear();
    mMessage = pMessage;
    int size = al::calcMessageSizeWithoutNullCharacter(pMessage, nullptr);
    mPageCount = al::countMessagePage(this, mMessage, size);
    mCurrentPage = 0;
    mPort = port;
    al::setPaneString(this, "TxtMessage", mMessage, 0, 0);
    al::setNerve(this, &NrvWindowMessageAppear);
}
/** @brief Shows a localized system message and lowers Bowser's Fury background audio. */
void WindowMessage::appearWithSystemMessage(const char* pFile, const char* pMessage, int port) {
    if (mIsSingleMode) {
        al::changeBgmVolume(this, 0.35f, 10);
        alSeFunction::setRequestKeeperVolumeSetting(this, "メイン", "AtmosphereDemo", 30, true);
    }
    al::startAction(this, "Appear", "Main");
    al::LayoutActor::appear();
    mMessage = al::getSystemMessageString(this, pFile, pMessage);
    int size = al::calcSystemMessageCharacterNum(this, pFile, pMessage);
    mPageCount = al::countMessagePage(this, mMessage, size);
    mCurrentPage = 0;
    mPort = port;
    al::setPaneString(this, "TxtMessage", mMessage, 0, 0);
    al::setNerve(this, &NrvWindowMessageAppear);
}
/** @brief Returns the current page index. */
int WindowMessage::getCurrentPageNum() const { return mCurrentPage; }
/** @brief Plays the entrance and prepares the controller confirmation icon. */
void WindowMessage::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", "Main");
        al::tryStartSe(this, "AppearStart", nullptr);
        rc::setPaneDecideIconFont(this, "TxtIconA", mPort);
    }
    if (al::isGreaterStep(this, 30) && al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvWindowMessageWait);
}
/** @brief Advances the page or closes the window on confirmation. */
void WindowMessage::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", "Main");
    if (rc::isPadTriggerWindowClose(mPort)) {
        mMessage = al::getNextMessagePage(this, mMessage);
        ++mCurrentPage;
        if (mMessage && mCurrentPage < mPageCount) al::setNerve(this, &NrvWindowMessagePageEnd);
        else al::setNerve(this, &NrvWindowMessageEnd);
    }
}
/** @brief Animates the outgoing page. */
void WindowMessage::exePageEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "PageEnd", "Page");
    if (al::isActionEnd(this, "Page")) al::setNerve(this, &NrvWindowMessagePageStart);
}
/** @brief Sets the new page and animates it into view. */
void WindowMessage::exePageStart() {
    if (al::isFirstStep(this)) {
        al::setPaneString(this, "TxtMessage", mMessage, 0, 0);
        al::startAction(this, "PageStart", "Page");
    }
    if (al::isActionEnd(this, "Page")) al::setNerve(this, &NrvWindowMessageWait);
}
/** @brief Restores audio while closing the window. */
void WindowMessage::exeEnd() {
    if (al::isFirstStep(this)) {
        al::tryStartSe(this, "HideStart", nullptr);
        al::startAction(this, "End", "Main");
        if (mIsSingleMode) {
            al::changeBgmVolume(this, 1.0f, 35);
            alSeFunction::setRequestKeeperVolumeSetting(this, "メイン", "通常", 30, true);
        }
    }
    if (al::isActionEnd(this, nullptr)) kill();
}
