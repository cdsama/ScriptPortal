#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <unordered_set>
#include <memory> 

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace rapidjson;

struct CodeGenerator::Impl
{

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
        std::list<std::shared_ptr<Node>> Nodes;
        std::string Comment;
    };

    struct CxxNamespace : Node {
        std::list<std::shared_ptr<Node>> Nodes;
    };

    struct HeaderFile : Node {
        std::list<std::string> IncludeFiles;
        std::list<std::shared_ptr<Node>> Nodes;
    };

    std::stringstream ssInclude;
    std::stringstream ssNormal;
    std::stringstream ssGlobal;

    std::list<std::shared_ptr<HeaderFile>> Files;
    std::unordered_set<std::shared_ptr<HeaderFile>> UnregistedFiles;
    std::unordered_set<std::shared_ptr<HeaderFile>> RegistedFiles;

    std::string CurrentFile;

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

        std::shared_ptr<Node> test = std::make_shared<HeaderFile>();

        try
        {
            ParseDocument(document);
            GenerateCode();
        }
        catch (std::string e)
        {
            std::cerr << " Error: " << e << " Current File :" << CurrentFile << std::endl;
            return false;
        }

        return true;
    }

    void GenerateCode()
    {
        for (auto& file : Files)
        {
            GenerateFile(file);
        }
    }

    void GenerateFile(const std::shared_ptr<HeaderFile>& File)
    {
        if (RegistedFiles.find(File) != RegistedFiles.end())
        {
            return;
        }
        else 
        {
            RegistedFiles.insert(File);
            UnregistedFiles.erase(File);
        }

        for (auto& IncludeFileName : File->IncludeFiles)
        {
            std::shared_ptr<HeaderFile> RegistFirstFile;
            if ((!IsRegisted(IncludeFileName)) && IsUnregisted(IncludeFileName, RegistFirstFile))
            {
                GenerateFile(RegistFirstFile);
            }
        }
        
    }

    bool IsRegisted(const std::string& FileName)
    {
        for (auto& file : RegistedFiles)
        {
            if (file->Name.find(FileName) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool IsUnregisted(const std::string& FileName, std::shared_ptr<HeaderFile>& FilePtr)
    {
        for (auto& file : UnregistedFiles)
        {
            if (file->Name.find(FileName) != std::string::npos)
            {
                FilePtr = file;
                return true;
            }
        }
        return false;
    }

    void ParseDocument(const Document& document)
    {
        for (SizeType i = 0; i < document.Size(); ++i)
        {
            auto& v = document[i];
            auto& file = ParseFile(v);
            Files.push_back(file);
            UnregistedFiles.insert(file);
        }
    }

    std::shared_ptr<HeaderFile> ParseFile(const Document::ValueType& FileObject)
    {
        std::shared_ptr<HeaderFile> File = std::make_shared<HeaderFile>();
        File->Name = FileObject["file"].GetString();
        CurrentFile = File->Name;
        auto& ContentArray = FileObject["content"];
        for (SizeType i = 0; i < ContentArray.Size(); ++i)
        {
            auto& Unit = ContentArray[i];
            std::string type = Unit["type"].GetString();
            if (type == "class")
            {
                File->Nodes.push_back(ParseClass(Unit));
            }
            else if (type == "function")
            {
                File->Nodes.push_back(ParseFunction(Unit));
            }
            else if (type == "include")
            {
                File->IncludeFiles.push_back(Unit["file"].GetString());
            }
            else if (type == "enum")
            {
                File->Nodes.push_back(ParseEnum(Unit));
            }
            else if (type == "namespace")
            {
                File->Nodes.push_back(ParseNamespace(Unit));
            }
        }
        return File;
    }

    void TryGetComment(const Document::ValueType& Object, std::string& Comment) 
    {
        auto itr = Object.FindMember("comment");
        if (itr != Object.MemberEnd())
        {
            Comment = itr->value.GetString();
        }
    }

    std::shared_ptr<CxxNamespace> ParseNamespace(const Document::ValueType& NamespaceObject)
    {
        std::shared_ptr<CxxNamespace> Namespace = std::make_shared<CxxNamespace>();
        Namespace->Name = NamespaceObject["name"].GetString();
        auto& NamespaceMembers = NamespaceObject["members"];
        for (SizeType i = 0; i < NamespaceMembers.Size(); ++i)
        {
            auto& Unit = NamespaceMembers[i];
            std::string type = Unit["type"].GetString();
            if (type == "class")
            {
                Namespace->Nodes.push_back(ParseClass(Unit));
            }
            else if (type == "function")
            {
                Namespace->Nodes.push_back(ParseFunction(Unit));
            }
            else if (type == "enum")
            {
                Namespace->Nodes.push_back(ParseEnum(Unit));
            }
        }
        return Namespace;
    }

    std::shared_ptr<LuaEnum> ParseEnum(const Document::ValueType& EnumObject)
    {
        std::shared_ptr<LuaEnum> Enum = std::make_shared<LuaEnum>();
        Enum->Name = EnumObject["name"].GetString();
        auto& EnumMembers = EnumObject["members"];
        for (SizeType i = 0; i < EnumMembers.Size(); ++i)
        {
            Enum->Keys.push_back(EnumMembers[i]["key"].GetString());
        }
        TryGetComment(EnumObject, Enum->Comment);
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
                Class->Nodes.push_back(ParseFunction(Unit));
            }
            else if (type == "property")
            {
                Class->Nodes.push_back(ParseProperty(Unit));
            }
            else if (type == "enum")
            {
                Class->Nodes.push_back(ParseEnum(Unit));
            }
        }
        TryGetComment(ClassObject, Class->Comment);
        return Class;
    }

    std::shared_ptr<LuaFunction> ParseFunction(const Document::ValueType& FunctionObject)
    {
        std::shared_ptr<LuaFunction> Function = std::make_shared<LuaFunction>();
        Function->Name = FunctionObject["name"].GetString();
        Function->Type = FunctionType::Common;
        auto itr = FunctionObject.FindMember("access");
        if (itr != FunctionObject.MemberEnd())
        {
            std::string Access = itr->value.GetString();
            if (Access != "public")
            {
                std::stringstream ss;
                ss << "Function :" << Function->Name << "must be public, Line: " << FunctionObject["line"].GetInt();
                throw ss.str();
            }
        }
        bool IsStatic = false;
        itr = FunctionObject.FindMember("static");
        if (itr != FunctionObject.MemberEnd())
        {
            IsStatic = itr->value.GetBool();
        }

        if (IsStatic)
        {
            auto& FunctionMeta = FunctionObject["meta"];
            Function->Type = (FunctionMeta.HasMember("global"))
                ? FunctionType::Global
                : FunctionType::Static;
        }
        TryGetComment(FunctionObject, Function->Comment);
        return Function;
    }

    std::shared_ptr<LuaProperty> ParseProperty(const Document::ValueType& PropertyObject)
    {
        std::shared_ptr<LuaProperty> Property = std::make_shared<LuaProperty>();
        Property->Name = PropertyObject["name"].GetString();
        Property->Writeable = !PropertyObject["meta"].HasMember("readonly");
        TryGetComment(PropertyObject, Property->Comment);
        return Property;
    }

    bool GetClassParent(const Document::ValueType& ClassObject, std::string& OutParentClassName)
    {
        auto itr = ClassObject.FindMember("parents");
        if (itr == ClassObject.MemberEnd())
        {
            return false;
        }
        auto &ParentsArray = itr->value;
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
