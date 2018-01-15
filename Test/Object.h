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

    LUA_CTOR()
    Object(const std::string& _name, const int a, const std::function<void(bool), int>& b, const int&& c,  const int* const d, int** e, const int** f, const int& g, int const& h)
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
    inline static std::function<void()> OnEnd;

private:
    std::string name;
};