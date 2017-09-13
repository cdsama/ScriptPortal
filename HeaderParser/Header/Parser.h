#pragma once

#include "Tokenizer.h"
#include "Options.h"
#include "TypeNodes.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string>

namespace hp
{
    enum class ScopeType
    {
        Global,
        Namespace,
        Class
    };

    enum class AccessControlType
    {
        Public,
        Private,
        Protected
    };

    enum class Phase
    {
        Parsing,
        ParseEnded,
    };

    class Parser : private Tokenizer
    {
    public:
        Parser(const Options& options);
        virtual ~Parser();

        // No copying of parser
        Parser(const Parser& other) = delete;
        Parser(Parser&& other) = delete;

        void Open();
        // Parses the given input
        bool Parse(const char* Input, const char* FileName);

        void Close();

        /// Returns the result of a previous parse
        std::string result() const;

    protected:
        /// Called to parse the next statement. Returns false if there are no more statements.
        bool ParseStatement(Token* const CurrentClass = nullptr);
        bool ParseDeclaration(Token &token, Token* const CurrentClass = nullptr);
        void ParseDirective();
        bool SkipDeclaration(Token &token);
        void ParseEnum(Token &token);
        void ParseMacroMeta();
        void ParseMetaSequence();

        void PushScope(const std::string& name, ScopeType scopeType, AccessControlType accessControlType);
        void PopScope();

        void ParseNamespace(Token* const macrotoken = nullptr);
        bool ParseAccessControl(const Token& token, AccessControlType& type);

        AccessControlType current_access_control_type() const { return TopScope->currentAccessControlType; }
        void WriteCurrentAccessControlType();

        void WriteAccessControlType(AccessControlType type);
        void ParseClass(Token &token);
        void ParseConstructor(Token &token, Token* const CurrentClass = nullptr);
        void ParseFunction(Token &token, const std::string& macroName);

        void ParseComment(bool WithNamespace = false);

        void ParseType();

        std::unique_ptr<TypeNode> ParseTypeNode();
        std::string ParseTypeNodeDeclarator();

        std::string ParseTypename();

        void WriteToken(const Token &token);
        void ParseCustomMacro(Token & token, const std::string& macroName);
    private:
        Options options;
        rapidjson::StringBuffer Buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer;

        struct Scope
        {
            ScopeType type;
            std::string name;
            AccessControlType currentAccessControlType;
        };

        Scope Scopes[128];
        Scope *TopScope;
        Phase phase;
        void ParseProperty(Token &token, const std::string& macroName);
    };
}