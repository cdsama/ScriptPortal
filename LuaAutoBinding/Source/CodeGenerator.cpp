#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace rapidjson;

#define check(__expression__,__info__) if(!(__expression__)){throw std::string(__info__);}

struct CodeGenerator::Impl
{
    std::stringstream ssInclude;
    std::stringstream ssNormal;
    std::stringstream ssGlobal;
    Impl()
    {

    }

    bool ParseAST(const std::string& InputFile)
    {
        std::ifstream ifs(InputFile);
        if (!ifs.is_open()) {
            std::cerr << "Could not open " << InputFile << std::endl;
            return false;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ifs.close();
        Document document;
        document.Parse(buffer.str().c_str());
        
        try
        {
            ParseDocument(document);
        }
        catch (std::string e)
        {
            std::cerr << "Error: " << e << std::endl;
            return false;
        }
        return true;
    }

    void ParseDocument(const Document& document)
    {
        check(document.IsArray(), "Expected array root");
        ssInclude << "#include <luaportal.h>;\n";
        for (SizeType i = 0; i< document.Size(); ++i)
        {
            auto& v = document[i];
            ParseFile(v);
        }
    }

    void ParseFile(const Document::ValueType& Object)
    {
        check(Object.IsObject(), "Expected file root is object");
        auto& itr = Object.FindMember("file");
        check(itr != Object.MemberEnd(), "Expected content member : file");

    }

    std::string GetResult()
    {
        std::stringstream ss;
        ss << ssInclude.str() << std::endl
            << "void RegistAPIs(luaportal::LuaState& l) \n{\n\tl.module()\n\t"
            << ssNormal.str()
            << ssGlobal.str() << std::endl
            << "\t;\n}" << std::endl;
        return ss.str();
    }

};

CodeGenerator::CodeGenerator()
    : impl(std::make_shared<Impl>())
{

}

CodeGenerator::~CodeGenerator()
{

}

bool CodeGenerator::ParseAST(const std::string& InputFile)
{
    return impl->ParseAST(InputFile);
}

std::string CodeGenerator::GetResult()
{
    return impl->GetResult();
}
