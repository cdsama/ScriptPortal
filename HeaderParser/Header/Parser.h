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

    class Parser : private Tokenizer
    {
    public:
        Parser(const Options& options);
        virtual ~Parser();

        // No copying of parser
        Parser(const Parser& other) = delete;
        Parser(Parser&& other) = delete;

        // Parses the given input
        bool Parse(const char* Input);

        /// Returns the result of a previous parse
        std::string result() const { return std::string(Buffer.GetString(), Buffer.GetString() + Buffer.GetSize()); }

    protected:
        /// Called to parse the next statement. Returns false if there are no more statements.
        bool ParseStatement();
        bool ParseDeclaration(Token &token);
        void ParseDirective();
        bool SkipDeclaration(Token &token);
        void ParseEnum(Token &token);
        void ParseMacroMeta();
        void ParseMetaSequence();

        void PushScope(const std::string& name, ScopeType scopeType, AccessControlType accessControlType);
        void PopScope();

        void ParseNamespace();
        bool ParseAccessControl(const Token& token, AccessControlType& type);

        AccessControlType current_access_control_type() const { return TopScope->currentAccessControlType; }
        void WriteCurrentAccessControlType();

        void WriteAccessControlType(AccessControlType type);
        void ParseClass(Token &token);
        void ParseFunction(Token &token, const std::string& macroName);

        void ParseComment();

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

        Scope Scopes[64];
        Scope *TopScope;

        void ParseProperty(Token &token);
    };
}