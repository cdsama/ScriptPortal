#include "Object.h"
#include <sstream>


// The class of a player data
LUA_CLASS()
class Player : public Object
{

    LUA_FUNCTION()
    void Test();

public:
    Player(const std::string& _name)
    : Object(const std::string& _name)
    {

    }
    
    LUA_FUNCTION()
    std::string GetDiscribe()
    {
        std::stringstream ss;
        ss<< "Name：" << GetName() << "ID: " << ID << "IP: " << IP;
        return ss.str();
    }

    // The id of a player
    LUA_PROPERTY(GET)
    int ID;

    LUA_PROPERTY(GET,SET)
    int IP;

};