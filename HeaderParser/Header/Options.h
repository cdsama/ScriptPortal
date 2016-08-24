#pragma once

#include <string>
#include <vector>

namespace hp
{
    struct Options
    {
        std::string ClassNameMacro;
        std::vector<std::string> FunctionNameMacro;
        std::string EnumNameMacro;
        std::string PropertyNameMacro;
        std::vector<std::string> CustomMacros;
    };
}
