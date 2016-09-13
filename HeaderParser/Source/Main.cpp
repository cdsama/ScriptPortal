#include "Parser.h"
#include "Options.h"

#include <tclap/CmdLine.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <regex>

#include <vector>
#include <list>

#include <io.h>

using hp::Options;
using hp::Parser;

using std::vector;
using std::list;
using std::string;
using std::pair;

using std::cout;
using std::cerr;
using std::endl;

using std::ifstream;
using std::ofstream;
using std::stringstream;

void DFSFolder(const string& path, list<string>& FileList)
{
    _finddata_t file_info;
    string current_path = path + "/*.*";
    auto handle = _findfirst(current_path.c_str(), &file_info);
    if (-1 == handle)
    {
        cout << "cannot match the path" << endl;
        return;
    }

    do
    {
        if (file_info.attrib == _A_SUBDIR)
        {
            if (strcmp(file_info.name, "..") != 0 && strcmp(file_info.name, ".") != 0)
            {
                DFSFolder(path + '/' + file_info.name, FileList);
            }
        }
        else
        {
            std::string FileName = path + "/" + file_info.name;
            static std::regex WordRegex(".*[.]h.*");
            static std::smatch BaseMatch;
            if (std::regex_match(FileName, BaseMatch, WordRegex))
            {
                FileList.push_back(FileName);
            }
        }
    } while (!_findnext(handle, &file_info));   
                                               
    _findclose(handle);
}

void RemoveSearchPath(const string& SearchPath, string& FileName)
{
    if (SearchPath.empty())
    {
        return;
    }
    auto pos = FileName.find(SearchPath);
    if (pos == 0)
    {
        FileName = FileName.substr(SearchPath.size() + 1);
    }
}

int main(int argc, char** argv)
{
    Options AppOption;
    vector<pair<string, string>> InputFiles;
    vector<string> InputDirs;
    string OutputFile;
    bool IsDirAsSearchPath;
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
        SwitchArg IsDirArg("d", "directory", "Input Files are directories", cmd, false);
        SwitchArg IsDirAsSearchPathArg("s", "searchpath", "Treat Directories as search path", cmd, true);
        UnlabeledMultiArg<string> InputFilesArg("InputFiles", "The files to process", true, "", cmd);

        cmd.parse(argc, argv);

        AppOption.ClassNameMacro = ClassName.getValue();
        AppOption.EnumNameMacro = EnumName.getValue();
        AppOption.FunctionNameMacro = FunctionName.getValue();
        AppOption.CustomMacros = CustomMacro.getValue();
        AppOption.PropertyNameMacro = PropertyName.getValue();
        if (IsDirArg.getValue())
        {
            InputDirs = InputFilesArg.getValue();
        }
        else 
        {
            for (auto& FileName:InputFilesArg.getValue())
            {
                InputFiles.push_back({ FileName, "" });
            }
        }
        IsDirAsSearchPath = IsDirAsSearchPathArg.getValue();
        OutputFile = OutputFileArg.getValue();
    }
    catch (TCLAP::ArgException& e)
    {
        cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
        return -1;
    }

    for (auto& Dir : InputDirs)
    {
        list<string> FileList;
        DFSFolder(Dir, FileList);
        for (auto& FileName : FileList)
        {
            InputFiles.push_back({ FileName, Dir });
        }
    }
    
    ifstream ifs;
    Parser parser(AppOption);
    stringstream buffer;
    parser.Open();
    for (auto& InputFile: InputFiles)
    {
        ifs.open(InputFile.first);
        if (!ifs.is_open())
        {
            cerr << "Could not open " << InputFile.first << endl;
            ifs.clear();
            continue;
        }

        
        buffer << ifs.rdbuf();
        ifs.clear();
        ifs.close();
        if (IsDirAsSearchPath)
        {
            RemoveSearchPath(InputFile.second, InputFile.first);
        }
        if (!parser.Parse(buffer.str().c_str(), InputFile.first.c_str()))
        {
            cerr << "Could not parse " << InputFile.first << endl;
            return -1;
        }
        buffer.clear();
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
