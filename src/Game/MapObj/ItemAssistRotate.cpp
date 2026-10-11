#include "MapObj/ItemAssistRotateParam.hpp"

ItemAssistRotateParam::ItemAssistRotateParam()
    : mFrame(60), mSpeed(20.0f), mIsUseSubRotate(true), mSubSpeed(3.0f), mSubFrame(60) {}

ItemAssistRotateParam::ItemAssistRotateParam(s32 frame, f32 speed, bool isUseSubRotate,
                                           f32 subSpeed, s32 subFrame)
    : mFrame(frame), mSpeed(speed), mIsUseSubRotate(isUseSubRotate), mSubSpeed(subSpeed),
      mSubFrame(subFrame) {}
