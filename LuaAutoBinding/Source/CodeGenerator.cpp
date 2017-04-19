#include "CodeGenerator.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <memory> 

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace rapidjson;

struct CodeGenerator::Impl
{

    struct ContentNode {
        std::string Name;
        std::string ExportName;
        virtual ~ContentNode() {};
    };

    enum class FunctionType {
        Common,
        Static,
        Global,
        Property
    };

    struct LuaFunction : ContentNode {
        std::string Comment;
        std::string SetterName;
        FunctionType Type;
        bool IsCFunction = false;
    };

    struct LuaData : ContentNode {
        std::string Comment;
        bool Writeable;
        bool IsStatic;
        bool IsAutoNull;
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
        bool bShouldExport;
    };

    struct HeaderFile : ContentNode {
        std::list<std::string> IncludeFiles;
        std::list<std::shared_ptr<ContentNode>> Nodes;
    };

    friend bool operator<(const std::shared_ptr<HeaderFile>& lhs,const std::shared_ptr<HeaderFile>& rhs)
    {
        return lhs->Name < rhs->Name;
    }

    std::stringstream ssInclude;
    std::stringstream ssNormal;
    std::stringstream ssGlobal;
    std::stringstream ssAutoNull;

    std::list<std::shared_ptr<HeaderFile>> Files;
    std::set<std::shared_ptr<HeaderFile>> UnregistedFiles;
    std::set<std::shared_ptr<HeaderFile>> RegistedFiles;

    std::string CurrentFile;
    std::string AutoNullMacro;
    std::string FunctionPropertyMacro;
    std::string CFunctionMacro;

    Document document;

    static inline std::string& GetExportName(const std::shared_ptr<ContentNode>& Node)
    {
        return Node->ExportName.empty() ? Node->Name : Node->ExportName;
    }

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
        ssInclude << "#include <lua.hpp>\n#include <luaportal/luaportal.h>\n";
        for (auto& file : Files)
        {
            GenerateFile(file);
        }
    }

    bool IsEmptyNode(const std::shared_ptr<ContentNode>& Node)
    {
        if (Node == nullptr)
        {
            return true;
        }
        auto NSNode = dynamic_cast<CxxNamespace*>(Node.get());
        if (NSNode == nullptr)
        {
            return false;
        }
        else 
        {
            for (auto& SubNode : NSNode->Nodes)
            {
                if (!IsEmptyNode(SubNode))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool isEmptyFile(const std::shared_ptr<HeaderFile>& File)
    {
        if (File == nullptr)
        {
            return true;
        }
        if (File->Nodes.empty())
        {
            return true;
        }
        for (auto& Node : File->Nodes)
        {
            if (!IsEmptyNode(Node))
            {
                return false;
            }
        }
        return true;
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

        if (isEmptyFile(File))
        {
            return;
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

        if (NSNode->bShouldExport)
        {
            ssNormal << "\t.BeginNamespace(\"" << GetExportName(Node) << "\")\n";
        }

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

        if (NSNode->bShouldExport)
        {
            ssNormal << "\t.EndNamespace()\n";
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
            ssNormal << "\t.BeginClass<" << GetStackedNameSpace(NamespaceStack) << ClassNode->Name << ">(\"" << GetExportName(Node) << "\")\n";
        } 
        else
        {
            ssNormal << "\t.DeriveClass<" << GetStackedNameSpace(NamespaceStack) << ClassNode->Name << "," << ClassNode->ParentClass << ">(\"" << GetExportName(Node) << "\")\n";
        }

        NamespaceStack.push_back(ClassNode->Name);
        
        for (auto& Node : ClassNode->Nodes)
        {
            if (GenerateData(Node, NamespaceStack)
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
        std::string COrEmptyString = FunctionNode->IsCFunction ? "C" : "";
        switch (FunctionNode->Type)
        {
        case FunctionType::Common:
        {
            ssNormal << "\t.Add" << COrEmptyString << "Function(\"" << GetExportName(Node) << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        case FunctionType::Static:
        {
            ssNormal << "\t.AddStatic" << COrEmptyString << "Function(\"" << GetExportName(Node) << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        case FunctionType::Global:
        {
            ssGlobal << "\t.Add" << COrEmptyString << "Function(\"" << GetExportName(Node) << "\", &" << GetStackedNameSpace(NamespaceStack) << FunctionNode->Name << ")\n";
            break;
        }
        case FunctionType::Property:
        {
            auto StackedNameSpace = GetStackedNameSpace(NamespaceStack);
            if (FunctionNode->SetterName.empty())
            {
                ssNormal << "\t.AddProperty(\"" << GetExportName(Node) << "\", &" << StackedNameSpace << FunctionNode->Name << ")\n";
            }
            else
            {
                ssNormal << "\t.AddProperty(\"" << GetExportName(Node) << "\", &" << StackedNameSpace << FunctionNode->Name << ", &"<< StackedNameSpace << FunctionNode->SetterName << ")\n";
            }
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
        ssNormal << "\t.BeginEnum<" << StackedNameSpace << EnumNode->Name << ">(\"" << GetExportName(Node) << "\")\n";
        for (auto& EnumKey : EnumNode->Keys)
        {
            ssNormal << "\t.AddEnumValue(\"" << EnumKey << "\", " << StackedNameSpace << EnumNode->Name << "::" << EnumKey << ")\n";
        }
        ssNormal << "\t.EndEnum()\n";
        return false;
    }

    bool GenerateData(const std::shared_ptr<ContentNode>& Node, std::list<std::string>& NamespaceStack)
    {
        auto DataNode = dynamic_cast<LuaData*>(Node.get());
        if (DataNode == nullptr)
        {
            return true;
        }
        auto StackedNameSpace = GetStackedNameSpace(NamespaceStack);
        ssNormal << (DataNode->IsStatic ? "\t.AddStaticData(\"" : "\t.AddData(\"") << GetExportName(Node) << "\", &" << StackedNameSpace << DataNode->Name << (DataNode->Writeable ? ", true" : ", false") << ")\n";
        if (DataNode->IsStatic && DataNode->IsAutoNull)
        {
            ssAutoNull << "\t" << StackedNameSpace << DataNode->Name << " = nullptr;\n";
        }

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
        Files.sort();
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
        File->IncludeFiles.sort();
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
        Namespace->bShouldExport = NamespaceObject.HasMember("macro");
        TryGetExportName(NamespaceObject, Namespace->ExportName);
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

    void TryGetExportName(const Document::ValueType& Object, std::string& Name)
    {
        auto itrMeta = Object.FindMember("meta");
        if (itrMeta != Object.MemberEnd())
        {
            auto itr = itrMeta->value.FindMember("name");
            if (itr != itrMeta->value.MemberEnd())
            {
                Name = itr->value.GetString();
            }
        }
        
    }

    std::shared_ptr<LuaEnum> ParseEnum(const Document::ValueType& EnumObject)
    {
        std::shared_ptr<LuaEnum> Enum = std::make_shared<LuaEnum>();
        Enum->Name = EnumObject["name"].GetString();
        TryGetExportName(EnumObject, Enum->ExportName);
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
        TryGetExportName(ClassObject, Class->ExportName);
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
                Class->Nodes.push_back(ParseData(Unit));
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
        TryGetExportName(FunctionObject, Function->ExportName);
        Function->Type = FunctionType::Common;
        Function->IsCFunction = false;
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
        std::string FunctionObjectMacro = FunctionObject["macro"].GetString();
        if (FunctionObjectMacro == FunctionPropertyMacro)
        {
            if (IsStatic || !IsMemberFunc)
            {
                std::stringstream ss;
                ss << "Property :" << Function->Name << "must not be static and should be member func, Line: " << FunctionObject["line"].GetInt();
                throw ss.str();
            }
            Function->Type = FunctionType::Property;
            auto& FunctionMeta = FunctionObject["meta"];
            auto itrSetter = FunctionMeta.FindMember("setter");
            if (itrSetter != FunctionMeta.MemberEnd())
            {
                Function->SetterName = itrSetter->value.GetString();
            }
        } 
        else if (FunctionObjectMacro == CFunctionMacro)
        {
            Function->IsCFunction = true;
        }
        TryGetComment(FunctionObject, Function->Comment);
        return Function;
    }

    std::shared_ptr<LuaData> ParseData(const Document::ValueType& DataObject)
    {
        std::shared_ptr<LuaData> Data = std::make_shared<LuaData>();
        Data->Name = DataObject["name"].GetString();
        TryGetExportName(DataObject, Data->ExportName);
        Data->Writeable = !DataObject["meta"].HasMember("readonly");
        Data->IsStatic = DataObject["dataType"].HasMember("static");
        Data->IsAutoNull = (DataObject["macro"].GetString() == AutoNullMacro);
        TryGetComment(DataObject, Data->Comment);
        return Data;
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
            << "void RegisterAPIs(luaportal::LuaState& LOL) \n{\n\tLOL.GlobalContext()\n"
            << ssNormal.str()
            << ssGlobal.str() 
            << "\t;\n}\n\n"
            << "void UnregisterStaticLuaProperties() \n{\n"
            << ssAutoNull.str()
            << "}\n";
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
    impl->AutoNullMacro = AutoNullMacro;
    impl->FunctionPropertyMacro = FunctionPropertyMacro;
    impl->CFunctionMacro = CFunctionMacro;
    return impl->ParseAST(InputFile);
}

std::string CodeGenerator::GetResult()
{
    return impl->GetResult();
}
