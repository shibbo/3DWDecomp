#pragma once

class NekoStateWaitParam {
public:
    NekoStateWaitParam(const char* pAction1, const char* pAction2, const char* pAction3,
                      const char* pAction4, const char* pAction5);

    const char* mAction2;
    const char* mAction3;
    const char* mAction1;
    const char* mAction4;
    const char* mAction5;
};
