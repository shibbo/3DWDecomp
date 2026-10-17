#pragma once

#include <basis/seadTypes.h>
#include <erepo/Types.h>
#include <prim/seadSafeString.h>

class GameDataHolder;

namespace erepo {
class SendData;
}

namespace nn {
class Result;
}

/**
 * @brief Game-side play report sender built on top of erepo.
 */
class PlayReport {
  public:
    PlayReport();
    ~PlayReport();

    void Save();
    static PlayReport* getInstance();
    void Init();
    void InitAccount(GameDataHolder* pHolder);
    bool isSaveNeeded();
    bool isSending();
    void Update();
    void requestSaveData();
    void UpdateStyle(erepo::EControllerStyle style);
    bool SetEventName(sead::FixedSafeString<31> eventName, int dataNum, int arrayNum);
    void AddItem(char* pKey, long value);
    void AddItem(char* pKey, unsigned long value);
    void AddItem(char* pKey, int value);
    void AddItem(char* pKey, float value);
    void AddItem(char* pKey, sead::SafeString& rValue);
    void AddItem(char* pKey, int* pValues, int num);
    void AddItem(char* pKey, float* pValues, int num);
    void sendNetworkStatus();
    void addSessionId();
    void HandleError(nn::Result& rResult);
    void Quit();

  private:
    static PlayReport* mInstance;

    erepo::SendData* mpSendData;
    s32 mItemNum;
    GameDataHolder* mpHolder;
    bool mIsInitialized;
    bool mIsEventActive;
    bool mIsSaveDataRequested;
};
static_assert(sizeof(PlayReport) == 0x20);
