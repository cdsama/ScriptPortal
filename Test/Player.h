#include "Object.h"
#include <sstream>

LUA_CLASS()
class Player : public Object
{
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

    LUA_PROPERTY(GET)
    int ID;

    LUA_PROPERTY(GET,SET)
    int IP;

};