#include <iostream>
#include <lua.hpp>
#include <luaportal/LuaPortal.h>
using namespace luaportal;

void RegistAPIs(luaportal::LuaState& l);

int main(int argc, char* argv[])
{
    LuaState l;
    RegistAPIs(l);

    l.DoString("print(PlayerManager.GetPlayerManagerVersion())", [](const std::string& err) {
        std::cerr << err << std::endl;
    });

    return 0;
}