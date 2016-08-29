#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace rapidjson;

struct CodeGenerator::Impl
{
    std::stringstream ssInclude;
    std::stringstream ssNormal;
    std::stringstream ssGlobal;
    Document document;

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
        ssInclude << "#include <luaportal/LuaPortal.h>\n";
        for (SizeType i = 0; i< document.Size(); ++i)
        {
            auto& v = document[i];
            ParseFile(v);
        }
    }

    void ParseFile(const Document::ValueType& Object)
    {
        ssInclude << "#include \"" << Object["file"].GetString() << "\"" << std::endl;
        ParseContent(Object["content"]);
    }

    void ParseContent(const Document::ValueType& Value)
    {
        std::vector<std::string> NameSpaceStack;
        for (SizeType i = 0; i< Value.Size(); ++i)
        {
            auto& Unit = Value[i];
            std::string type = Unit["type"].GetString();
            if (type == "class")
            {
                ParseClass(Unit, NameSpaceStack);
            }
        }
    }

    std::string GetNameSpaceHead(const std::vector<std::string>& NameSpaceStack)
    {
        std::stringstream ss;
        for (auto& ns : NameSpaceStack)
        {
            ss << ns << "::";
        }
        return ss.str();
    }

    void ParseClass(const Document::ValueType& ClassObject, std::vector<std::string>& NameSpaceStack)
    {
        std::string ClassName = ClassObject["name"].GetString();
        std::string ParentClassName;
        auto NameSpaceHead = GetNameSpaceHead(NameSpaceStack);
        if (!ClassObject["meta"].HasMember("noinherit") && GetClassParent(ClassObject, ParentClassName))
        {
            
            if (RegistedClasses.find(ParentClassName) != RegistedClasses.end())
            {
                ssNormal << ".DeriveClass<" << NameSpaceHead << ClassName << ", " << ParentClassName << ">(\"" << ClassName << "\")" << std::endl;
            }
            else
            {

            }
        }
        else 
        {
            ssNormal << ".BeginClass<" << NameSpaceHead << ClassName << ">(\"" << ClassName << "\")" << std::endl;
        }

        NameSpaceStack.push_back(ClassName);

        NameSpaceStack.pop_back();

        ssNormal << ".EndClass()" << std::endl;
    }

    bool GetClassParent(const Document::ValueType& ClassObject, std::string& OutParentClassName)
    {
        auto &ParentsArray= ClassObject["parents"];
        if (!ParentsArray.IsArray())
        {
            return false;
        }
        for (SizeType i = 0; i < ParentsArray.Size(); ++i)
        {
            auto& ParentObject = ParentsArray[i];
            if (ParentObject["access"].GetString() != "public")
            {
                continue;
            }
            auto& ParentName = ParentObject["name"];
            if (ParentName["type"].GetString() != "literal")
            {
                continue;
            }
            OutParentClassName = ParentName["name"].GetString();
            return true;
        }
        return false;
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
