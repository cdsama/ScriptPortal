#pragma once

#include "Object.h"
#include <sstream>
 

// The class of a player data
LUA_CLASS(name=LuaPlayer)
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
    LUA_DATA(readonly)
    int ID;

    LUA_DATA()
    int IP;

    LUA_PROPERTY(name=strName)
    std::string& GetStrName() const{return str_name;}

    LUA_PROPERTY(name=Name2, setter = SetStrName2)
    std::string& GetStrName2() const{return str_name;}

    void SetStrName2(const std::string& Val){str_name = Val;}

    private:
    std::string str_name;
}; 
