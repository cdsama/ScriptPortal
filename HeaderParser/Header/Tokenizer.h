#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

namespace hp
{
    struct Token;

    class Tokenizer
    {
    public:
        Tokenizer();
        virtual ~Tokenizer();

        // Do not allow copy or move
        Tokenizer(const Tokenizer& other) = delete;
        Tokenizer(Tokenizer &&other) = delete;

        /// Reset the parser with the given Input text
        void Reset(const char* input, std::size_t startingLine = 1);

        /// Parses a token from the stream
        bool GetToken(Token& token, bool angleBracketsForStrings = false, bool seperateBraces = false);

        /// Parses an identifier from the stream
        bool GetIdentifier(Token& token);

        /// Returns a token to the stream, effectively resetting the cursor to the start of the token
        void UngetToken(const Token &token);

    protected:
        /**
        * @brief Returns the next character from the stream.
        * @details Returns the next character from the stream while advancing the cursor position.
        */
        char GetChar();

        /// Resets the cursor to the last read character
        void UngetChar();

        /// Returns the next character from the stream but skips comments and white spaces.
        char GetLeadingChar();

        /// Returns the next character from the stream without modifying the cursor position.
        char peek() const;

        /// Returns true if the stream is at the end
        bool is_eof() const;

    protected:
        /// Returns true if the current token is an identifier with the given text
        bool MatchIdentifier(const char* identifier);

        /// Returns true if the current token is a symbol with the given text
        bool MatchSymbol(const char* symbol);

        /// Advances the tokenizer past the expected identifier or errors if the symbol is not encountered.
        void RequireIdentifier(const char* identifier);

        /// Advances the tokenizer past the expected symbol or errors if the symbol is not encountered.
        void RequireSymbol(const char* symbol);

    protected:
        /// The input
        const char *Input = nullptr;

        /// The length of the input
        std::size_t InputLength = 0;

        /// Current position in the input
        std::size_t CursorPos = 0;

        /// Current line of the cursor
        std::size_t CursorLine = 0;

        /// The cursor position of the last read character
        std::size_t PrevCursorPos = 0;

        /// The cursor line of the the last read character
        std::size_t PrevCursorLine = 0;

        /// Stores the last comment block
        struct Comment {
            std::string Text;
            std::size_t StartLine;
            std::size_t EndLine;
        };

        Comment ThisComment;
        Comment LastComment;
    };
}