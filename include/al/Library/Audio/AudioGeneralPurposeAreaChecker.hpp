#pragma once
namespace al {
class AreaObjDirector;
class PlayerHolder;
class AudioGeneralPurposeAreaChecker {
  public:
    explicit AudioGeneralPurposeAreaChecker(const char* name);
    void init(AreaObjDirector* director);
    void setPlayerHolder(const PlayerHolder* holder);
    void update(int playerIndex);
    bool isInArea() const;
    bool isEnter() const { return mEntered; }
    bool isExit() const { return mExited; }

  private:
    unsigned char _0[0x38];
    bool mEntered;
    bool mExited;
    unsigned char _3a[6];
};
static_assert(sizeof(AudioGeneralPurposeAreaChecker) == 0x40);
} // namespace al
