#pragma once
#include "BaseParse.h"

namespace XScript
{
    class ParseArguments;
    class ParseVariable;
    class ParseFunction : public BaseParse
    {
    private:
        std::wstring _function;
        ParseArguments* _args;
        BaseParse* _object;
        ParseVariable* _retvar;
        ParseCondition* _condition;
        bool            _postRun;

    public:
        ParseFunction(const std::wstring& line, const std::wstring &str);
        virtual ~ParseFunction();

        void simplify() override;

        unsigned int lineCount() const override;

        void setArguments(ParseArguments* args);
        void setObject(BaseParse* obj);
        void setReturnVariable(ParseVariable* retvar);
        void setCondition(ParseCondition* cond);
        void setPostRun(bool postRun);

        const std::wstring& function() const;
        BaseParse* object() const;
        ParseArguments* arguments() const;
        ParseVariable* returnVariable() const;
        ParseCondition* condition() const;
        bool isPostRun() const;
    };

}

