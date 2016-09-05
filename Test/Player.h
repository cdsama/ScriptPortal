#pragma once

#include "Object.h"
#include <sstream>
 

// The class of a player data
LUA_CLASS()
class Player : public Object
{
public:
    Player(const std::string& _name)
    : Object(_name)
    {

    }

    LUA_FUNCTION()
        std::string GetDiscribe()
    {
        std::stringstream ss;
        ss << "Name: " << GetName() << "ID: " << ID << "IP: " << IP;
        return ss.str();
    }

    // The id of a player
    LUA_PROPERTY(readonly)
    int ID;

    LUA_PROPERTY()
    int IP;

}; 
