#include "Player.h"

LUA_CLASS()
class PlayerManager
{
public:

    LUA_FUNCTION(global)
    static PlayerManager* GetPlayerManager();

    LUA_FUNCTION()
    Player* GetPlayer();

private:
    
};

// Get current time to string.
LUA_FUNCTION()
std::string GetCurrentTime();