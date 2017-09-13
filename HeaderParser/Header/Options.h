#pragma once

#include <string>
#include <vector>

namespace hp
{
    struct Options
    {
        std::string NamespaceMacro;
        std::string ClassNameMacro;
        std::string ConstructorNameMacro;
        std::vector<std::string> FunctionNameMacro;
        std::string EnumNameMacro;
        std::vector<std::string> PropertyNameMacro;
        std::vector<std::string> CustomMacros;
    };
}
