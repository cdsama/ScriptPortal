#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

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
        ssInclude << "#include <luaportal/LuaPortal.h>\n";
        for (SizeType i = 0; i< document.Size(); ++i)
        {
            auto& v = document[i];
            ParseFile(v);
        }
    }

    void ParseFile(const Document::ValueType& Object)
    {
        check(Object.IsObject(), "Expected file root type : 'object'");
        auto itr = Object.FindMember("file");
        check(itr != Object.MemberEnd() && itr->value.IsString(), "Expected content member : 'file', type : string");
        ssInclude << "#include \"" << itr->value.GetString() << "\"" << std::endl;
        itr = Object.FindMember("content");
        check(itr != Object.MemberEnd(), "Expected content member : 'content'");
        ParseContent(itr->value);
    }

    void ParseContent(const Document::ValueType& Value)
    {
        check(Value.IsArray(), "Expected content root is array");
        std::vector<std::string> NameSpaceStack;
        for (SizeType i = 0; i< Value.Size(); ++i)
        {
            auto& Unit = Value[i];
            check(Object.IsObject(), "Expected Unit type : 'object'");
            auto itr = Unit->FindMember("type");
            check(itr != Unit.MemberEnd() && itr->value.IsString(), "Expected Unit member : 'type' , type : string");
            std::string type = itr->value.GetString();
            if (type == "class")
            {
                ParseClass(Unit, NameSpaceStack);
            }
        }
    }

    void ParseClass(const Document::ValueType& ClassObject, std::vector<std::string>& NameSpaceStack)
    {
        auto itr = ClassObject->FindMember("name");
        check(itr != ClassObject.MemberEnd() && itr->value.IsString(), "Expected Class member : 'name' , type : string");
        std::string ClassName = itr->value.GetString();

        NameSpaceStack.push_back(ClassName);

        NameSpaceStack.pop_back();
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
