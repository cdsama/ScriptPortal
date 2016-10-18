#include "Parser.h"
#include "Token.h"

#include <iostream>
#include <vector>
#include <algorithm>

namespace hp {

    //-------------------------------------------------------------------------------------------------
    // Class used to write a typenode structure to json
    //-------------------------------------------------------------------------------------------------
    class TypeNodeWriter : public TypeNodeVisitor
    {
    public:
        TypeNodeWriter(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) :
            Writer(writer) {}

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(FunctionNode& node) override
        {
            Writer.String("type");
            Writer.String("function");

            Writer.String("returnType");
            VisitNode(*node.returns);

            Writer.String("arguments");
            Writer.StartArray();
            for (auto& arg : node.arguments)
            {
                Writer.StartObject();
                if (!arg->name.empty())
                {
                    Writer.String("name");
                    Writer.String(arg->name.c_str());
                }
                Writer.String("type");
                VisitNode(*arg->type);
                Writer.EndObject();
            }
            Writer.EndArray();
        }

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(LReferenceNode& node) override
        {
            Writer.String("type");
            Writer.String("lreference");

            Writer.String("baseType");
            VisitNode(*node.base);
        }

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(LiteralNode& node) override
        {
            Writer.String("type");
            Writer.String("literal");

            Writer.String("name");
            Writer.String(node.name.c_str());
        }

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(PointerNode& node) override
        {
            Writer.String("type");
            Writer.String("pointer");

            Writer.String("baseType");
            VisitNode(*node.base);
        }

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(ReferenceNode& node) override
        {
            Writer.String("type");
            Writer.String("reference");

            Writer.String("baseType");
            VisitNode(*node.base);
        }

        //-------------------------------------------------------------------------------------------------
        virtual void Visit(TemplateNode& node) override
        {
            Writer.String("type");
            Writer.String("template");

            Writer.String("name");
            Writer.String(node.name.c_str());

            Writer.String("arguments");
            Writer.StartArray();
            for (auto& arg : node.arguments)
                VisitNode(*arg);
            Writer.EndArray();
        }

        //-------------------------------------------------------------------------------------------------
        virtual void VisitNode(TypeNode &node) override
        {
            Writer.StartObject();
            if (node.isConst)
            {
                Writer.String("const");
                Writer.Bool(true);
            }
            if (node.isMutable)
            {
                Writer.String("mutable");
                Writer.Bool(true);
            }
            if (node.isVolatile)
            {
                Writer.String("volatile");
                Writer.Bool(true);
            }
            if (node.isStatic)
            {
                Writer.String("static");
                Writer.Bool(true);
            }
            TypeNodeVisitor::VisitNode(node);
            Writer.EndObject();
        }

    private:
        rapidjson::PrettyWriter<rapidjson::StringBuffer> &Writer;
    };

    //--------------------------------------------------------------------------------------------------
    Parser::Parser(const Options &options) : options(options), Writer(Buffer), phase(Phase::ParseEnded)
    {

    }
    
    //--------------------------------------------------------------------------------------------------
    Parser::~Parser()
    {
        
    }


    void Parser::Open()
    {
        if (phase != Phase::ParseEnded)
        {
            throw;
        }
        Writer.StartArray();
        phase = Phase::Parsing;
    }

    //--------------------------------------------------------------------------------------------------
    bool Parser::Parse(const char *Input, const char* FileName)
    {
        if (phase != Phase::Parsing)
        {
            throw;
        }

        // Pass the input to the tokenizer
        Reset(Input);

        try
        {
            Writer.StartObject();
            Writer.String("file");
            Writer.String(FileName);
            Writer.String("content");
            // Start the array
            Writer.StartArray();

            // Reset scope
            TopScope = Scopes;
            TopScope->name = "";
            TopScope->type = ScopeType::Global;
            TopScope->currentAccessControlType = AccessControlType::Public;

            // Parse all statements in the file
            while (ParseStatement())
            {

            }

            // End the array
            Writer.EndArray();
            Writer.EndObject();
        }
        catch (std::string e)
        {
            std::cerr<<"Error: "<< e <<"\nFile: "<< FileName <<"\t Line: "<< CursorLine << std::endl;
            return false;
        }

        return true;
    }

    void Parser::Close()
    {
        if (phase != Phase::Parsing)
        {
            throw;
        }
        phase = Phase::ParseEnded;
        Writer.EndArray();
    }

    std::string Parser::result() const
    {
        if (phase != Phase::ParseEnded)
        {
            throw;
        }
        return std::string(Buffer.GetString(), Buffer.GetString() + Buffer.GetSize());
    }

    //--------------------------------------------------------------------------------------------------
    bool Parser::ParseStatement()
    {
        Token token;
        if (!GetToken(token))
        {
            return false;
        }

        if (!ParseDeclaration(token))
        {
            throw std::string("ParseDeclaration failed");
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------
    bool Parser::ParseDeclaration(Token &token)
    {
        std::vector<std::string>::const_iterator customMacroIt;
        if (token.token == "#")
        {
            ParseDirective();
        }
        else if (token.token == ";")
        {
            ; // Empty statement
        }
        else if (token.token == options.EnumNameMacro)
        {
            ParseEnum(token);
        }
        else if (token.token == options.ClassNameMacro)
        {
            ParseClass(token);
        }
        else if ((customMacroIt = std::find(options.FunctionNameMacro.begin(), options.FunctionNameMacro.end(), token.token)) != options.FunctionNameMacro.end())
        {
            ParseFunction(token, *customMacroIt);
        }
        else if ((customMacroIt = std::find(options.PropertyNameMacro.begin(), options.PropertyNameMacro.end(), token.token)) != options.PropertyNameMacro.end())
        {
            ParseProperty(token, *customMacroIt);
        }
        else if (token.token == "namespace")
        {
            ParseNamespace();
        }
        else if (ParseAccessControl(token, TopScope->currentAccessControlType))
        {
            RequireSymbol(":");
        }
        else if ((customMacroIt = std::find(options.CustomMacros.begin(), options.CustomMacros.end(), token.token)) != options.CustomMacros.end())
        {
            ParseCustomMacro(token, *customMacroIt);
        }
        else
        {
            return SkipDeclaration(token);
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseDirective()
    {
        Token token;

        // Check the compiler directive
        if (!GetIdentifier(token))
        {
            throw std::string("Missing compiler directive after #"); // Missing compiler directive after #
        }

        bool multiLineEnabled = false;
        if (token.token == "define")
        {
            multiLineEnabled = true;
        }
        else if (token.token == "include")
        {
            Token includeToken;
            GetToken(includeToken, true);

            Writer.StartObject();
            Writer.String("type");
            Writer.String("include");
            Writer.String("file");
            Writer.String(includeToken.token.c_str());
            Writer.EndObject();
        }

        // Skip past the end of the token
        char lastChar = '\n';
        do
        {
            // Skip to the end of the line
            char c;
            while (!this->is_eof() && (c = GetChar()) != '\n')
            {
                lastChar = c;
            }

        } while (multiLineEnabled && lastChar == '\\');
    }

    //--------------------------------------------------------------------------------------------------
    bool Parser::SkipDeclaration(Token &token)
    {
        int32_t scopeDepth = 0;
        while (GetToken(token))
        {
            if (token.token == ";" && scopeDepth == 0)
            {
                break;
            }

            if (token.token == "{")
            {
                scopeDepth++;
            }

            if (token.token == "}")
            {
                --scopeDepth;
                if (scopeDepth == 0)
                {
                    break;
                }
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseEnum(Token &startToken)
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("enum");
        Writer.String("line");
        Writer.Uint((unsigned)startToken.StartLine);

        WriteCurrentAccessControlType();

        ParseComment(true);
        ParseMacroMeta();

        RequireIdentifier("enum");

        // C++1x enum class type?
        bool isEnumClass = MatchIdentifier("class");

        // Parse enum name
        Token enumToken;
        if (!GetIdentifier(enumToken))
        {
            throw std::string("Missing enum name"); // Missing enum name?
        }

        Writer.String("name");
        Writer.String(enumToken.token.c_str());

        if (isEnumClass)
        {
            Writer.String("cxxclass");
            Writer.Bool(isEnumClass);
        }

        // Parse C++1x enum base
        if (isEnumClass && MatchSymbol(":"))
        {
            Token baseToken;
            if (!GetIdentifier(baseToken))
            {
                throw std::string("Missing enum base"); // Missing enum base
            }

            // Validate base token
            Writer.String("base");
            Writer.String(baseToken.token.c_str());
        }

        // Require opening brace
        RequireSymbol("{");

        Writer.String("members");
        Writer.StartArray();

        // Parse all the values
        Token token;
        while (GetIdentifier(token))
        {
            Writer.StartObject();

            // Store the identifier
            Writer.String("key");
            Writer.String(token.token.c_str());

            // Parse constant
            if (MatchSymbol("="))
            {
                // Just parse the value, not doing anything with it atm
                std::string value;
                while (GetToken(token) && (token.Type != TokenType::Symbol || (token.token != "," && token.token != "}")))
                {
                    value += token.token;
                }
                UngetToken(token);

                Writer.String("value");
                Writer.String(value.c_str());
            }

            Writer.EndObject();

            // Next value?
            if (!MatchSymbol(","))
            {
                break;
            }
        }

        RequireSymbol("}");
        Writer.EndArray();

        MatchSymbol(";");

        Writer.EndObject();
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseMacroMeta()
    {
        Writer.String("meta");

        RequireSymbol("(");
        ParseMetaSequence();

        // Possible ;
        MatchSymbol(";");
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseMetaSequence()
    {
        Writer.StartObject();

        if (!MatchSymbol(")"))
        {
            do
            {
                // Parse key value
                Token keyToken;
                if (!GetIdentifier(keyToken))
                {
                    throw std::string("Expected identifier"); // Expected identifier
                }

                Writer.String(keyToken.token.c_str());

                // Simple value?
                if (MatchSymbol("="))
                {
                    Token token;
                    if (!GetToken(token))
                    {
                        throw std::string("Expected token"); // Expected token
                    }

                    WriteToken(token);
                }
                else if (MatchSymbol("(")) // Compound value
                {
                    ParseMetaSequence();
                }
                else // No value
                {
                    Writer.Null();
                }
            } while (MatchSymbol(","));

            MatchSymbol(")");
        }

        Writer.EndObject();
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::PushScope(const std::string &name, ScopeType scopeType, AccessControlType accessControlType)
    {
        if (TopScope == Scopes + (sizeof(Scopes) / sizeof(Scope)) - 1)
        {
            throw std::string("Max scope depth"); // Max scope depth
        }

        TopScope++;
        TopScope->type = scopeType;
        TopScope->name = name;
        TopScope->currentAccessControlType = accessControlType;
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::PopScope()
    {
        if (TopScope == Scopes)
        {
            throw std::string("Scope error"); // Scope error
        }

        TopScope--;
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseNamespace()
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("namespace");

        Token token;
        if (!GetIdentifier(token))
        {
            throw std::string("Missing namespace name"); // Missing namespace name
        }

        Writer.String("name");
        Writer.String(token.token.c_str());

        RequireSymbol("{");

        Writer.String("members");
        Writer.StartArray();

        PushScope(token.token, ScopeType::Namespace, AccessControlType::Public);

        while (!MatchSymbol("}"))
        {
            ParseStatement();
        }

        PopScope();

        Writer.EndArray();

        Writer.EndObject();
    }

    //-------------------------------------------------------------------------------------------------
    bool Parser::ParseAccessControl(const Token &token, AccessControlType& type)
    {
        if (token.token == "public")
        {
            type = AccessControlType::Public;
            return true;
        }
        else if (token.token == "protected")
        {
            type = AccessControlType::Protected;
            return true;
        }
        else if (token.token == "private")
        {
            type = AccessControlType::Private;
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------------------------------
    void Parser::WriteCurrentAccessControlType()
    {
        // Writing access is not required if the current scope is not owned by a class
        if (TopScope->type != ScopeType::Class)
        {
            return;
        }

        WriteAccessControlType(current_access_control_type());
    }

    //-------------------------------------------------------------------------------------------------
    void Parser::WriteAccessControlType(AccessControlType type)
    {
        Writer.String("access");
        switch (type)
        {
        case AccessControlType::Public:
            Writer.String("public");
            break;
        case AccessControlType::Protected:
            Writer.String("protected");
            break;
        case AccessControlType::Private:
            Writer.String("private");
            break;
        default:
            throw std::string("Unknown access control type"); // Unknown access control type
        }
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseClass(Token &token)
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("class");
        Writer.String("line");
        Writer.Uint((unsigned)token.StartLine);

        WriteCurrentAccessControlType();

        ParseComment(true);
        ParseMacroMeta();

        RequireIdentifier("class");

        // Get the class name
        Token classNameToken;
        if (!GetIdentifier(classNameToken))
        {
            throw std::string("Missing class name"); // Missing class name
        }

        Writer.String("name");
        Writer.String(classNameToken.token.c_str());

        if (MatchIdentifier("final"))
        {

        }

        // Match base types
        if (MatchSymbol(":"))
        {
            Writer.String("parents");
            Writer.StartArray();

            do
            {
                Writer.StartObject();

                Token accessOrName;
                if (!GetIdentifier(accessOrName))
                {
                    throw std::string("Missing class or access control specifier"); // Missing class or access control specifier
                }

                // Parse the access control specifier
                AccessControlType accessControlType = AccessControlType::Private;
                if (!ParseAccessControl(accessOrName, accessControlType))
                    UngetToken(accessOrName);
                WriteAccessControlType(accessControlType);

                // Get the name of the class
                Writer.String("name");
                ParseType();

                Writer.EndObject();
            } while (MatchSymbol(","));

            Writer.EndArray();
        }

        RequireSymbol("{");

        Writer.String("members");
        Writer.StartArray();

        PushScope(classNameToken.token, ScopeType::Class, AccessControlType::Private);

        while (!MatchSymbol("}"))
        {
            ParseStatement();
        }

        PopScope();

        Writer.EndArray();

        RequireSymbol(";");

        Writer.EndObject();
    }

    //-------------------------------------------------------------------------------------------------
    void Parser::ParseProperty(Token &token, const std::string& macroName)
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("property");
        Writer.String("macro");
        Writer.String(macroName.c_str());
        Writer.String("line");
        Writer.Uint((unsigned)token.StartLine);

        ParseComment();
        ParseMacroMeta();
        WriteCurrentAccessControlType();

        // Check mutable
        bool isMutable = MatchIdentifier("mutable");
        if (isMutable)
        {
            Writer.String("mutable");
            Writer.Bool(true);
        }

        // Parse the type
        Writer.String("dataType");
        ParseType();

        // Parse the name
        Token nameToken;
        if (!GetIdentifier(nameToken))
        {
            throw std::string("Expected a property name"); // Expected a property name
        }

        Writer.String("name");
        Writer.String(nameToken.token.c_str());

        Writer.EndObject();

        // Skip until the end of the definition
        Token t;
        while (GetToken(t))
        {
            if (t.token == ";")
            {
                break;
            }
        }

    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseFunction(Token &token, const std::string& macroName)
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("function");
        Writer.String("macro");
        Writer.String(macroName.c_str());
        Writer.String("line");
        Writer.Uint((unsigned)token.StartLine);

        ParseComment();


        ParseMacroMeta();
        WriteCurrentAccessControlType();

        // Process method specifiers in any particular order
        bool isVirtual = false, isInline = false, isConstExpr = false, isStatic = false;
        for (bool matched = true; matched;)
        {
            matched = (!isVirtual && (isVirtual = MatchIdentifier("virtual"))) ||
                (!isInline && (isInline = MatchIdentifier("inline"))) ||
                (!isConstExpr && (isConstExpr = MatchIdentifier("constexpr"))) ||
                (!isStatic && (isStatic = MatchIdentifier("static"))) ;
        }

        // Write method specifiers
        if (isVirtual)
        {
            Writer.String("virtual");
            Writer.Bool(isVirtual);
        }
        if (isInline)
        {
            Writer.String("inline");
            Writer.Bool(isInline);
        }
        if (isConstExpr)
        {
            Writer.String("constexpr");
            Writer.Bool(isConstExpr);
        }
        if (isStatic)
        {
            Writer.String("static");
            Writer.Bool(isStatic);
        }

        // Parse the return type
        Writer.String("returnType");
        ParseType();

        // Parse the name of the method
        Token nameToken;
        if (!GetIdentifier(nameToken))
        {
            throw std::string("Expected method name"); // Expected method name
        }

        Writer.String("name");
        Writer.String(nameToken.token.c_str());

        Writer.String("arguments");
        Writer.StartArray();

        // Start argument list from here
        MatchSymbol("(");

        // Is there an argument list in the first place or is it closed right away?
        if (!MatchSymbol(")"))
        {
            // Walk over all arguments
            do
            {
                Writer.StartObject();

                // Get the type of the argument
                Writer.String("type");
                ParseType();

                // Parse the name of the function
                Writer.String("name");
                if (!GetIdentifier(nameToken))
                {
                    throw std::string("Expected identifier"); // Expected identifier
                }
                Writer.String(nameToken.token.c_str());

                // Parse default value
                if (MatchSymbol("="))
                {
                    Writer.String("defaultValue");

                    std::string defaultValue;
                    Token token;
                    GetToken(token);
                    if (token.Type == TokenType::Const)
                    {
                        WriteToken(token);
                    }
                    else
                    {
                        do
                        {
                            if (token.token == "," ||
                                token.token == ")")
                            {
                                UngetToken(token);
                                break;
                            }
                            defaultValue += token.token;
                        } while (GetToken(token));
                        Writer.String(defaultValue.c_str());
                    }
                }

                Writer.EndObject();
            } while (MatchSymbol(",")); // Only in case another is expected

            MatchSymbol(")");
        }

        Writer.EndArray();

        // Optionally parse constness
        if (MatchIdentifier("const"))
        {
            Writer.String("const");
            Writer.Bool(true);
        }

        // Pure?
        if (MatchSymbol("="))
        {
            Token token;
            if (!GetToken(token) || token.token != "0")
            {
                throw std::string("Expected nothing else than null"); // Expected nothing else than null
            }

            Writer.String("abstract");
            Writer.Bool(true);
        }

        Writer.EndObject();

        // Skip either the ; or the body of the function
        Token skipToken;
        SkipDeclaration(skipToken);
    }

    //-------------------------------------------------------------------------------------------------
    void Parser::ParseComment(bool WithNamespace)
    {
        auto& TheComment = WithNamespace ? ThisComment : LastComment;
        std::string comment = TheComment.EndLine == CursorLine ? TheComment.Text : "";
        if (!comment.empty())
        {
            Writer.String("comment");
            Writer.String(comment.c_str());
        }
    }

    //--------------------------------------------------------------------------------------------------
    void Parser::ParseType()
    {
        std::unique_ptr<TypeNode> node = ParseTypeNode();
        TypeNodeWriter writer(Writer);
        writer.VisitNode(*node);
    }

    //-------------------------------------------------------------------------------------------------
    std::unique_ptr<TypeNode> Parser::ParseTypeNode()
    {
        std::unique_ptr<TypeNode> node;
        Token token;

        bool isConst = false, isVolatile = false, isMutable = false, isStatic = false;
        for (bool matched = true; matched;)
        {
            matched = (!isConst && (isConst = MatchIdentifier("const"))) ||
                (!isVolatile && (isVolatile = MatchIdentifier("volatile"))) ||
                (!isMutable && (isMutable = MatchIdentifier("mutable"))) || 
                (!isStatic && (isStatic = MatchIdentifier("static")));
        }

        // Parse a literal value
        std::string declarator = ParseTypeNodeDeclarator();

        // Postfix const specifier
        isConst |= MatchIdentifier("const");

        // Template?
        if (MatchSymbol("<"))
        {
            std::unique_ptr<TemplateNode> templateNode(new TemplateNode(declarator));
            do
            {
                templateNode->arguments.emplace_back(ParseTypeNode());
            } while (MatchSymbol(","));

            if (!MatchSymbol(">")) {
                throw std::string("Expected >"); // Expected >
            }

            node.reset(templateNode.release());
        }
        else
        {
            node.reset(new LiteralNode(declarator));
        }

        // Store gathered stuff
        node->isConst = isConst;

        // Check reference or pointer types
        while (GetToken(token))
        {
            if (token.token == "&")
            {
                node.reset(new ReferenceNode(std::move(node)));
            }
            else if (token.token == "&&")
            {
                node.reset(new LReferenceNode(std::move(node)));
            }
            else if (token.token == "*")
            {
                node.reset(new PointerNode(std::move(node)));
            }
            else
            {
                UngetToken(token);
                break;
            }

            if (MatchIdentifier("const"))
            {
                node->isConst = true;
            }
        }

        // Function pointer?
        if (MatchSymbol("("))
        {
            // Parse void(*)(args, ...)
            //            ^
            //            |
            if (MatchSymbol("*"))
            {
                Token token;
                GetToken(token);
                if (token.token != ")" || (token.Type != TokenType::Identifier && !MatchSymbol(")")))
                {
                    throw std::string("Expected ) or Identifier )");
                }
            }

            // Parse arguments
            std::unique_ptr<FunctionNode> funcNode(new FunctionNode());
            funcNode->returns = std::move(node);

            if (!MatchSymbol(")"))
            {
                do
                {
                    std::unique_ptr<FunctionNode::Argument> argument(new FunctionNode::Argument);
                    argument->type = ParseTypeNode();

                    // Get , or name identifier
                    if (!GetToken(token))
                    {
                        throw std::string("Unexpected end of file"); // Unexpected end of file
                    }

                    // Parse optional name
                    if (token.Type == TokenType::Identifier)
                    {
                        argument->name = token.token;
                    }
                    else
                    {
                        UngetToken(token);
                    }

                    funcNode->arguments.emplace_back(std::move(argument));

                } while (MatchSymbol(","));
                if (!MatchSymbol(")"))
                {
                    throw std::string("Expected ) ");
                }
            }

            node = std::move(funcNode);
        }

        // This stuff refers to the top node
        node->isVolatile = isVolatile;
        node->isMutable = isMutable;
        node->isStatic = isStatic;

        return std::move(node);
    }

    //-------------------------------------------------------------------------------------------------
    std::string Parser::ParseTypeNodeDeclarator()
    {
        // Skip optional forward declaration specifier
        MatchIdentifier("class");
        MatchIdentifier("struct");
        MatchIdentifier("typename");

        // Parse a type name 
        std::string declarator;
        Token token;
        bool first = true;
        do
        {
            // Parse the declarator
            if (MatchSymbol("::"))
            {
                declarator += "::";
            }
            else if (!first)
            {
                break;
            }

            // Mark that this is not the first time in this loop
            first = false;

            // Match an identifier
            if (!GetIdentifier(token))
            {
                throw std::string("Expected identifier"); // Expected identifier
            }
            declarator += token.token;

        } while (true);

        return declarator;
    }

    //-------------------------------------------------------------------------------------------------
    std::string Parser::ParseTypename()
    {
        return "";
    }

    //----------------------------------------------------------------------------------------------------------------------
    void Parser::WriteToken(const Token &token)
    {
        if (token.Type == TokenType::Const)
        {
            switch (token.constType)
            {
            case ConstType::Boolean:
                Writer.Bool(token.boolConst);
                break;
            case ConstType::UInt32:
                Writer.Uint(token.uint32Const);
                break;
            case ConstType::Int32:
                Writer.Int(token.int32Const);
                break;
            case ConstType::UInt64:
                Writer.Uint64(token.uint64Const);
                break;
            case ConstType::Int64:
                Writer.Int64(token.int64Const);
                break;
            case ConstType::Real:
                Writer.Double(token.realConst);
                break;
            case ConstType::String:
                //writer_.String((std::string("\"") + token.stringConst + "\"").c_str());
                Writer.String(token.stringConst.c_str());
                break;
            }
        }
        else
        {
            Writer.String(token.token.c_str());
        }
    }

    //-------------------------------------------------------------------------------------------------
    void Parser::ParseCustomMacro(Token & token, const std::string& macroName)
    {
        Writer.StartObject();
        Writer.String("type");
        Writer.String("macro");
        Writer.String("name");
        Writer.String(macroName.c_str());
        Writer.String("line");
        Writer.Uint((unsigned)token.StartLine);

        WriteCurrentAccessControlType();

        ParseMacroMeta();

        Writer.EndObject();
    }

}