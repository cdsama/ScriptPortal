#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <memory> 

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace rapidjson;

struct CodeGenerator::Impl
{
    std::stringstream ssInclude;
    std::stringstream ssNormal;
    std::stringstream ssGlobal;

    std::vector<std::shared_ptr<HeaderFile>> Files;

    std::string CurrentFile;

    Document document;

    struct Node {
        std::string Name;
    };

    enum class FunctionType {
        Common,
        Static,
        Global
    };

    struct LuaFunction : Node {
        std::string Comment;
        FunctionType Type;
    };

    struct LuaProperty : Node {
        std::string Comment;
        bool Writeable;
    };

    struct LuaEnum : Node {
        std::list<std::string> Keys;
        std::string Comment;
    };

    struct LuaClass : Node {
        std::string ParentClass;
        std::list<std::shared_ptr<LuaFunction>> Functions;
        std::list<std::shared_ptr<LuaProperty>> Properties;
        std::list<std::shared_ptr<LuaEnum>> Enums;
        std::string Comment;
    };

    struct CxxNamespace : Node {
        std::list<std::shared_ptr<LuaFunction>> StaticFunctions;
        std::list<std::shared_ptr<CxxNamespace>> Namespaces;
        std::list<std::shared_ptr<LuaClass>> Classes;
        std::list<std::shared_ptr<LuaEnum>> Enums;
    };

    struct HeaderFile : Node {
        std::list<std::string> IncludeFiles;
        std::list<std::shared_ptr<LuaFunction>> GlobalFunctions;
        std::list<std::shared_ptr<CxxNamespace>> Namespaces;
        std::list<std::shared_ptr<LuaClass>> Classes;
        std::list<std::shared_ptr<LuaEnum>> Enums;
    };

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
            std::cerr << "Error: " << e << "Current File :" << CurrentFile << std::endl;
            return false;
        }
        return true;
    }

    void ParseDocument(const Document& document)
    {
        for (SizeType i = 0; i < document.Size(); ++i)
        {
            auto& v = document[i];
            Files.push_back(ParseFile(v));
        }
    }

    std::shared_ptr<HeaderFile> ParseFile(const Document::ValueType& FileObject)
    {
        std::shared_ptr<HeaderFile> File = std::make_shared<HeaderFile>();
        File->Name = FileObject["content"]->GetString();
        CurrentFile = File->Name;
        auto& ContentArray = FileObject["content"];
        for (SizeType i = 0; i < ContentArray.Size(); ++i)
        {
            auto& Unit = ContentArray[i];
            std::string type = Unit["type"].GetString();
            if (type == "class")
            {
                File->Classes.push_back(ParseClass(Unit));
            }
            else if (type == "function")
            {
                File->GlobalFunctions.push_back(ParseFunction(Unit));
            }
            else if (type == "include")
            {
                File->IncludeFiles.push_back(Unit["file"].GetString());
            }
            else if (type == "enum")
            {
                File->Enums.push_back(ParseEnum(Unit));
            }
        }
        return File;
    }


    std::shared_ptr<LuaEnum> ParseEnum(const Document::ValueType&& EnumObject)
    {
        std::shared_ptr<LuaEnum> Enum = std::make_shared<LuaEnum>();
        Enum->Name = EnumObject["name"];
        return Enum;
    }

    std::shared_ptr<LuaClass> ParseClass(const Document::ValueType& ClassObject)
    {
        std::shared_ptr<LuaClass> Class = std::make_shared<LuaClass>();
        Class->Name = ClassObject["name"].GetString();
        if (!ClassObject["meta"].HasMember("noinherit"))
        {
            GetClassParent(ClassObject, Class->ParentClass);
        }

        auto& ClassMembers = ClassObject["members"];
        for (SizeType i = 0; i < ClassMembers.Size(); ++i)
        {
            auto& Unit = ClassMembers[i];
            std::string type = Unit["type"].GetString();
            if (type == "function")
            {
                Class->Functions.push_back(ParseFunction(Unit));
            }
        }

        return Class;
    }

    std::shared_ptr<LuaFunction> ParseFunction(const Document::ValueType& FunctionObject)
    {
        std::shared_ptr<LuaFunction> Function = std::make_shared<LuaFunction>();
        Function->Name = FunctionObject["name"];
        Function->Type = FunctionType::Common;
        auto itr = FunctionObject.FindMember("access");
        if (itr != FunctionObject.End())
        {
            if (itr->value.GetString() != "public")
            {
                stringstream ss;
                ss << "Function :" << Function->Name << "must be public, Line: " << FunctionObject["line"]->GetInt();
                throw ss.str();
            }
        }
        bool IsStatic = false;
        itr = FunctionObject.FindMember("static");
        if (itr != FunctionObject.End())
        {
            IsStatic = itr->value.GetBool();
        }

        if (IsStatic)
        {
            auto& FunctionMeta = FunctionObject["meta"];
            Function->Type = (FunctionMeta->HasMember("global"))
                ? FunctionType::Global
                : FunctionType::Static;
        }
        return Function;
    }

    bool GetClassParent(const Document::ValueType& ClassObject, std::string& OutParentClassName)
    {
        auto &ParentsArray = ClassObject["parents"];
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
