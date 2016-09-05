#pragma once
#include <string>
#define LUA_ENUM(...) 
#define LUA_CLASS(...) 
#define LUA_FUNCTION(...) 
#define LUA_PROPERTY(...) 


// The type enum of an object.
LUA_ENUM()
enum class ObjectType : int
{
    Player,
    Npc,
    Item,
};

LUA_CLASS()
class Object
{
public:

    Object(const std::string& _name)
    : name(_name)
    {

    }

    virtual ~Object(){}

    // Get name of an object.
    LUA_FUNCTION()
    const std::string& GetName(){
        return name;
    }


private:
    std::string name;
};