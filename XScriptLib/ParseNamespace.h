#pragma once
#include "BaseParse.h"

namespace XScript
{

    class ParseNamespace : public BaseParse
    {
    private:
        std::wstring    _namespace;
        std::wstring    _keyword;

    public:
        ParseNamespace(const std::wstring& line);

        const std::wstring& namespaceString() const;
        const std::wstring& keyword() const;

        void setNamespace(const std::wstring& str);
        void setKeyword(const std::wstring& str);
    };

}