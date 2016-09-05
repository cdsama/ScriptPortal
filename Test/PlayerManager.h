#pragma once

#include "Player.h"

LUA_CLASS()
class PlayerManager
{
public:

    LUA_FUNCTION(global)
    static PlayerManager* GetPlayerManager(){
        return new PlayerManager();
    }

    LUA_FUNCTION()
    static std::string GetPlayerManagerVersion() {
        return "1.1";
    }

    LUA_FUNCTION()
    Player* GetPlayer()
    {
        return new Player("hahaha");
    }


};

// Get current time to string.
LUA_FUNCTION()
std::string GetCurrentTime() {
    return "1.2.3";
}

namespace Test
{
    // Get current time to string.
    LUA_FUNCTION()
    std::string GetCurrentTimeTest() {
        return "11.2.3";
    }
}