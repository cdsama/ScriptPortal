#pragma once

#include <vector>
#include <string>
#include <memory>

class CodeGenerator
{
public:
    CodeGenerator();
    ~CodeGenerator();

    bool ParseAST(const std::string& InputFile);

    std::string GetResult();

    std::string AutoNullMacro;

private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};
