#pragma once
#include <basis/seadTypes.h>
class GameDataHolder;
namespace sead {
class ReadStream;
class WriteStream;
} // namespace sead

/**
 * @brief Common play-report counters shared by every save file.
 */
struct GameDataPlayReportCommonValues {
    u32 mValue0;
    u32 mValue1;
    u32 mValue2;
    u32 mValue3;
    u32 mCounts0[5];
    u32 mCounts1[5];
    u32 mCounts2[5];
    f32 mCounts3[4];
};
static_assert(sizeof(GameDataPlayReportCommonValues) == 0x5c);

class GameDataPlayReportCommon {
  public:
    explicit GameDataPlayReportCommon(GameDataHolder* pHolder);
    void initializeData();
    bool checkValid();
    bool readFromStream(sead::ReadStream* pStream);
    void writeToStream(sead::WriteStream* pStream) const;

  private:
    friend class PlayReport;

    GameDataPlayReportCommonValues mValues;
};
static_assert(sizeof(GameDataPlayReportCommon) == 0x5c);
