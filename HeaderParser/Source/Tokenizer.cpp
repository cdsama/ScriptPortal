#include "Tokenizer.h"
#include "Token.h"

#include <string>
#include <cctype>
#include <stdexcept>
#include <vector>
#include <sstream>

namespace hp {
    static const char EndOfFileChar = std::char_traits<char>::to_char_type(std::char_traits<char>::eof());
    //--------------------------------------------------------------------------------------------------
    Tokenizer::Tokenizer() :
        Input(nullptr),
        InputLength(0),
        CursorPos(0),
        CursorLine(0)
    {

    }

    //--------------------------------------------------------------------------------------------------
    Tokenizer::~Tokenizer()
    {

    }

    //--------------------------------------------------------------------------------------------------
    void Tokenizer::Reset(const char* input, std::size_t startingLine)
    {
        Input = input;
        InputLength = std::char_traits<char>::length(input);
        CursorPos = 0;
        CursorLine = startingLine;
    }

    //--------------------------------------------------------------------------------------------------
    char Tokenizer::GetChar()
    {
        char c = Input[CursorPos];

        PrevCursorPos = CursorPos;
        PrevCursorLine = CursorLine;

        // New line moves the cursor to the new line
        if (c == '\n')
            CursorLine++;

        CursorPos++;
        return c;
    }

    //--------------------------------------------------------------------------------------------------
    void Tokenizer::UngetChar()
    {
        CursorLine = PrevCursorLine;
        CursorPos = PrevCursorPos;
    }

    //--------------------------------------------------------------------------------------------------
    char Tokenizer::peek() const
    {
        return !is_eof() ?
            Input[CursorPos] :
            EndOfFileChar;
    }

    //--------------------------------------------------------------------------------------------------
    char Tokenizer::GetLeadingChar()
    {
        if (!ThisComment.Text.empty())
        {
            LastComment = ThisComment;
        }

        ThisComment.Text = "";
        ThisComment.StartLine = CursorLine;
        ThisComment.EndLine = CursorLine;

        char c;
        for (c = GetChar(); !is_eof(); c = GetChar())
        {
            // If this is a whitespace character skip it
            std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);

            // In case of a new line
            if (c == '\n')
            {
                if (!ThisComment.Text.empty())
                {
                    ThisComment.Text += "\n";
                }
                continue;
            }

            if (std::isspace(intc) || std::iscntrl(intc))
            {
                continue;
            }

            // If this is a single line comment
            char next = peek();
            if (c == '/' && next == '/')
            {
                std::vector<std::string> lines;

                size_t indentationLastLine = 0;
                while (!is_eof() && c == '/' && next == '/')
                {
                    // Search for the end of the line
                    std::string line;
                    for (c = GetChar();
                        c != EndOfFileChar && c != '\n';
                        c = GetChar())
                    {
                        line += c;
                    }

                    // Store the line
                    size_t lastSlashIndex = line.find_first_not_of("/");
                    if (lastSlashIndex == std::string::npos)
                    {
                        line = "";
                    }
                    else
                    {
                        line = line.substr(lastSlashIndex);
                    }

                    size_t firstCharIndex = line.find_first_not_of(" \t");
                    if (firstCharIndex == std::string::npos)
                    {
                        line = "";
                    }
                    else
                    {
                        line = line.substr(firstCharIndex);
                    }

                    if (firstCharIndex > indentationLastLine && !lines.empty())
                    {
                        lines.back() += std::string(" ") + line;
                    }
                    else
                    {
                        lines.emplace_back(std::move(line));
                        indentationLastLine = firstCharIndex;
                    }

                    // Check the next line
                    while (!is_eof() && std::isspace(c = GetChar()));

                    if (!is_eof())
                    {
                        next = peek();
                    }
                }

                // Unget previously get char
                if (!is_eof())
                {
                    UngetChar();
                }

                // Build comment string
                std::stringstream ss;
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    if (i > 0)
                    {
                        ss << "\n";
                    }
                    ss << lines[i];
                }

                ThisComment.Text = ss.str();
                ThisComment.EndLine = CursorLine;

                // Go to the next
                continue;
            }

            // If this is a block comment
            if (c == '/' && next == '*')
            {
                // Search for the end of the block comment
                std::vector<std::string> lines;
                std::string line;
                for (c = GetChar(), next = peek();
                    c != EndOfFileChar && (c != '*' || next != '/');
                    c = GetChar(), next = peek())
                {
                    if (c == '\n')
                    {
                        if (!lines.empty() || !line.empty())
                        {
                            lines.emplace_back(line);
                        }
                        line.clear();
                    }
                    else
                    {
                        if (!line.empty() || !(std::isspace(c) || c == '*'))
                        {
                            line += c;
                        }
                    }
                }

                // Skip past the slash
                if (c != EndOfFileChar)
                {
                    GetChar();
                }

                // Skip past new lines and spaces
                while (!is_eof() && std::isspace(c = GetChar()));
                if (!is_eof())
                {
                    UngetChar();
                }

                // Remove empty lines from the back
                while (!lines.empty() && lines.back().empty())
                {
                    lines.pop_back();
                }

                // Build comment string
                std::stringstream ss;
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    if (i > 0)
                    {
                        ss << "\n";
                    }
                    ss << lines[i];
                }

                ThisComment.Text = ss.str();
                ThisComment.EndLine = CursorLine;

                // Move to the next character
                continue;
            }

            break;
        }

        return c;
    }

    //--------------------------------------------------------------------------------------------------
    bool Tokenizer::GetToken(Token &token, bool angleBracketsForStrings, bool seperateBraces)
    {
        // Get the next character
        char c = GetLeadingChar();
        char p = peek();
        std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);
        std::char_traits<char>::int_type intp = std::char_traits<char>::to_int_type(p);

        if (!std::char_traits<char>::not_eof(intc))
        {
            UngetChar();
            return false;
        }

        // Record the start of the token position
        token.startPos = PrevCursorPos;
        token.StartLine = PrevCursorLine;
        token.token.clear();
        token.Type = TokenType::None;

        // Alphanumeric token
        if (std::isalpha(intc) || c == '_')
        {
            // Read the rest of the alphanumeric characters
            do
            {
                token.token.push_back(c);
                c = GetChar();
                intc = std::char_traits<char>::to_int_type(c);
            } while (std::isalnum(intc) || c == '_');

            // Put back the last read character since it's not part of the identifier
            UngetChar();

            // Set the type of the token
            token.Type = TokenType::Identifier;

            if (token.token == "true")
            {
                token.Type = TokenType::Const;
                token.constType = ConstType::Boolean;
                token.boolConst = true;
            }
            else if (token.token == "false")
            {
                token.Type = TokenType::Const;
                token.constType = ConstType::Boolean;
                token.boolConst = false;
            }

            return true;
        }
        // Constant
        else if (std::isdigit(intc) || ((c == '-' || c == '+') && std::isdigit(intp)))
        {
            bool isFloat = false;
            bool isHex = false;
            bool isNegated = c == '-';
            do
            {
                if (c == '.')
                {
                    isFloat = true;
                }

                if (c == 'x' || c == 'X')
                {
                    isHex = true;
                }

                token.token.push_back(c);
                c = GetChar();
                intc = std::char_traits<char>::to_int_type(c);

            } while (std::isdigit(intc) ||
                (!isFloat && c == '.') ||
                (!isHex && (c == 'X' || c == 'x')) ||
                (isHex && std::isxdigit(intc)));

            if (!isFloat || (c != 'f' && c != 'F'))
            {
                UngetChar();
            }

            token.Type = TokenType::Const;
            if (!isFloat)
            {
                try
                {
                    if (isNegated)
                    {
                        token.int32Const = std::stoi(token.token, 0, 0);
                        token.constType = ConstType::Int32;
                    }
                    else
                    {
                        token.uint32Const = std::stoul(token.token, 0, 0);
                        token.constType = ConstType::UInt32;
                    }
                }
                catch (std::out_of_range)
                {
                    if (isNegated)
                    {
                        token.int64Const = std::stoll(token.token, 0, 0);
                        token.constType = ConstType::Int64;
                    }
                    else
                    {
                        token.uint64Const = std::stoull(token.token, 0, 0);
                        token.constType = ConstType::UInt64;
                    }
                }
            }
            else
            {
                token.realConst = std::stod(token.token);
                token.constType = ConstType::Real;
            }

            return true;
        }
        else if (c == '"' || (angleBracketsForStrings && c == '<'))
        {
            const char closingElement = c == '"' ? '"' : '>';

            c = GetChar();
            while (c != closingElement && std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
            {
                if (c == '\\')
                {
                    c = GetChar();
                    if (!std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
                    {
                        break;
                    }
                    else if (c == 'n')
                    {
                        c = '\n';
                    }
                    else if (c == 't')
                    {
                        c = '\t';
                    }
                    else if (c == 'r')
                    {
                        c = '\r';
                    }
                    else if (c == '"')
                    {
                        c = '"';
                    }
                }

                token.token.push_back(c);
                c = GetChar();
            }

            if (c != closingElement)
            {
                UngetChar();
            }

            token.Type = TokenType::Const;
            token.constType = ConstType::String;
            token.stringConst = token.token;

            return true;
        }
        // Symbol
        else
        {
            // Push back the symbol
            token.token.push_back(c);

#define PAIR(cc,dd) (c==cc&&d==dd) /* Comparison macro for two characters */
            const char d = GetChar();
            if (PAIR('<', '<') ||
                PAIR('-', '>') ||
                (!seperateBraces && PAIR('>', '>')) ||
                PAIR('!', '=') ||
                PAIR('<', '=') ||
                PAIR('>', '=') ||
                PAIR('+', '+') ||
                PAIR('-', '-') ||
                PAIR('+', '=') ||
                PAIR('-', '=') ||
                PAIR('*', '=') ||
                PAIR('/', '=') ||
                PAIR('^', '=') ||
                PAIR('|', '=') ||
                PAIR('&', '=') ||
                PAIR('~', '=') ||
                PAIR('%', '=') ||
                PAIR('&', '&') ||
                PAIR('|', '|') ||
                PAIR('=', '=') ||
                PAIR(':', ':')
                )
#undef PAIR
            {
                token.token.push_back(d);
            }
            else {
                UngetChar();
            }


            token.Type = TokenType::Symbol;

            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------------------------------
    bool Tokenizer::is_eof() const
    {
        return CursorPos >= InputLength;
    }

    //--------------------------------------------------------------------------------------------------
    bool Tokenizer::GetIdentifier(Token &token)
    {
        if (!GetToken(token))
        {
            return false;
        }

        if (token.Type == TokenType::Identifier)
        {
            return true;
        }

        UngetToken(token);
        return false;
    }

    //--------------------------------------------------------------------------------------------------
    void Tokenizer::UngetToken(const Token &token)
    {
        CursorLine = token.StartLine;
        CursorPos = token.startPos;
    }

    //--------------------------------------------------------------------------------------------------
    bool Tokenizer::MatchIdentifier(const char *identifier)
    {
        Token token;
        if (GetToken(token))
        {
            if (token.Type == TokenType::Identifier && token.token == identifier)
            {
                return true;
            }

            UngetToken(token);
        }

        return false;
    }

    //--------------------------------------------------------------------------------------------------
    bool Tokenizer::MatchSymbol(const char *symbol)
    {
        Token token;
        if (GetToken(token, false, std::char_traits<char>::length(symbol) == 1 && symbol[0] == '>'))
        {
            if (token.Type == TokenType::Symbol && token.token == symbol)
            {
                return true;
            }

            UngetToken(token);
        }

        return false;
    }

    //--------------------------------------------------------------------------------------------------
    void Tokenizer::RequireIdentifier(const char *identifier)
    {
        if (!MatchIdentifier(identifier))
        {
            throw std::string("Expected ").append(identifier);
        }
    }

    //--------------------------------------------------------------------------------------------------
    void Tokenizer::RequireSymbol(const char *symbol)
    {
        if (!MatchSymbol(symbol)) 
        {
            throw std::string("Expected ").append(symbol);
        }
    }
}