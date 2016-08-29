#include "Parser.h"
#include "Options.h"

#include <tclap/CmdLine.h>

#include <iostream>
#include <fstream>
#include <sstream>

using hp::Options;
using hp::Parser;

using std::vector;
using std::string;

using std::cout;
using std::cerr;
using std::endl;

using std::ifstream;
using std::ofstream;
using std::stringstream;

int main(int argc, char** argv)
{
    Options AppOption;
    vector<string> InputFiles;
    string OutputFile;
    try
    {
        using namespace TCLAP;

        CmdLine cmd("Header Parser");

        ValueArg<string> EnumName("e", "enum", "The name of the enum macro", false, "ENUM", "", cmd);
        ValueArg<string> ClassName("c", "class", "The name of the class macro", false, "CLASS", "", cmd);
        MultiArg<string> FunctionName("f", "function", "The name of the function macro", false, "", cmd);
        ValueArg<string> PropertyName("p", "property", "The name of the property macro", false, "PROPERTY", "", cmd);
        MultiArg<string> CustomMacro("m", "macro", "Custom macro names to parse", false, "", cmd);
        ValueArg<string> OutputFileArg("o", "output", "Output file path for writing json ast", false, "", "", cmd);
        UnlabeledMultiArg<string> InputFilesArg("InputFiles", "The files to process", true, "", cmd);

        cmd.parse(argc, argv);

        AppOption.ClassNameMacro = ClassName.getValue();
        AppOption.EnumNameMacro = EnumName.getValue();
        AppOption.FunctionNameMacro = FunctionName.getValue();
        AppOption.CustomMacros = CustomMacro.getValue();
        AppOption.PropertyNameMacro = PropertyName.getValue();
        InputFiles = InputFilesArg.getValue();
        OutputFile = OutputFileArg.getValue();
    }
    catch (TCLAP::ArgException& e)
    {
        cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
        return -1;
    }

    
    ifstream ifs;
    Parser parser(AppOption);
    stringstream buffer;
    parser.Open();
    for (auto& InputFile: InputFiles)
    {
        ifs.open(InputFile);
        if (!ifs.is_open())
        {
            cerr << "Could not open " << InputFile << endl;
            ifs.clear();
            continue;
        }

        
        buffer << ifs.rdbuf();
        ifs.close();

        if (!parser.Parse(buffer.str().c_str(), InputFile.c_str()))
        {
            return -1;
        }
        buffer.str("");
    }
    parser.Close();

    if (OutputFile.empty()) {
        cout << parser.result() << endl;
    }
    else 
    {
        ofstream ofs(OutputFile);
        if (!ofs.is_open())
        {
            cerr << "Could not write " << OutputFile << endl;
            ofs.clear();
            return -1;
        }
        ofs << parser.result();
        ofs.close();
    }
    
    return 0;
}
