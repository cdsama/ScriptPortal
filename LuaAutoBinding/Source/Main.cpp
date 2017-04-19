#include "CodeGenerator.h"

#include <iostream>
#include <fstream>

#include <tclap/CmdLine.h>
using std::string;
using std::vector;

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, const char** argv)
{
    string InputFile;
    string OutputFile;
    vector<string> PreIncludeList;
    string AutoNullMacro;
    string FunctionPropertyMacro;
    string CFunctionMacro;
    try
    {
        using namespace TCLAP;

        CmdLine cmd("Header Parser");

        ValueArg<string> OutputFileArg("o", "output", "Output file path for writing generated code.", false, "", "", cmd);
        MultiArg<string> PreIncludeListArg("p", "preinclude", "Add pre include contents.", false, "", cmd);
        ValueArg<string> AutoNullMacroArg("a", "autonullmacro", "Generate set data nullptr code.", false, "", "", cmd);
        ValueArg<string> FunctionPropertyMacroArg("f", "functionpropertymacro", "Mark function as property.", false, "", "", cmd);
        ValueArg<string> CFunctionMacroArg("c", "cfunctionmacro", "Mark function as lua_cfunction.", false, "", "", cmd);
        UnlabeledValueArg<string> InputFileArg("InputFile", "Input json ast file.", true, "", "", cmd);
        

        cmd.parse(argc, argv);
        InputFile = InputFileArg.getValue();
        OutputFile = OutputFileArg.getValue();
        PreIncludeList = PreIncludeListArg.getValue();
        AutoNullMacro = AutoNullMacroArg.getValue();
        FunctionPropertyMacro = FunctionPropertyMacroArg.getValue();
        CFunctionMacro = CFunctionMacroArg.getValue();
    }
    catch (TCLAP::ArgException& e)
    {
        cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
        return -1;
    }


    CodeGenerator cg;
    cg.AutoNullMacro = AutoNullMacro;
    cg.FunctionPropertyMacro = FunctionPropertyMacro;
    cg.CFunctionMacro = CFunctionMacro;
    if (cg.ParseAST(InputFile))
    {
        if (OutputFile.empty())
        {
            cout << cg.GetResult() << endl;
        }
        else
        {
            std::ofstream ofs(OutputFile);
            if (!ofs.is_open())
            {
                cerr << "Could not write " << OutputFile << endl;
                ofs.clear();
                return -1;
            }
            for (auto& Include : PreIncludeList)
            {
                ofs << Include << endl;
            }

            ofs << cg.GetResult();
            ofs.close();
        }
    }
    else
    {
        return 1;
    }
    
    return 0;
}