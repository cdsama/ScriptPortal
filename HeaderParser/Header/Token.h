#pragma once

#include <cstdint>
#include <string>
#include <array>

namespace hp
{
    enum class TokenType
    {
        None,
        Symbol,
        Identifier,
        Const
    };

    enum class ConstType
    {
        String,
        Boolean,
        UInt32,
        Int32,
        UInt64,
        Int64,
        Real
    };

    struct Token
    {
        TokenType Type;
        std::size_t startPos;
        std::size_t StartLine;
        std::string token;

        ConstType constType;
        union
        {
            bool boolConst;
            uint32_t uint32Const;
            int32_t int32Const;
            uint64_t uint64Const;
            int64_t int64Const;
            double realConst;
        };
        std::string stringConst;
    };
}