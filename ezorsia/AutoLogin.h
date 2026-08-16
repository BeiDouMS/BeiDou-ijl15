#pragma once
#include <string>

namespace AutoLogin {
    void Init(bool bEnable, const std::string& username, const std::string& password, int nWorld = 0, int nChannel = 0);
    void Hook(bool bEnable);
}
