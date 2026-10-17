#pragma once
#include <basis/seadTypes.h>
class GameDataHolder;
class PlayLogData;
namespace sead {
class ReadStream;
class WriteStream;
} // namespace sead
class GameDataCommon {
  public:
    explicit GameDataCommon(GameDataHolder* pHolder);
    void initializeData();
    void updatePlayReportCommonVersion();
    bool checkValid();
    bool readFromStream(sead::ReadStream* pStream);
    void writeToStream(sead::WriteStream* pStream) const;

    /**
     * @brief Read the game mode (3D World or Bowser's Fury) played last.
     * @return The raw GameMode value.
     */
    u32 getLastPlayedMode() const { return mValues[0]; }

  private:
    friend class GameDataHolder;
    u32 mValues[3];
    PlayLogData* mpPlayLog;
    bool mState;
    u8 mPlayReportVersion;
};
static_assert(sizeof(GameDataCommon) == 0x20);
