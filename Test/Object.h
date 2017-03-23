#pragma once
#include <string>
#include <luaexport>


// The type enum of an object.
LUA_NAMESPACE(name = ot)
namespace testnamespace{
    LUA_ENUM(name=OT)
    enum class ObjectType : int
    {
        Player,
        Npc,
        Item,
    };
}
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
    LUA_FUNCTION(name = GetNameFunc)
    const std::string& GetName(){
        return name;
    }

    LUA_CALLBACK(name = OnObjEnd)
    static std::function<void()> OnEnd;

private:
    std::string name;
};