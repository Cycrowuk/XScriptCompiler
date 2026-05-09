#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ParseFail : public BaseParse
	{
	public:
		static int _DEBUG_FAILS;

	protected:
		std::vector<std::wstring> _data;
		BaseParse* _parsed;
		ParseErrors _error;

	public:
		ParseFail(const std::wstring& l, ParseErrors e);
		ParseFail(const BaseParse *p, ParseErrors e);
		virtual ~ParseFail();

		void addData(const std::wstring& str);

		std::wstring data(size_t i) const;
		ParseErrors error() const;
	};

}

