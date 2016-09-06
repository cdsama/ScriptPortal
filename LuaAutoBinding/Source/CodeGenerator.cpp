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

    struct ContentNode {
        std::string Name;
        virtual ~ContentNode() {};
    };

    enum class FunctionType {
        Common,
        Static,
        Global
    };

    struct LuaFunction : ContentNode {
        std::string Comment;
        FunctionType Type;
    };

    struct LuaProperty : ContentNode {
        std::string Comment;
        bool Writeable;
    };

    struct LuaEnum : ContentNode {
        std::list<std::string> Keys;
        std::string Comment;
    };

    struct LuaClass : ContentNode {
        std::string ParentClass;
        std::list<std::shared_ptr<ContentNode>> Nodes;
        std::string Comment;
    };

    struct CxxNamespace : ContentNode {
        std::list<std::shared_ptr<ContentNode>> Nodes;
    };

    struct HeaderFile : ContentNode {
        std::list<std::string> IncludeFiles;
        std::list<std::shared_ptr<ContentNode>> Nodes;
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

        std::shared_ptr<ContentNode> test = std::make_shared<HeaderFile>();

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
        ssInclude << "#include <lua.hpp>\n#include <luaportal/LuaPortal.h>\n";
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

        ssInclude << "#include \"" << File->Name << "\"" << std::endl;

        for (auto& Node : File->Nodes)
        {
            std::list<std::string> NamespaceStack;
            if (GenerateNamespace(Node, NamespaceStack) 
                && GenerateClass(Node, NamespaceStack)
                && GenerateEnum(Node, NamespaceStack)
                && GenerateFunction(Node, NamespaceStack)
                )
            {
                std::cerr << "Invalid Node:" << Node->Name << std::endl;
            }
        }
    }

    bool GenerateNamespace(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack) 
    {
        auto NSNode = dynamic_cast<CxxNamespace*>(Node.get());
        if (NSNode == nullptr)
        {
            return true;
        }

        NamespaceStack.push_back(NSNode->Name);

        for (auto& Node : NSNode->Nodes)
        {
            if (GenerateNamespace(Node, NamespaceStack)
                && GenerateClass(Node, NamespaceStack)
                && GenerateEnum(Node, NamespaceStack)
                && GenerateFunction(Node, NamespaceStack)
                )
            {
                std::cerr << "Invalid Node:" << Node->Name << std::endl;
            }
        }

        NamespaceStack.pop_back();
        return false;
    }

    std::string GetStackedNameSpace(const std::list<std::string>& NamespaceStack)
    {
        std::stringstream ss;
        for (auto& NS: NamespaceStack)
        {
            ss << NS << "::";
        }
        return ss.str();
    }

    bool GenerateClass(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack)
    {
        auto ClassNode = dynamic_cast<LuaClass*>(Node.get());
        if (ClassNode == nullptr)
        {
            return true;
        }

        if (ClassNode->ParentClass.empty())
        {
            ssNormal << "\t.BeginClass<" << GetStackedNameSpace(NamespaceStack) << ClassNode->Name << ">(\"" << ClassNode->Name << "\")\n";
        } 
        else
        {
            ssNormal << "\t.DeriveClass<" << GetStackedNameSpace(NamespaceStack) << ClassNode->Name << "," << ClassNode->ParentClass << ">(\"" << ClassNode->Name << "\")\n";
        }

        NamespaceStack.push_back(ClassNode->Name);
        
        for (auto& Node : ClassNode->Nodes)
        {
            if (GenerateProperty(Node, NamespaceStack)
                && GenerateEnum(Node, NamespaceStack)
                && GenerateFunction(Node, NamespaceStack)
                )
            {
                std::cerr << "Invalid Node:" << Node->Name << std::endl;
            }
        }

        NamespaceStack.pop_back();
        ssNormal << "\t.EndClass()\n";
        return false;
    }

    bool GenerateFunction(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack)
    {
        auto FunctionNode = dynamic_cast<LuaFunction*>(Node.get());
        if (FunctionNode == nullptr)
        {
            return true;
        }
        switch (FunctionNode->Type)
        {
        case FunctionType::Common:
        {
            ssNormal << "\t.AddFunction(\"" << FunctionNode->Name << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        case FunctionType::Static:
        {
            ssNormal << "\t.AddStaticFunction(\"" << FunctionNode->Name << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        case FunctionType::Global:
        {
            ssGlobal << "\t.AddFunction(\"" << FunctionNode->Name << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        default:
            break;
        }

        return false;
    }

    bool GenerateEnum(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack)
    {
        auto EnumNode = dynamic_cast<LuaEnum*>(Node.get());
        if (EnumNode == nullptr)
        {
            return true;
        }
        auto StackedNameSpace = GetStackedNameSpace(NamespaceStack);
        ssNormal << "\t.BeginEnum<" << StackedNameSpace << EnumNode->Name << ">(\"" << EnumNode->Name << "\")\n";
        for (auto& EnumKey : EnumNode->Keys)
        {
            ssNormal << "\t.AddEnumValue(\"" << EnumKey << "\", " << StackedNameSpace << EnumNode->Name << "::" << EnumKey << ")\n";
        }
        ssNormal << "\t.EndEnum()\n";
        return false;
    }

    bool GenerateProperty(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack)
    {
        auto PropertyNode = dynamic_cast<LuaProperty*>(Node.get());
        if (PropertyNode == nullptr)
        {
            return true;
        }

        ssNormal << "\t.AddData(\"" << PropertyNode->Name << "\", &" << GetStackedNameSpace(NamespaceStack) << PropertyNode->Name << (PropertyNode->Writeable ? ", true" : ", false") << ")\n";

        return false;
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
        bool IsMemberFunc = false;
        auto itr = FunctionObject.FindMember("access");
        if (itr != FunctionObject.MemberEnd())
        {
            if (std::string(itr->value.GetString()) != "public")
            {
                std::stringstream ss;
                ss << "Function :" << Function->Name << "must be public, Line: " << FunctionObject["line"].GetInt();
                throw ss.str();
            }
            IsMemberFunc = true;
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
                : (IsMemberFunc 
                    ? FunctionType::Static 
                    : FunctionType::Common);
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
            if (std::string(ParentObject["access"].GetString()) != "public")
            {
                continue;
            }
            auto& ParentName = ParentObject["name"];
            if (std::string(ParentName["type"].GetString()) != "literal")
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
            << "void RegistAPIs(luaportal::LuaState& l) \n{\n\tl.GlobalContext()\n"
            << ssNormal.str()
            << ssGlobal.str() 
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
