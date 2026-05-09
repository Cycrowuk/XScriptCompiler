#pragma once
#include "BaseParse.h"

namespace XScript {

    enum class SymbolType
    {
        Unknown,
        OpenBracket,
        CloseBracket,
        Comma,
        Object,
        Function,
        Assignment,
        End,
        StartBlock,
        EndBlock,
        OpenArray,
        CloseArray,
        InlineElse,
        DefineLabel,
        Namespace,
        Preprocessor,
        Increment,      // ++
        Decrement,      // --
        PlusAssign,     // +=
        MinusAssign,    // -=
        MultiplyAssign, // *=
        DivideAssign,   // /=
    };

    class ParseSymbol : public BaseParse       
    {
    private:
        SymbolType _symbol;
        std::wstring _str;

    public:
        ParseSymbol(const std::wstring& line, const std::wstring &symbol);
        virtual ~ParseSymbol();

        void switchSymbol();

        std::wstring stringData() const override;
        SymbolType symbol() const;

    private:
        SymbolType _convertSymbol(const std::wstring& symbol) const;
    };

}

