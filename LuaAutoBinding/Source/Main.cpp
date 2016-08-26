#include "CodeGenerator.h"

#include <iostream>

#include <tclap/CmdLine.h>
using std::string;

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, const char** argv)
{
    string InputFile;
    try
    {
        using namespace TCLAP;

        CmdLine cmd("Header Parser");

        ValueArg<string> OutputFileArg("o", "output", "Output file path for writing generated code.", false, "", "", cmd);
        UnlabeledValueArg<string> InputFileArg("InputFile", "Input json ast file", true, "", "", cmd);

        cmd.parse(argc, argv);
        InputFile = InputFileArg.getValue();
    }
    catch (TCLAP::ArgException& e)
    {
        cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
        return -1;
    }


    CodeGenerator cg;
    if (cg.ParseAST(InputFile))
    {
        cout << cg.GetResult() << endl;
    }
    else
    {
        return 1;
    }
    
    return 0;
}