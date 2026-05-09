#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ParseFunction;
	class ParseArray : public BaseParse
	{
	private:
		BaseParse *_value;
		BaseParse* _value2;
		BaseParse *_variable;
		BaseParse *_assignment;
		BaseParse* _assign;
		ParseFunction* _function;
		BaseParse* _preRun;

	public:
		ParseArray(const std::wstring& line);
		virtual ~ParseArray();

		unsigned int lineCount() const override;
		void simplify() override;

		void setValue(BaseParse* value);
		void setValue2(BaseParse* value);
		void setVariable(BaseParse* value);
		void setAssignment(BaseParse* value);
		void setAssign(BaseParse* value);
		void setPreRun(BaseParse* value);
		void setFunction(ParseFunction* func);

		BaseParse* value() const;
		BaseParse* value2() const;
		BaseParse* variable() const;
		BaseParse* assignment() const;
		BaseParse* assign() const;
		BaseParse* preRun() const;
		ParseFunction* function() const;
	};

}

