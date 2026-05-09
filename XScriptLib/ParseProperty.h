#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ParseFunction;
	class ParseProperty : public BaseParse
	{
	private:
		BaseParse* _object;
		std::wstring _property;
		BaseParse* _getter;
		BaseParse* _setter;
		ParseFunction* _getterFunction;
		ParseFunction* _setterFunction;

	public:
		ParseProperty(const std::wstring& line, const std::wstring& prop);
		virtual ~ParseProperty();

		void setObject(BaseParse* obj);
		void setSetter(BaseParse* setter);
		void setGetter(BaseParse* getter);
		void setGetterFunction(ParseFunction* func);
		void setSetterFunction(ParseFunction* func);

		ParseFunction* getterFunction() const;
		ParseFunction* setterFunction() const;
		BaseParse* setter() const;
		BaseParse* getter() const;
		BaseParse* object() const;
		const std::wstring& property() const;

		void simplify() override;
	};

}

