#include <string>
LUA_CLASS()
class Object
{
public:

    Object(const std::string& _name)
    : name(_name)
    {

    }

    virtual ~Object(){}

    LUA_FUNCTION()
    const std::string& GetName(){
        return name;
    }
private:
    std::string name;
}