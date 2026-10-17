#include "Raidon/PlayReport.hpp"
#include "Project/Account/AccountUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataPlayReportCommon.hpp"
#include <erepo/Data/Array.h>
#include <erepo/Data/SendData.h>
#include <erepo/Manager.h>
#include <heap/seadHeapMgr.h>
#include <thread/seadThreadUtil.h>

PlayReport* PlayReport::mInstance = nullptr;

/**
 * @brief Construct an uninitialized play report sender.
 */
PlayReport::PlayReport()
    : mpSendData(nullptr), mItemNum(0), mIsInitialized(false), mIsEventActive(false),
      mIsSaveDataRequested(false) {}

/**
 * @brief Flush the pending event, shut down and release the singleton.
 */
PlayReport::~PlayReport() {
    Save();
    mIsInitialized = false;

    if (mInstance != nullptr) {
        delete mInstance;
    }

    mInstance = nullptr;
}

/**
 * @brief Request the pending event data to be sent.
 */
void PlayReport::Save() {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    if (mpSendData != nullptr && !mpSendData->isSaved() && mpSendData->requestSave()) {
        mIsEventActive = false;
    }
}

/**
 * @brief Get the play report singleton, creating it on the stationed heap if needed.
 * @return The play report singleton.
 */
PlayReport* PlayReport::getInstance() {
    if (mInstance == nullptr) {
        sead::ScopedCurrentHeapSetter setter(al::getStationedHeap());
        mInstance = new PlayReport();
    }

    return mInstance;
}

/**
 * @brief Create and initialize the erepo manager.
 */
void PlayReport::Init() {
    if (mIsInitialized) {
        return;
    }

    erepo::Manager::createInstance(al::getStationedHeap());

    erepo::Manager::InitializeArg arg;
    arg.threadAffinity = sead::CoreIdMask(sead::CoreId::cMain);
    arg.threadPriority = sead::ThreadUtil::ConvertPrioritySeadToPlatform(17);
    arg.sendQueueSize = 0x80;
    arg.threadStackSize = 0x8000;
    arg.threadMessageQueueSize = 0x80;
    arg.isSystemReportTagEnabled = true;
    arg.uid = {};
    arg.heap = al::getStationedHeap();
    arg.heapSize = 0x30000;
    erepo::Manager::instance()->initialize(arg);
    mIsInitialized = true;
}

/**
 * @brief Start up the erepo manager for the current user account.
 * @param pHolder Game data holder the persistent counters are read from and written to.
 */
void PlayReport::InitAccount(GameDataHolder* pHolder) {
    if (!mIsInitialized) {
        return;
    }

    mpHolder = pHolder;

    const sead::SafeString& rEmpty = sead::SafeString::cEmptyString;
    erepo::Manager::StartupArg arg = {
        {0xffffffff, 0xffffffff}, rEmpty, {rEmpty, rEmpty}, false, true};
    erepo::Manager::instance()->startUp(arg);
    erepo::Manager::instance()->setUid(al::getUid());
}

/**
 * @brief Check whether the erepo manager is waiting for its save data.
 * @return True when a save is pending.
 */
bool PlayReport::isSaveNeeded() {
    return erepo::Manager::instance()->isSaveRequested();
}

/**
 * @brief Check whether the erepo manager is currently sending.
 * @return True while sending.
 */
bool PlayReport::isSending() {
    return erepo::Manager::instance()->isSending();
}

/**
 * @brief Service pending save/load requests or update the erepo manager.
 */
void PlayReport::Update() {
    erepo::Manager* pManager = erepo::Manager::instance();
    if (pManager == nullptr) {
        return;
    }

    erepo::Manager::UpdateArg arg;
    arg.deltaTime = 1.0f / 60.0f;

    if (pManager->isLoadRequested()) {
        GameDataHolder* pHolder = mpHolder;
        if (pHolder->mUnknown6A) {
            pHolder->mUnknown6B = true;
            GameDataPlayReportCommonValues& rValues = pHolder->mpPlayReportCommon->mValues;
            mpHolder->mUnknown6C = true;
            pManager->loadData(rValues.mValue0, rValues.mValue1, rValues.mValue2,
                               rValues.mValue3, rValues.mCounts0, rValues.mCounts1,
                               rValues.mCounts2, rValues.mCounts3);
            erepo::Manager::instance()->setLoadFinish();
        }

        return;
    }

    if (pManager->isSaveRequested()) {
        GameDataPlayReportCommonValues& rValues = mpHolder->mpPlayReportCommon->mValues;
        pManager->saveData(&rValues.mValue0, &rValues.mValue1, &rValues.mValue2,
                           &rValues.mValue3, rValues.mCounts0, rValues.mCounts1,
                           rValues.mCounts2, rValues.mCounts3);
        erepo::Manager::instance()->setSaveFinish();
        mIsSaveDataRequested = false;
        return;
    }

    pManager->update(arg);
}

/**
 * @brief Ask the erepo manager to write its save data.
 */
void PlayReport::requestSaveData() {
    erepo::Manager::instance()->requestSaveData();
    mIsSaveDataRequested = true;
}

/**
 * @brief Report the controller style currently in use.
 * @param style Current controller style.
 */
void PlayReport::UpdateStyle(erepo::EControllerStyle style) {
    erepo::Manager::instance()->setCurrentControllerStyle(style);
}

/**
 * @brief Begin a new event.
 * @param eventName Event identifier.
 * @param dataNum Number of data items the event will hold.
 * @param arrayNum Number of arrays (and structs) the event will hold.
 * @return True when the event could not be created.
 */
bool PlayReport::SetEventName(sead::FixedSafeString<31> eventName, int dataNum, int arrayNum) {
    if (!mIsInitialized) {
        return true;
    }

    mpSendData = erepo::SendData::CreateSendData(eventName, dataNum, arrayNum, arrayNum,
                                                 erepo::StringId(0), true);
    if (mpSendData == nullptr) {
        return true;
    }

    mItemNum = dataNum;
    mpSendData->addLog(sead::SafeString("SendKibana"), true);
    mIsEventActive = true;
    return false;
}

/**
 * @brief Add a signed 64-bit item to the current event.
 * @param pKey Item key.
 * @param value Item value.
 */
void PlayReport::AddItem(char* pKey, long value) {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    mpSendData->addData(erepo::KeyString(pKey), static_cast<s64>(value));
    mItemNum--;
}

/**
 * @brief Add an unsigned 64-bit item to the current event.
 * @param pKey Item key.
 * @param value Item value.
 */
void PlayReport::AddItem(char* pKey, unsigned long value) {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    mpSendData->addData(erepo::KeyString(pKey), static_cast<u64>(value));
    mItemNum--;
}

/**
 * @brief Add a 32-bit integer item to the current event.
 * @param pKey Item key.
 * @param value Item value.
 */
void PlayReport::AddItem(char* pKey, int value) {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    mpSendData->addData(erepo::KeyString(pKey), static_cast<s32>(value));
    mItemNum--;
}

/**
 * @brief Add a floating-point item to the current event.
 * @param pKey Item key.
 * @param value Item value.
 */
void PlayReport::AddItem(char* pKey, float value) {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    mpSendData->addData(erepo::KeyString(pKey), value);
    mItemNum--;
}

/**
 * @brief Add a string item to the current event.
 * @param pKey Item key.
 * @param rValue Item value.
 */
void PlayReport::AddItem(char* pKey, sead::SafeString& rValue) {
    if (!mIsInitialized && mpSendData == nullptr) {
        return;
    }

    mpSendData->addData(erepo::KeyString(pKey), rValue);
    mItemNum--;
}

/**
 * @brief Add an integer array item to the current event.
 * @param pKey Item key.
 * @param pValues Array values.
 * @param num Number of values.
 */
void PlayReport::AddItem(char* pKey, int* pValues, int num) {
    erepo::Array* pArray = mpSendData->CreateStructArrayWithBufferSize(num, 4);
    for (int i = 0; i < num; i++) {
        pArray->addData<int>(pValues[i]);
    }

    mpSendData->addData(erepo::KeyString(pKey), *pArray);
    mItemNum--;
}

/**
 * @brief Add a float array item to the current event, truncating each value to an integer.
 * @param pKey Item key.
 * @param pValues Array values.
 * @param num Number of values.
 */
void PlayReport::AddItem(char* pKey, float* pValues, int num) {
    erepo::Array* pArray = mpSendData->CreateStructArrayWithBufferSize(num, 4);
    for (int i = 0; i < num; i++) {
        int value = static_cast<int>(pValues[i]);
        pArray->addData<int>(value);
    }

    mpSendData->addData(erepo::KeyString(pKey), *pArray);
    mItemNum--;
}

/**
 * @brief Add the internet connection status to the current event.
 */
void PlayReport::sendNetworkStatus() {
    mpSendData->addInternetConnectionStatus();
}

/**
 * @brief Add the play session id to the current event.
 */
void PlayReport::addSessionId() {
    mpSendData->addSessionId();
}

/**
 * @brief Handle a play report error (no-op).
 * @param rResult Error result.
 */
void PlayReport::HandleError(nn::Result& rResult) {}

/**
 * @brief Shut down the play report sender.
 */
void PlayReport::Quit() {
    if (mIsInitialized) {
        mIsInitialized = false;
    }
}

namespace {

/**
 * @brief Event and key names used by the play reports.
 */
sead::FixedSafeString<31> sEventNames[] = {
    sead::SafeString("options"),
    sead::SafeString("stage"),
    sead::SafeString("mode_start"),
    sead::SafeString("erepo_time"),
    sead::SafeString("player_location"),
    sead::SafeString("death"),
    sead::SafeString("damage"),
    sead::SafeString("get_shine"),
    sead::SafeString("get_shard"),
    sead::SafeString("get_item"),
    sead::SafeString("start_disaster"),
    sead::SafeString("end_disaster"),
    sead::SafeString("island_id"),
    sead::SafeString("island"),
    sead::SafeString("plessie_on"),
    sead::SafeString("plessie_off"),
    sead::SafeString("graffiti"),
    sead::SafeString("map_open"),
    sead::SafeString("map_close"),
    sead::SafeString("player_transformed"),
    sead::SafeString("phase_clear"),
    sead::SafeString("boss"),
    sead::SafeString("amiibo"),
    sead::SafeString("network_stats"),
    sead::SafeString("network_error"),
};

} // namespace
