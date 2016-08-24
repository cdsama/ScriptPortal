#include "Parser.h"
#include "Options.h"

#include <tclap/CmdLine.h>

#include <iostream>
#include <fstream>
#include <sstream>

using hp::Options;
using hp::Parser;

int main(int argc, char** argv)
{
    Options AppOption;
    std::string InputFile;
    try
    {
        using namespace TCLAP;

        CmdLine cmd("Header Parser");

        ValueArg<std::string> EnumName("e", "enum", "The name of the enum macro", false, "ENUM", "", cmd);
        ValueArg<std::string> ClassName("c", "class", "The name of the class macro", false, "CLASS", "", cmd);
        MultiArg<std::string> FunctionName("f", "function", "The name of the function macro", false, "", cmd);
        ValueArg<std::string> PropertyName("p", "property", "The name of the property macro", false, "PROPERTY", "", cmd);
        MultiArg<std::string> CustomMacro("m", "macro", "Custom macro names to parse", false, "", cmd);
        UnlabeledValueArg<std::string> InputFileArg("InputFile", "The file to process", true, "", "", cmd);

        cmd.parse(argc, argv);

        InputFile = InputFileArg.getValue();
        AppOption.ClassNameMacro = ClassName.getValue();
        AppOption.EnumNameMacro = EnumName.getValue();
        AppOption.FunctionNameMacro = FunctionName.getValue();
        AppOption.CustomMacros = CustomMacro.getValue();
        AppOption.PropertyNameMacro = PropertyName.getValue();
    }
    catch (TCLAP::ArgException& e)
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return -1;
    }

    // Open from file
    std::ifstream ifs(InputFile);
    if (!ifs.is_open())
    {
        std::cerr << "Could not open " << InputFile << std::endl;
        return -1;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    ifs.close();
    Parser parser(AppOption);
    if (parser.Parse(buffer.str().c_str())) {
        std::cout << parser.result() << std::endl;
    }

    return 0;
}
